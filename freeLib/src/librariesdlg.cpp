#define QT_USE_QSTRINGBUILDER
#include "librariesdlg.h"
#include "ui_librariesdlg.h"

#include <QMessageBox>
#include <QToolButton>
#include <QStringBuilder>
#include <QDir>
#include <QFileDialog>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>

#include "exportdlg.h"
#include "utilites.h"

LibrariesDlg::LibrariesDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LibrariesDlg)
{
    bLibChanged = false;
    ui->setupUi(this);

    QToolButton* tbInpx = new QToolButton(this);
    tbInpx->setFocusPolicy(Qt::NoFocus);
    tbInpx->setCursor(Qt::ArrowCursor);
    tbInpx->setText(QStringLiteral("..."));
    QHBoxLayout* layout = new QHBoxLayout(ui->inpx);
    layout->addWidget(tbInpx, 0, Qt::AlignRight);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    QToolButton* tbBooksDir = new QToolButton(this);
    tbBooksDir->setFocusPolicy(Qt::NoFocus);
    tbBooksDir->setCursor(Qt::ArrowCursor);
    tbBooksDir->setText(QStringLiteral("..."));
    layout = new QHBoxLayout(ui->BookDir);
    layout->addWidget(tbBooksDir, 0, Qt::AlignRight);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    bool darkTheme = palette().color(QPalette::Window).lightness() < 127;
    QString sIconsPath = QStringLiteral(":/img/icons/") + (darkTheme ?QStringLiteral("dark/") :QStringLiteral("light/"));
    ui->Add->setIcon(QIcon::fromTheme(QStringLiteral("list-add"), QIcon(sIconsPath + QStringLiteral("plus.svg"))));
    ui->Del->setIcon(QIcon::fromTheme(QStringLiteral("list-remove"), QIcon(sIconsPath + QStringLiteral("minus.svg"))));

    idCurrentLib_ = idCurrentLib;
    UpdateLibList();

    connect(tbInpx, &QAbstractButton::clicked, this, &LibrariesDlg::InputINPX);
    connect(tbBooksDir, &QAbstractButton::clicked, this, &LibrariesDlg::SelectBooksDir);
    connect(ui->btnUpdate, &QPushButton::clicked, this, [this](){this->StartImport();});
    connect(ui->btnExport, &QAbstractButton::clicked, this, &LibrariesDlg::ExportLib);
    connect(ui->btnAddBook, &QPushButton::clicked, this, &LibrariesDlg::addBook);
    connect(ui->ExistingLibs, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int index){this->onComboboxLibraryChanged(index);});
    connect(ui->Del, &QAbstractButton::clicked, this, &LibrariesDlg::DeleteLibrary);
    connect(ui->Add, &QAbstractButton::clicked, this, &LibrariesDlg::Add_Library);
    connect(ui->ExistingLibs->lineEdit(), &QLineEdit::editingFinished, this, &LibrariesDlg::ExistingLibsChanged);
    ui->add_new->setChecked(true);

    SelectLibrary();
}

LibrariesDlg::~LibrariesDlg()
{
    delete ui;
}

void LibrariesDlg::Add_Library()
{
    idCurrentLib_ = 0;
    QString sNewName = UniqueName( tr("new") );
    ui->ExistingLibs->blockSignals(true);
    ui->ExistingLibs->addItem(sNewName, 0);
    SLib lib;
    lib.name = sNewName;
    lib.bFirstAuthor = false;
    lib.bWoDeleted = false;
    SaveLibrary(lib);
    ui->ExistingLibs->blockSignals(false);
    emit ui->ExistingLibs->currentIndexChanged(ui->ExistingLibs->count()-1);
}

void LibrariesDlg::LogMessage(const QString &msg)
{
    while(ui->Log->count() > 100)
        delete ui->Log->takeItem(0);
    ui->Log->addItem(msg);
    ui->Log->setCurrentRow(ui->Log->count()-1);
}

void LibrariesDlg::InputINPX()
{
    QDir::setCurrent(QFileInfo(ui->inpx->text()).absolutePath());
    QString fileName = QFileDialog::getOpenFileName(this, tr("Add library"),QLatin1String(""), tr("Library") + QStringLiteral(" (*.inpx)"));
    if(!fileName.isEmpty())
    {
        ui->inpx->setText(fileName);
        ui->BookDir->setText(QFileInfo(fileName).absolutePath());
        QString sLib = SLib::nameFromInpx(fileName);
        if(!sLib.isEmpty())
            ui->ExistingLibs->setItemText(ui->ExistingLibs->currentIndex(), sLib);
    }
}

void LibrariesDlg::SelectBooksDir()
{
    QDir::setCurrent(ui->BookDir->text());
    QString dir = QFileDialog :: getExistingDirectory(this, tr("Select books directory"));
    if(!dir.isEmpty())
        ui->BookDir->setText(dir);
}

void LibrariesDlg::UpdateLibList()

{
    if(!QSqlDatabase::database(QStringLiteral("libdb"), false).isOpen())
        return;
    bool block = ui->ExistingLibs->blockSignals(true);
    ui->ExistingLibs->clear();
    auto i = libs.constBegin();
    int index = 0;
    while(i != libs.constEnd()){
        uint idLib = i.key();
        if(idLib > 0){
            ui->ExistingLibs->addItem(i->name, idLib);
            if(idLib == idCurrentLib_)
                ui->ExistingLibs->setCurrentIndex(index);
            ++index;
        }
        ++i;
    }
    ui->ExistingLibs->setEnabled(ui->ExistingLibs->count()>0);
    ui->ExistingLibs->blockSignals(block);
}

void LibrariesDlg::StartImport()
{
    SLib *lib;
    if(idCurrentLib_ != 0)
        lib = &libs[idCurrentLib_];
    else
        lib = new SLib;
    lib->name = ui->ExistingLibs->currentText().trimmed();
    lib->sInpx = ui->inpx->text().trimmed();
    lib->path = ui->BookDir->text().trimmed();
    lib->bFirstAuthor = ui->firstAuthorOnly->isChecked();
    lib->bWoDeleted = ui->checkwoDeleted->isChecked();
    StartImport(*lib);
    if(lib != &libs[idCurrentLib_])
        delete lib;
}

void LibrariesDlg::StartImport(const SLib &lib)
{
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

    uchar nUdateType = (ui->add_new->isChecked() ?UT_NEW :ui->del_old->isChecked() ?UT_DEL_AND_NEW :UT_FULL);
    SaveLibrary(lib);
    ui->btnUpdate->setDisabled(true);
    ui->BookDir->setDisabled(true);
    ui->inpx->setDisabled(true);
    ui->ExistingLibs->setDisabled(true);
    ui->Del->setDisabled(true);
    ui->Add->setDisabled(true);
    ui->firstAuthorOnly->setDisabled(true);
    ui->checkwoDeleted->setDisabled(true);
    ui->btnCancel->setText(tr("Break"));
    ui->update_group->hide();

    pThread_ = new QThread;
    pImportThread_ = new ImportThread();
    pImportThread_->init(idCurrentLib_, lib, nUdateType);
    pImportThread_->moveToThread(pThread_);
    connect(pImportThread_, &ImportThread::Message, this, &LibrariesDlg::LogMessage);
    connect(pThread_, &QThread::started, pImportThread_, &ImportThread::process);
    connect(pImportThread_, &ImportThread::End, pThread_, &QThread::quit);
    connect(pThread_, &QThread::finished, pThread_, &QObject::deleteLater);
    connect(pImportThread_, &ImportThread::End, this, &LibrariesDlg::EndUpdate);
    connect(this, &LibrariesDlg::break_import, pImportThread_, &ImportThread::break_import);

    pThread_->start();
}

void LibrariesDlg::AddNewLibrary(SLib &lib)
{
    if(!QSqlDatabase::database(QStringLiteral("libdb"), false).isOpen())
    {
        openDB(QStringLiteral("libdb"));
    }
    idCurrentLib_ = 0;
    StartImport(lib);
    exec();
}

void LibrariesDlg::SelectLibrary()
{
    QString dir,inpx;
    bool firstAuthor = false;
    bool bWoDeleted = false;
    if(idCurrentLib_ > 0){
        dir = libs[idCurrentLib_].path;
        inpx = libs[idCurrentLib_].sInpx;
        firstAuthor = libs[idCurrentLib_].bFirstAuthor;
        bWoDeleted = libs[idCurrentLib_].bWoDeleted;
    }

    ui->BookDir->setText(dir);
    ui->inpx->setText(inpx);
    ui->firstAuthorOnly->setChecked(firstAuthor);
    ui->checkwoDeleted->setChecked(bWoDeleted);
    ui->Del->setDisabled(idCurrentLib_ == 0);
    ui->ExistingLibs->setDisabled(idCurrentLib_ == 0);
    ui->inpx->setDisabled(idCurrentLib_ == 0);
    ui->BookDir->setDisabled(idCurrentLib_ == 0);
    ui->btnUpdate->setDisabled(idCurrentLib_ == 0);
    ui->OPDS->setText(idCurrentLib_ == 0 ?QStringLiteral("") :QStringLiteral("<a href=\"http://localhost:%2/opds_%1\">http://localhost:%2/opds_%1</a>").arg(idCurrentLib_).arg(options.nOpdsPort));
    ui->HTTP->setText(idCurrentLib_ == 0 ?QStringLiteral("") :QStringLiteral("<a href=\"http://localhost:%2/http_%1\">http://localhost:%2/http_%1</a>").arg(idCurrentLib_).arg(options.nOpdsPort));

    auto settings = GetSettings();
    settings->setValue(QStringLiteral("LibID"), idCurrentLib_);
}

void LibrariesDlg::SaveLibrary(const SLib &lib)
{
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
    if(idCurrentLib_ == 0)
    {
        LogMessage(tr("Add library"));
        bool result = query.exec(QStringLiteral("INSERT INTO lib(name,path,inpx,firstAuthor,woDeleted) values('%1','%2','%3',%4,%5)")
                                 .arg(lib.name, lib.path, lib.sInpx, lib.bFirstAuthor ?"1" :"0", lib.bWoDeleted ?"1" :"0"));
        if(!result)
            qDebug()<<query.lastError().databaseText();
        idCurrentLib_ = query.lastInsertId().toInt();
        auto settings = GetSettings();
        settings->setValue(QStringLiteral("LibID"), idCurrentLib_);
        SLib &newLib = libs[idCurrentLib_];
        newLib.name = lib.name;
        newLib.path = lib.path;
        newLib.sInpx = lib.sInpx;
        newLib.bFirstAuthor = lib.bFirstAuthor;
        newLib.bWoDeleted = lib.bWoDeleted;
    }
    else
    {
        LogMessage(tr("Update library"));
        bool result = query.exec(QStringLiteral("UPDATE Lib SET name='%1',path='%2',inpx='%3' ,firstAuthor=%4, woDeleted=%5 WHERE ID=%6")
                                 .arg(lib.name, lib.path, lib.sInpx, lib.bFirstAuthor ?QStringLiteral("1"): QStringLiteral("0"), lib.bWoDeleted ? QStringLiteral("1"): QStringLiteral("0")).arg(idCurrentLib_));
        if(!result)
            qDebug()<<query.lastError().databaseText();
    }

    UpdateLibList();
    bLibChanged = true;
}

void LibrariesDlg::DeleteLibrary()
{
    if(idCurrentLib_ == 0)
        return;

    if(QMessageBox::question(this, tr("Delete library"), tr("Delete library ") +"\"" + ui->ExistingLibs->currentText() + "\"?", QMessageBox::Yes|QMessageBox::No,QMessageBox::No)==QMessageBox::No)
        return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
    query.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    if(!query.exec(QStringLiteral("DELETE FROM lib where ID=") + QString::number(idCurrentLib_)))
        qDebug()<<query.lastError().databaseText();
    query.exec(QStringLiteral("VACUUM"));
    libs.remove(idCurrentLib_);
    UpdateLibList();
    if(ui->ExistingLibs->count() > 0){
        ui->ExistingLibs->setCurrentIndex(0);
        idCurrentLib_ = ui->ExistingLibs->itemData(0).toInt();
    }else
        idCurrentLib_ = 0;
    SelectLibrary();
    bLibChanged = true;
    QApplication::restoreOverrideCursor();
}

void LibrariesDlg::EndUpdate()
{
    LogMessage(tr("Ending"));
    ui->btnUpdate->setDisabled(false);
    ui->btnCancel->setText(tr("Close"));
    ui->BookDir->setDisabled(false);
    ui->inpx->setDisabled(false);
    ui->Del->setDisabled(false);
    ui->Add->setDisabled(false);
    ui->ExistingLibs->setDisabled(false);
    ui->update_group->show();
    ui->firstAuthorOnly->setDisabled(false);
    ui->checkwoDeleted->setDisabled(false);
    bLibChanged = true;

    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
    query.prepare(QStringLiteral("SELECT version FROM lib WHERE id=:idLib;"));
    query.bindValue(QStringLiteral(":idLib"),idCurrentLib_);
    if(!query.exec())
        qDebug() << query.lastError().text();
    else{
        if(query.next())
            libs[idCurrentLib_].sVersion = query.value(0).toString();
    }


    QApplication::restoreOverrideCursor();
}

void LibrariesDlg::terminateImport()
{
    emit break_import();
}

void LibrariesDlg::reject()
{
    if (ui->btnCancel->text() == tr("Close"))
    {
        if(idCurrentLib_ != idCurrentLib){
            bLibChanged = true;
            idCurrentLib = idCurrentLib_;
        }
        QDialog::reject();
    }
    else
    {
        terminateImport();
    }
}

void LibrariesDlg::ExistingLibsChanged()
{
    ui->ExistingLibs->setItemText(ui->ExistingLibs->currentIndex(),ui->ExistingLibs->lineEdit()->text());
}

void LibrariesDlg::ExportLib()
{
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Select destination directory"));
    if (!dirName.isEmpty())
    {
        ExportDlg ed(this);
        ed.exec(idCurrentLib_, dirName);
    }
}

void LibrariesDlg::addBook()
{
    QStringList listFiles = QFileDialog::getOpenFileNames(this, tr("Select books to add"), ui->BookDir->text(),
                                                      tr("Books (*.fb2 *.epub *.zip)"), nullptr, QFileDialog::ReadOnly);
    if(!listFiles.isEmpty()){
        QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
        SLib *pLib;
        if(idCurrentLib_ != 0)
            pLib = &libs[idCurrentLib_];
        else
            pLib = new SLib;
        pLib->name = ui->ExistingLibs->currentText().trimmed();
        pLib->sInpx = ui->inpx->text().trimmed();
        pLib->path = ui->BookDir->text().trimmed();
        if(pLib->path.endsWith(u"/"))
            pLib->path.chop(1);;
        pLib->bFirstAuthor = ui->firstAuthorOnly->isChecked();
        pLib->bWoDeleted = ui->checkwoDeleted->isChecked();

        SaveLibrary(*pLib);
        ui->btnUpdate->setDisabled(true);
        ui->BookDir->setDisabled(true);
        ui->inpx->setDisabled(true);
        ui->ExistingLibs->setDisabled(true);
        ui->Del->setDisabled(true);
        ui->Add->setDisabled(true);
        ui->firstAuthorOnly->setDisabled(true);
        ui->checkwoDeleted->setDisabled(true);
        ui->btnCancel->setText(tr("Break"));
        ui->update_group->hide();

        for(qsizetype i = 0 ; i < listFiles.size(); ++i) {
            QString sFile = listFiles.at(i);
            if(!sFile.startsWith(pLib->path)){
                //перенос файлов книг в папку библиотеки
                QFileInfo fiSrc = QFileInfo(sFile);
                QString baseName = fiSrc.baseName(); // получаем имя файла без расширения
                QString extension = fiSrc.completeSuffix(); // получаем расширение файла
                QFileInfo fiDst = QFileInfo(pLib->path + "/" + baseName + "." + extension);
                uint j = 1;
                while(fiDst.exists()){
                    QString sNewName = QStringLiteral("%1 (%2).%3").arg(baseName, QString::number(j), extension);
                    fiDst = QFileInfo(pLib->path + "/" + sNewName);
                    j++;
                }
                QFile::copy(sFile, fiDst.absoluteFilePath());
                listFiles[i] = fiDst.absoluteFilePath();
            }    
        }
        pThread_ = new QThread;
        pImportThread_ = new ImportThread();
        pImportThread_->init(idCurrentLib_, *pLib, listFiles);
        pImportThread_->moveToThread(pThread_);
        connect(pImportThread_, &ImportThread::Message, this, &LibrariesDlg::LogMessage);
        connect(pThread_, &QThread::started, pImportThread_, &ImportThread::process);
        connect(pImportThread_, &ImportThread::End, pThread_, &QThread::quit);
        connect(pThread_, &QThread::finished, pThread_, &QObject::deleteLater);
        connect(pImportThread_, &ImportThread::End, this, &LibrariesDlg::EndUpdate);
        connect(this, &LibrariesDlg::break_import, pImportThread_, &ImportThread::break_import);

        pThread_->start();
    }
}

void LibrariesDlg::onComboboxLibraryChanged(int index)
{
    if(index >= 0){
        idCurrentLib_ = ui->ExistingLibs->itemData(index).toInt();
        SelectLibrary();
    }
}

QString LibrariesDlg::UniqueName(const QString &sName)
{
    QString sResult = sName;
    int i = 1;
    while(ui->ExistingLibs->findText(sResult) != -1){
        sResult = QStringLiteral("%1 (%2)").arg(sName, QString::number(i++));
    }
    return sResult;
}


