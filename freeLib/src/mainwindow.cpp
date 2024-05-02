#define QT_USE_QSTRINGBUILDER
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtSql>
#include <QSplashScreen>
#include <QButtonGroup>
#include <QMessageBox>
#include <QDesktopServices>
#include <QTextBrowser>
#include <QCloseEvent>
#include <QWidgetAction>
#include <QtConcurrent>


#include "librariesdlg.h"
#include "settingsdlg.h"
#include "exportdlg.h"
#include "aboutdialog.h"
#include "tagdialog.h"
#include "bookeditdlg.h"
#include "treebookitem.h"
#include "genresortfilterproxymodel.h"
#include "library.h"
#include "starsdelegate.h"
#include "opds_server.h"
#include "statisticsdialog.h"
#include "utilites.h"


using namespace std::chrono_literals;
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
QString sizeToString(uint size)
{
    QStringList mem;
    mem <<QCoreApplication::translate("MainWindow","B")<<QCoreApplication::translate("MainWindow","kB")<<QCoreApplication::translate("MainWindow","MB")<<QCoreApplication::translate("MainWindow","GB")<<QCoreApplication::translate("MainWindow","TB")<<QCoreApplication::translate("MainWindow","PB");
    uint rest=0;
    int mem_i=0;

    while(size>1024)
    {
        mem_i++;
        if(mem_i==mem.count())
        {
            mem_i--;
            break;
        }
        rest=size%1024;
        size=size/1024;
     }
    double size_d = (float)size + (float)rest / 1024.0;
    return QString("%L1 %2").arg(size_d,0,'f',mem_i>0?1:0).arg(mem[mem_i]);
}
#endif

extern bool bTray;
extern bool bVerbose;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    pTrayIcon_ = nullptr;
    pTrayMenu_ = nullptr;
    pHideAction_ = nullptr;
    pShowAction_ = nullptr;
    error_quit = false;

    auto settings = GetSettings();

    ui->setupUi(this);
    ui->btnEdit->setVisible(false);
    bTreeView_ = true;
    bTreeView_ = settings->value(QStringLiteral("TreeView"), true).toBool();
    ui->btnTreeView->setChecked(bTreeView_);
    ui->btnListView->setChecked(!bTreeView_);;

    ui->tabWidget->setCurrentIndex(0);
    ui->Books->setColumnWidth(0,400);
    ui->Books->setColumnWidth(3,50);
    ui->Books->setColumnWidth(4,100);
    ui->Books->setColumnWidth(5,90);
    ui->Books->setColumnWidth(6,120);
    ui->Books->setColumnWidth(7,250);
    ui->Books->setColumnWidth(8,50);
    ui->Books->setColumnWidth(9,50);

    pCover = new CoverLabel(this);
    ui->horizontalLayout_3->addWidget(pCover);

    if(settings->contains(QStringLiteral("MainWnd/geometry")))
        restoreGeometry(settings->value(QStringLiteral("MainWnd/geometry")).toByteArray());
    if(settings->contains(QStringLiteral("MainWnd/windowState")))
        restoreState(settings->value(QStringLiteral("MainWnd/windowState")).toByteArray());
    if(settings->contains(QStringLiteral("MainWnd/VSplitterSizes")))
        ui->splitterV->restoreState(settings->value(QStringLiteral("MainWnd/VSplitterSizes")).toByteArray());
    if(settings->contains(QStringLiteral("MainWnd/HSplitterSizes")))
        ui->splitterH->restoreState(settings->value(QStringLiteral("MainWnd/HSplitterSizes")).toByteArray());

    bool darkTheme = palette().color(QPalette::Window).lightness() < 127;
    QString sIconsPath = QStringLiteral(":/img/icons/") + (darkTheme ?QStringLiteral("dark/") :QStringLiteral("light/"));
    ui->btnOpenBook->setIcon(QIcon(sIconsPath + QStringLiteral("book.svg")));
    ui->btnEdit->setIcon(QIcon::fromTheme(QStringLiteral("document-edit"),QIcon(sIconsPath + QStringLiteral("pen.svg"))));
    ui->btnCheck->setIcon(QIcon::fromTheme(QStringLiteral("checkbox"), QIcon(sIconsPath + QStringLiteral("checkbox.svg"))));
    ui->btnLibrary->setIcon(QIcon(sIconsPath + QStringLiteral("library.svg")));
    ui->btnOption->setIcon(QIcon::fromTheme(QStringLiteral("settings-configure"), QIcon(sIconsPath + QStringLiteral("settings.svg"))));
    ui->btnTreeView->setIcon(QIcon(sIconsPath + QStringLiteral("view-tree.svg")));
    ui->btnListView->setIcon(QIcon(sIconsPath + QStringLiteral("view-list.svg")));

    GenreSortFilterProxyModel *MyProxySortModel = new GenreSortFilterProxyModel(ui->s_genre);
    MyProxySortModel->setSourceModel(ui->s_genre->model());
    ui->s_genre->model()->setParent(MyProxySortModel);
    ui->s_genre->setModel(MyProxySortModel);
    MyProxySortModel->setDynamicSortFilter(false);

    StarsDelegate* delegate = new StarsDelegate(this);
    ui->Books->setItemDelegateForColumn(5, delegate);

    idCurrentLanguage_ = -1;

    int nCurrentTab;

    QString sFilter;
    if(options.bStorePosition)
    {
        idCurrentAuthor_= settings->value(QStringLiteral("current_author_id"), 0).toUInt();
        idCurrentSerial_ = settings->value(QStringLiteral("current_serial_id"), 0).toUInt();
        idCurrentBook_ = settings->value(QStringLiteral("current_book_id"), 0).toUInt();
        idCurrentGenre_ = settings->value(QStringLiteral("current_genre_id"), 0).toUInt();
        nCurrentTab = settings->value(QStringLiteral("current_tab"), 0).toInt();
        sFilter = settings->value(QStringLiteral("filter_set")).toString();
    }
    else
    {
        idCurrentAuthor_ = 0;
        idCurrentSerial_ = 0;
        idCurrentBook_ = 0;
        idCurrentGenre_ = 0;
        nCurrentTab = TabAuthors;
    }

    UpdateTags();
    loadGenres();
    loadLibrary(idCurrentLib);
    fillLanguages();

//    FillAuthors();
//    FillSerials();
    FillGenres();

    connect(ui->searchAuthor, &QLineEdit::textChanged, this, &MainWindow::onSerachAuthorsChanded);
    connect(ui->searchSeries, &QLineEdit::textChanged, this, &MainWindow::onSerachSeriesChanded);
    connect(ui->actionAddLibrary, &QAction::triggered, this, &MainWindow::ManageLibrary);
    connect(ui->actionStatistics, &QAction::triggered, this, &MainWindow::onStatistics);
    connect(ui->actionAddBooks, &QAction::triggered, this, &MainWindow::onAddBooks);
    connect(ui->btnLibrary, &QAbstractButton::clicked, this, &MainWindow::ManageLibrary);
    connect(ui->btnOpenBook, &QAbstractButton::clicked, this, &MainWindow::BookDblClick);
    connect(ui->btnOption, &QAbstractButton::clicked, this, &MainWindow::Settings);
    connect(ui->actionPreference, &QAction::triggered, this, &MainWindow::Settings);
    connect(ui->actionCheck_uncheck, &QAction::triggered, this, &MainWindow::CheckBooks);
    connect(ui->btnCheck, &QAbstractButton::clicked, this, &MainWindow::CheckBooks);
    connect(ui->btnEdit, &QAbstractButton::clicked, this, &MainWindow::EditBooks);
    connect(ui->btnTreeView, &QAbstractButton::clicked, this, &MainWindow::onTreeView);
    connect(ui->btnListView, &QAbstractButton::clicked, this, &MainWindow::onListView);
    connect(ui->actionExit, &QAction::triggered, qApp, &QApplication::quit);
    #ifdef Q_OS_MACX
        ui->actionExit->setShortcut(QKeySequence(Qt::CTRL|Qt::Key_Q));
    #endif
    #ifdef Q_OS_LINUX
        ui->actionExit->setShortcut(QKeySequence(Qt::CTRL|Qt::Key_Q));
        setWindowIcon(QIcon(QStringLiteral(":/library.png")));
    #endif
    #ifdef Q_OS_WIN
        ui->actionExit->setShortcut(QKeySequence(Qt::ALT|Qt::Key_F4));
    #endif
    #ifdef Q_OS_WIN32
        ui->actionExit->setShortcut(QKeySequence(Qt::ALT|Qt::Key_F4));
    #endif
    connect(ui->AuthorList, &QListWidget::itemSelectionChanged, this, &MainWindow::SelectAuthor);
    connect(ui->Books, &QTreeWidget::itemSelectionChanged, this, &MainWindow::SelectBook);
    connect(ui->Books, &QTreeWidget::itemDoubleClicked, this, &MainWindow::BookDblClick);
    connect(ui->GenreList, &QTreeWidget::itemSelectionChanged, this, &MainWindow::SelectGenre);
    connect(ui->SeriaList, &QListWidget::itemSelectionChanged, this, &MainWindow::SelectSeria);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabWidgetChanged);
    connect(ui->do_search, &QAbstractButton::clicked, this, &MainWindow::StartSearch);
    connect(ui->s_author, &QLineEdit::returnPressed, this, &MainWindow::StartSearch);
    connect(ui->s_seria, &QLineEdit::returnPressed, this, &MainWindow::StartSearch);
    connect(ui->s_name, &QLineEdit::returnPressed, this, &MainWindow::StartSearch);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::About);
    connect(ui->Books, &QTreeWidget::itemChanged, this, &MainWindow::onItemChanged);
    connect(ui->language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::onLanguageFilterChanged);
    connect(ui->Review, &QTextBrowser::anchorClicked, this, &MainWindow::ReviewLink );

    FillAlphabet(options.sAlphabetName);
    ExportBookListBtn(false);

    setWindowTitle(QStringLiteral("freeLib") + ((idCurrentLib==0 || libs[idCurrentLib].name.isEmpty() ?QStringLiteral("") :QStringLiteral(" — ")) + libs[idCurrentLib].name));

    ui->tabWidget->setCurrentIndex(nCurrentTab);
    switch (nCurrentTab) {
    case TabAuthors:
        ui->searchAuthor->setText(sFilter);
        ui->searchAuthor->setFocus();
        if(sFilter.trimmed().isEmpty())
            FirstButton->click();
        SelectAuthor();
        break;
    case TabSeries:
        ui->searchSeries->setText(sFilter);
        ui->searchSeries->setFocus();
        break;
    }

    ui->date_to->setDate(QDate::currentDate());

    pHelpDlg = nullptr;
    connect(ui->actionHelp, &QAction::triggered, this, &MainWindow::HelpDlg);
    ui->Books->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->Books, &QWidget::customContextMenuRequested, this, &MainWindow::ContextMenu);
    ui->AuthorList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->AuthorList, &QWidget::customContextMenuRequested, this, &MainWindow::ContextMenu);
    ui->SeriaList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->SeriaList, &QWidget::customContextMenuRequested, this, &MainWindow::ContextMenu);
    connect(ui->TagFilter, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &MainWindow::onTagFilterChanged);
    ui->Books->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->Books->header(), &QWidget::customContextMenuRequested, this, &MainWindow::HeaderContextMenu);

    if(options.bOpdsEnable){
        pOpds_ = std::unique_ptr<opds_server>( new opds_server(this) );
        pOpds_->server_run();
    }
    FillLibrariesMenu();
    UpdateExportMenu();

    ChangingTrayIcon(options.nIconTray, options.nTrayColor);

    connect(ui->actionMinimize_window, &QAction::triggered, this, &MainWindow::MinimizeWindow);

    settings->beginGroup(QStringLiteral("Columns"));
    QVariant varHeaders = settings->value(QStringLiteral("headersTree"));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if(varHeaders.type() == QVariant::ByteArray)
#else
    if(varHeaders.metaType().id() == QMetaType::QByteArray)
#endif
        aHeadersTree_ = varHeaders.toByteArray();
    else{
        ui->Books->setColumnHidden(1, true);
        ui->Books->setColumnHidden(2, true);
        ui->Books->setColumnHidden(9, true);
    }
    varHeaders = settings->value(QStringLiteral("headersList"));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if(varHeaders.type() == QVariant::ByteArray)
#else
    if(varHeaders.metaType().id() == QMetaType::QByteArray)
#endif
        aHeadersList_ = varHeaders.toByteArray();
    settings->endGroup();
    if(bTreeView_)
        ui->Books->header()->restoreState(aHeadersTree_);
    else
        ui->Books->header()->restoreState(aHeadersList_);
    ui->Books->header()->setSortIndicatorShown(true);
}

void MainWindow::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
}

void MainWindow::UpdateTags()
{
    if(!QSqlDatabase::database(QStringLiteral("libdb"), false).isOpen())
        return;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    auto settings = GetSettings();
    bool darkTheme = palette().color(QPalette::Window).lightness() < 127;

    QButtonGroup *group = new QButtonGroup(this);
    group->setExclusive(true);

    iconsTags_.clear();
    QString sMultiTagsIcon = QStringLiteral(":/img/icons/") +(darkTheme ?QStringLiteral("dark/") :QStringLiteral("light/")) + QStringLiteral("multitags.svg");
    iconsTags_[0] = QIcon(sMultiTagsIcon);
    QMap<uint, QIcon> icons;
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
    query.exec(QStringLiteral("SELECT id, light_theme, dark_theme FROM icon"));
    while(query.next())
    {
        QPixmap pixIcon;
        uint idIcon = query.value(0).toUInt();
        QByteArray baLightIcon = query.value(1).toByteArray();
        QByteArray baDarkIcon = query.value(2).toByteArray();
        if(darkTheme){
            if(baDarkIcon.isEmpty())
                pixIcon.loadFromData(baLightIcon);
            else
                pixIcon.loadFromData(baDarkIcon);
        }else
            pixIcon.loadFromData(baLightIcon);
        icons[idIcon] = pixIcon;
    }

    const bool wasBlocked = ui->TagFilter->blockSignals(true);

    query.exec(QStringLiteral("SELECT id,name,id_icon FROM tag"));
    //                                0  1    2
    ui->TagFilter->clear();
    int con = 1;
    ui->TagFilter->addItem(QStringLiteral("*"), 0);
    TagMenu.clear();
    ui->TagFilter->setVisible(options.bUseTag);
    ui->tag_label->setVisible(options.bUseTag);

    while(query.next())
    {
        uint idTag = query.value(0).toUInt();
        QString sTagName = query.value(1).toString().trimmed();
        bool bLatin1 = true;
        for(int i=0; i<sTagName.length(); i++){
            if(sTagName.at(i).unicode() > 127){
                bLatin1 = false;
                break;
            }
        }
        if(bLatin1)
            sTagName = TagDialog::tr(sTagName.toLatin1().data());
        ui->TagFilter->addItem(sTagName, idTag);
        if(settings->value(QStringLiteral("current_tag")).toInt() == ui->TagFilter->count()-1 && options.bUseTag)
            ui->TagFilter->setCurrentIndex(ui->TagFilter->count()-1);
        QAction *ac;
        uint idIcon = query.value(2).toUInt();
        QIcon &iconTag = icons[idIcon];
        ui->TagFilter->setItemIcon(con, iconTag);
        ac = new QAction(iconTag, sTagName, &TagMenu);
        ac->setCheckable(true);
        iconsTags_[idTag] = iconTag;
        ac->setData(idTag);
        connect(ac, &QAction::triggered, this, &MainWindow::onSetTag);
        TagMenu.addAction(ac);
        con++;
    }

    updateIcons();

    ui->TagFilter->addItem(tr("setup ..."), -1);
    ui->TagFilter->blockSignals(wasBlocked);

    QApplication::restoreOverrideCursor();
}

void MainWindow::updateIcons()
{
    bool wasBlocked = ui->SeriaList->blockSignals(true);
    for(int i=0; i<ui->SeriaList->count(); i++){
        auto item = ui->SeriaList->item(i);
        uint idSerial = item->data(Qt::UserRole).toUInt();
        item->setIcon(getTagIcon(libs[idCurrentLib].serials[idSerial].listIdTags));
    }
    ui->SeriaList->blockSignals(wasBlocked);

    wasBlocked = ui->AuthorList->blockSignals(true);
    for(int i=0; i<ui->AuthorList->count(); i++){
        auto item = ui->AuthorList->item(i);
        uint idAuthor = item->data(Qt::UserRole).toUInt();
        item->setIcon(getTagIcon(libs[idCurrentLib].authors[idAuthor].listIdTags));
    }
    ui->AuthorList->blockSignals(wasBlocked);

    wasBlocked = ui->Books->blockSignals(true);
    for(int i=0; i<ui->Books->topLevelItemCount(); i++){
        auto item1 = ui->Books->topLevelItem(i);
        updateItemIcon(item1);
        for(int j=0; j<item1->childCount(); j++){
            auto item2 = item1->child(j);
            updateItemIcon(item2);
                for(int k=0; k<item2->childCount(); k++){
                    auto item3 = item2->child(k);
                    updateItemIcon(item3);
                }
        }
    }
    ui->Books->blockSignals(wasBlocked);
}

void MainWindow::updateItemIcon(QTreeWidgetItem *item)
{
    uint id = item->data(0, Qt::UserRole).toUInt();
    switch(item->type()){
    case ITEM_TYPE_BOOK:
        item->setIcon(0, getTagIcon(libs[idCurrentLib].books[id].listIdTags));
        break;
    case ITEM_TYPE_SERIA:
        item->setIcon(0, getTagIcon(libs[idCurrentLib].serials[id].listIdTags));
        break;
    case ITEM_TYPE_AUTHOR:
        item->setIcon(0, getTagIcon(libs[idCurrentLib].authors[id].listIdTags));
        break;
    }
}

QIcon MainWindow::getTagIcon(const QList<uint> &listTags)
{
    int size = listTags.size();
    if(size == 0)
        return QIcon();
    if(size == 1)
        return iconsTags_[listTags.first()];
    return iconsTags_[0];
}

MainWindow::~MainWindow()
{
    if(pTrayIcon_)
        delete pTrayIcon_;
    if(pTrayMenu_)
        delete pTrayMenu_;
    if(pHelpDlg != nullptr)
        delete pHelpDlg;

    if(options.bStorePosition)
        SaveLibPosition();
    auto settings = GetSettings();
    settings->beginGroup(QStringLiteral("Columns"));
    if(bTreeView_){
        settings->setValue(QStringLiteral("headersTree"), ui->Books->header()->saveState());
        settings->setValue(QStringLiteral("headersList"), aHeadersList_);
    }else{
        settings->setValue(QStringLiteral("headersTree"), aHeadersTree_);
        settings->setValue(QStringLiteral("headersList"), ui->Books->header()->saveState());
    }
    settings->endGroup();

    settings->setValue(QStringLiteral("MainWnd/geometry"), saveGeometry());
    settings->setValue(QStringLiteral("MainWnd/windowState"), saveState());
    settings->setValue(QStringLiteral("MainWnd/VSplitterSizes"), ui->splitterV->saveState());
    settings->setValue(QStringLiteral("MainWnd/HSplitterSizes"), ui->splitterH->saveState());
    QString TempDir;
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count() > 0)
        TempDir = QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
    QDir(TempDir + QStringLiteral("/freeLib/")).removeRecursively();

    delete ui;
}

void MainWindow::EditBooks()
{
    BookEditDlg dlg(this);
    dlg.exec();
}

/*
    обработчик клика мышкой на ссылках в описании Книги
*/
void MainWindow::ReviewLink(const QUrl &url)
{
    QString sPath = url.path();
    if(sPath.startsWith(u"author_"))
    {
        MoveToAuthor(sPath.right(sPath.length()-8).toLongLong(), sPath.mid(7,1).toUpper());
    }
    else if(sPath.startsWith(u"genre_"))
    {
        MoveToGenre(sPath.right(sPath.length()-7).toLongLong());
    }
    else if(sPath.startsWith(u"seria_"))
    {
        MoveToSeria(sPath.right(sPath.length()-7).toLongLong(), sPath.mid(6,1).toUpper());
    }
}

/*
    Изменение языка интерфейса
*/
void MainWindow::ChangingLanguage()
{
    ui->retranslateUi(this);
    FillListBooks();
}

void MainWindow::FillAlphabet(const QString &sAlphabetName)
{
    QFile file(QStringLiteral(":/language/abc_%1.txt").arg(sAlphabetName));
    QString sAlphabet;
    if(!file.fileName().isEmpty() && file.open(QFile::ReadOnly))
    {
        sAlphabet = QString::fromUtf8(file.readLine()).toUpper();
    }
    QVBoxLayout *layout_abc_all = new QVBoxLayout();

    FirstButton = nullptr;
    if(!sAlphabet.isEmpty())
    {
        QHBoxLayout *layout_abc = new QHBoxLayout();
        for(int i=0; i<sAlphabet.length(); i++)
        {
            QToolButton *btn = new QToolButton(this);
            btn->setText(sAlphabet.at(i));
            btn->setMaximumSize(QSize(22, 22));
            btn->setMinimumSize(QSize(22, 22));
            btn->setFont(QFont(QStringLiteral("Noto Sans"), 10));
            btn->setCheckable(true);
            btn->setAutoExclusive(true);
            btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            layout_abc->addWidget(btn);
            connect(btn, &QAbstractButton::clicked, this, &MainWindow::btnSearch);
            if(!FirstButton)
                FirstButton = btn;
        }
        layout_abc->addStretch();
        layout_abc->setSpacing(1);
        layout_abc->setContentsMargins(0, 0, 0, 0);
        layout_abc_all->addItem(layout_abc);
    }
    QString abc = QStringLiteral("*#ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    QHBoxLayout *layout_abc = new QHBoxLayout();
    for(int i=0; i<abc.length(); i++)
    {
        QToolButton *btn = new QToolButton(this);
        btn->setText(abc.at(i));
        btn->setMaximumSize(QSize(22, 22));
        btn->setMinimumSize(QSize(22, 22));
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        layout_abc->addWidget(btn);
        connect(btn, &QAbstractButton::clicked, this, &MainWindow::btnSearch);
        if(!FirstButton && abc.at(i) == 'A')
            FirstButton = btn;
        if(abc.at(i) == '#')
            btn_Hash = btn;
    }
    layout_abc->addStretch();
    layout_abc->setSpacing(1);
#ifdef Q_OS_WIN
    layout_abc->setContentsMargins(0,!sAlphabet.isEmpty()?4:0,0,0);
#else
    layout_abc->setContentsMargins(0,!sAlphabet.isEmpty() ?5 :0, 0, 5);
#endif
    layout_abc_all->addItem(layout_abc);

    ui->abc->setLayout(layout_abc_all);
    ui->abc->layout()->setSpacing(1);
#ifdef Q_OS_WIN
    ui->abc->layout()->setContentsMargins(0,4,0,5);
#else
    ui->abc->layout()->setContentsMargins(0,0,0,0);
#endif
}

/*
    Установка метки на элемент
*/
void MainWindow::onSetTag()
{
    uint idTag = qobject_cast<QAction*>(QObject::sender())->data().toUInt();
    bool bSet = qobject_cast<QAction*>(QObject::sender())->isChecked();
    uint id;

    if(currentListForTag_ == ui->Books)
    {
        auto listItems = checkedItemsBookList();
        if(listItems.isEmpty())
            listItems = ui->Books->selectedItems();
        for(int iItem=0; iItem<listItems.size(); iItem++){
            QTreeWidgetItem* item = listItems.at(iItem);
            id = item->data(0, Qt::UserRole).toUInt();
            switch (item->type()) {
            case ITEM_TYPE_BOOK:
                setTag(idTag, id, libs[idCurrentLib].books[id].listIdTags, QStringLiteral("book"), bSet);
                break;

            case ITEM_TYPE_SERIA:
                setTag(idTag, id, libs[idCurrentLib].serials[id].listIdTags, QStringLiteral("seria"), bSet);

                break;

            case ITEM_TYPE_AUTHOR:
                setTag(idTag, id, libs[idCurrentLib].authors[id].listIdTags, QStringLiteral("author"), bSet);

                break;

            default:
                break;
            }
        }
    }
    else if(currentListForTag_ == ui->AuthorList)
    {
        id = ui->AuthorList->selectedItems().at(0)->data(Qt::UserRole).toUInt();
        setTag(idTag, id, libs[idCurrentLib].authors[id].listIdTags, QStringLiteral("author"), bSet);
    }
    else if(currentListForTag_ == ui->SeriaList)
    {
        id = ui->SeriaList->selectedItems().at(0)->data(Qt::UserRole).toUInt();
        setTag(idTag, id, libs[idCurrentLib].serials[id].listIdTags, QStringLiteral("seria"), bSet);
    }

    //TODO оптимизировать обновление иконок при большом количестве отображаемых авторов/книг
    updateIcons();
}

void MainWindow::setTag(uint idTag, uint id, QList<uint> &listIdTags,  QString sTable, bool bSet)
{
    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
    QString sQuery;
    QIcon icon;
    if(bSet){
        listIdTags << idTag;
        sQuery = QStringLiteral("INSERT INTO %1_tag (id_%1,id_tag) VALUES(%2,%3)").
                arg(sTable, QString::number(id), QString::number(idTag));
        icon = iconsTags_[idTag];
    }else{
        int index = listIdTags.indexOf(idTag);
        listIdTags.removeAt(index);
        query.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        sQuery = QStringLiteral("DELETE FROM %1_tag WHERE (id_%1=%2) AND (id_tag=%3)").
                arg(sTable, QString::number(id), QString::number(idTag));
    }
    query.exec(sQuery);
}

void MainWindow::onTagFilterChanged(int index)
{
    auto settings = GetSettings();
    if(ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toInt() == -1)
    {
        const bool wasBlocked = ui->TagFilter->blockSignals(true);
        ui->TagFilter->setCurrentIndex(settings->value(QStringLiteral("current_tag"), 0).toInt());
        ui->TagFilter->blockSignals(wasBlocked);
        TagDialog td(this);
        if(td.exec())
            UpdateTags();
    }
    else if(index >= 0)
    {
        settings->setValue(QStringLiteral("current_tag"), index);
        FillAuthors();
        FillSerials();
        FillGenres();
        FillListBooks();
    }
}

void MainWindow::SaveLibPosition()
{
    auto settings = GetSettings();
    switch (ui->tabWidget->currentIndex()) {
    case TabAuthors:
        settings->setValue(QStringLiteral("filter_set"), ui->searchAuthor->text());
        break;
    case TabSeries:
        settings->setValue(QStringLiteral("filter_set"), ui->searchSeries->text());
        break;
    }
    settings->setValue(QStringLiteral("current_tab"), ui->tabWidget->currentIndex());
    settings->setValue(QStringLiteral("current_book_id"), idCurrentBook_);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (pTrayIcon_ != nullptr && pTrayIcon_->isVisible()) {
        hide();
        event->ignore();
    }
}

/*
    Открытие окна настроек
*/
void MainWindow::Settings()
{
    SettingsDlg *pDlg = new SettingsDlg(this);
    connect(pDlg, &SettingsDlg::ChangingLanguage, this, [this](){this->ChangingLanguage();});
    connect(pDlg, &SettingsDlg::ChangeAlphabet, this, &MainWindow::onChangeAlpabet);
    connect(pDlg, &SettingsDlg::ChangingTrayIcon, this, &MainWindow::ChangingTrayIcon);
    if(pDlg->exec() == QDialog::Accepted){
        if(options.bShowDeleted != pDlg->options_.bShowDeleted || options.bUseTag != pDlg->options_.bUseTag)
        {
            UpdateTags();
            SaveLibPosition();
            FillAuthors();
            FillGenres();
            FillListBooks();
        }
        if(options.bOpdsEnable != pDlg->options_.bOpdsEnable || options.nOpdsPort != pDlg->options_.nOpdsPort ||
           options.bOpdsNeedPassword != pDlg->options_.bOpdsNeedPassword || options.sOpdsUser != pDlg->options_.sOpdsUser ||
           options.sOpdsPassword != pDlg->options_.sOpdsPassword)
        {
            if(pOpds_ == nullptr && options.bOpdsEnable)
                pOpds_ = std::unique_ptr<opds_server>( new opds_server(this) );
            pOpds_->server_run();
        }
        UpdateExportMenu();
        resizeEvent(nullptr);
    }
    pDlg->deleteLater();
}

/*
    Возвращает список отмеченных книг
*/
QList<uint> MainWindow::getCheckedBooks(bool bCheckedOnly)
{
    QList<uint> listBooks;
    FillCheckedItemsBookList(nullptr, false, &listBooks);
    if(listBooks.count() == 0 && !bCheckedOnly)
    {
        if(ui->Books->selectedItems().count() > 0)
        {
            if(ui->Books->selectedItems()[0]->childCount() > 0)
                FillCheckedItemsBookList(ui->Books->selectedItems()[0], true, &listBooks);
            else
            {
                if(ui->Books->selectedItems()[0]->type() == ITEM_TYPE_BOOK)
                {
                    uint idBook = ui->Books->selectedItems()[0]->data(0, Qt::UserRole).toUInt();
                    listBooks << idBook;
                }
            }
        }
    }
    return listBooks;
}

void MainWindow::FillCheckedItemsBookList(const QTreeWidgetItem* item, bool send_all, QList<uint> *pList)
{
    QTreeWidgetItem* current;
    int count = item ?item->childCount() :ui->Books->topLevelItemCount();
    for(int i=0; i < count; i++)
    {
        current = item ?item->child(i) :ui->Books->topLevelItem(i);
        if(current->childCount()>0)
        {
            FillCheckedItemsBookList(current, send_all, pList);
        }
        else
        {
            if(current->checkState(0) == Qt::Checked || send_all)
            {
                if(current->type() == ITEM_TYPE_BOOK)
                {
                    uint id_book = current->data(0, Qt::UserRole).toUInt();
                    *pList << id_book;
                }
            }
        }
    }
}

QList<QTreeWidgetItem*> MainWindow::checkedItemsBookList(const QTreeWidgetItem *item)
{
    QList<QTreeWidgetItem*> listItems;
    QTreeWidgetItem* current;
    int count = item ?item->childCount() :ui->Books->topLevelItemCount();
    for(int i=0; i < count; i++)
    {
        current = item ?item->child(i) :ui->Books->topLevelItem(i);
        if(current->childCount()>0)
        {
            listItems << checkedItemsBookList(current);
        }
        if(current->checkState(0) == Qt::Checked)
            listItems << current;
    }
    return listItems;
}

void MainWindow::uncheck_books(QList<qlonglong> list)
{
    if(!options.bUncheckAfterExport)
        return;
    QList<QTreeWidgetItem*> items;
    if(ui->Books->topLevelItemCount() == 0)
        return;
    for(auto id: list)
    {
        for(int i=0; i<ui->Books->topLevelItemCount(); i++)
            items << ui->Books->topLevelItem(i);
        do
        {
            if(items[0]->childCount() > 0)
            {
                for(int j=0; j<items[0]->childCount(); j++)
                    items << items[0]->child(j);
            }
            else
            {
                if(items[0]->data(0, Qt::UserRole).toLongLong() == id && items[0]->checkState(0) == Qt::Checked)
                    items[0]->setCheckState(0, Qt::Unchecked);
            }
            items.removeAt(0);
        }while(items.count() > 0);
        items.clear();
    }
}

void MainWindow::SendToDevice(const ExportOptions &exportOptions)
{
    QList<uint> book_list = getCheckedBooks();
    if(book_list.count() == 0)
        return;
    ExportDlg dlg(this);
    dlg.exec(book_list, ST_Device, ui->tabWidget->currentIndex() == TabAuthors ?ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toLongLong() :0, exportOptions);
    uncheck_books(dlg.succesfull_export_books);
}

void MainWindow::SendMail(const ExportOptions &exportOptions)
{
    QList<uint> book_list = getCheckedBooks();
    if(book_list.count() == 0)
        return;
    ExportDlg dlg(this);
    dlg.exec(book_list, ST_Mail, ui->tabWidget->currentIndex() == TabAuthors ?ui->AuthorList->selectedItems()[0]->data(Qt::UserRole).toLongLong() :0, exportOptions);
    uncheck_books(dlg.succesfull_export_books);
}


void MainWindow::BookDblClick()
{
    if(ui->Books->selectedItems().count() == 0)
        return;
    QTreeWidgetItem* item = ui->Books->selectedItems()[0];
    if(item->type() != ITEM_TYPE_BOOK)
        return;
    QBuffer outbuff;
    QFileInfo fi = libs[idCurrentLib].getBookFile(item->data(0, Qt::UserRole).toUInt(), &outbuff);
    if(fi.fileName().isEmpty())
        return;
    QString TempDir;
    if(QStandardPaths::standardLocations(QStandardPaths::TempLocation).count() > 0)
        TempDir = QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
    QDir dir(TempDir + QStringLiteral("/freeLib"));
    dir.mkpath(dir.path());
    QFile file(dir.path() + QStringLiteral("/") + fi.fileName());
    file.open(QFile::WriteOnly);
    file.write(outbuff.data());
    file.close();

    QString sExt = fi.suffix().toLower();
    if(options.applications.contains(sExt)){
        if(

        QProcess::startDetached(options.applications.value(sExt), QStringList() << file.fileName())

        )
            return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(file.fileName()));
}

void MainWindow::CheckBooks()
{
    QList<uint> book_list = getCheckedBooks(true);

    const QSignalBlocker blocker(ui->Books);
    Qt::CheckState cs = book_list.count() > 0 ?Qt::Unchecked :Qt::Checked;
    for(int i = 0; i<ui->Books->topLevelItemCount(); i++)
    {
        ui->Books->topLevelItem(i)->setCheckState(0, cs);
        CheckChild(ui->Books->topLevelItem(i));
    }
}

void MainWindow::CheckParent(QTreeWidgetItem *parent)
{
    bool checked = false;
    bool unchecked = false;
    bool partially = false;
    for(int i = 0; i<parent->childCount(); i++)
    {
        switch(parent->child(i)->checkState(0))
        {
        case Qt::Checked:
            checked = true;
            break;
        case Qt::Unchecked:
            unchecked = true;
            break;
        case Qt::PartiallyChecked:
            partially = true;
            break;
        }
    }
    if(partially || (checked && unchecked))
        parent->setCheckState(0, Qt::PartiallyChecked);
    else if(checked)
        parent->setCheckState(0, Qt::Checked);
    else
        parent->setCheckState(0, Qt::Unchecked);
    if(parent->parent())
        CheckParent(parent->parent());
}

void MainWindow::CheckChild(QTreeWidgetItem *parent)
{
    if(parent->childCount() > 0)
    {
        for(int i=0; i<parent->childCount(); i++)
        {
            parent->child(i)->setCheckState(0, parent->checkState(0));
            if(parent->child(i)->childCount() > 0)
                CheckChild(parent->child(i));
        }
    }
}

void MainWindow::onItemChanged(QTreeWidgetItem *item, int)
{
    const bool wasBlocked = ui->Books->blockSignals(true);

    CheckChild(item);
    QTreeWidgetItem* parent = item->parent();
    if(parent)
        CheckParent(parent);
    QList<uint> book_list = getCheckedBooks();
    ExportBookListBtn(book_list.count() != 0);

    ui->Books->blockSignals(wasBlocked);
}

void MainWindow::ExportBookListBtn(bool Enable)
{
    ui->btnExport->setEnabled(Enable);
    ui->btnOpenBook->setEnabled(false);
}

void MainWindow::StartSearch()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QApplication::processEvents();
    ui->Books->clear();
    ExportBookListBtn(false);
    QString sName = ui->s_name->text().trimmed();
    QString sAuthor = ui->s_author->text().trimmed();
    QString sSeria = ui->s_seria->text().trimmed();
    QDate dateFrom = ui->date_from->date();
    QDate dateTo = ui->date_to->date();
    int nMaxCount = ui->maxBooks->value();
    ushort idGenre = ui->s_genre->currentData().toUInt();
    int idLanguage = ui->findLanguage->currentData().toInt();

    SLib& lib = libs[idCurrentLib];
    listBooks_.clear();
    int nCount = 0;
    auto iBook = lib.books.constBegin();
    QList<uint> listPosFound;
    while(iBook != lib.books.constEnd()){
        bool bMatchAuthor = false;
        if(!sAuthor.isEmpty()){
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            QStringList listAuthor1 = sAuthor.split(' ', Qt::SkipEmptyParts);
#else
            QStringList listAuthor1 = sAuthor.split(' ');
#endif
            for(auto iAuthor = 0; iAuthor<iBook->listIdAuthors.size(); iAuthor++){
                listPosFound.clear();
                QStringList listAuthor2 = lib.authors[iBook->listIdAuthors[iAuthor]].getName().split(' ');
                bool bMatch = true;
                for(auto i1=0; i1<listAuthor1.size(); i1++){
                    int i2 = 0;
                    while(i2 < listAuthor2.size()){
                        if(QString::compare(listAuthor1.at(i1), listAuthor2.at(i2), Qt::CaseInsensitive) == 0){
                            if(!listPosFound.contains(i2)){
                                listPosFound << i2;
                                break;
                            }
                        }
                        ++i2;
                    }
                    if(i2 == listAuthor2.size()){
                        bMatch = false;
                        break;
                    }
                }
                if(bMatch){
                    bMatchAuthor = true;
                    break;
                }
            }
        }else
            bMatchAuthor = true;

        if((options.bShowDeleted || !iBook->bDeleted)&&
                iBook->date>= dateFrom && iBook->date <= dateTo &&
                bMatchAuthor &&
                (sName.isEmpty() || iBook->sName.contains(sName, Qt::CaseInsensitive)) &&
                (sSeria.isEmpty() || (iBook->idSerial>0 && lib.serials[iBook->idSerial].sName.contains(sSeria, Qt::CaseInsensitive))) &&
                (idLanguage == -1 ||(iBook->idLanguage == idLanguage)))
        {
            if(idGenre == 0){
                nCount++;
                listBooks_ << iBook.key();
            }else
            {
                for(auto id: iBook->listIdGenres) {
                   if(id == idGenre){
                       nCount++;
                       listBooks_ << iBook.key();
                       break;
                   }
                }
            }
        }
        ++iBook;
        if(nCount == nMaxCount)
            break;
    }
    ui->find_books->setText(QString::number(nCount));


    FillListBooks(listBooks_, QList<uint>(), 0);

    QApplication::restoreOverrideCursor();
}

void MainWindow::SelectLibrary()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QApplication::processEvents();

    QAction* action = qobject_cast<QAction*>(sender());
    SaveLibPosition();
    uint idOldLib = idCurrentLib;
    idCurrentLib = action->data().toUInt();
    if(idCurrentLib != idOldLib){
        auto &oldLib = libs[idOldLib];
        QFuture<void> future;
        if(oldLib.timeHttp + 24h < std::chrono::system_clock::now()){
            future = QtConcurrent::run([&oldLib]()
            {
                oldLib.bLoaded = false;
                oldLib.books.clear();
                oldLib.authors.clear();
                oldLib.serials.clear();
                oldLib.authorBooksLink.clear();
                oldLib.vLaguages.clear();
            });
        }

        auto settings = GetSettings();
        settings->setValue(QStringLiteral("LibID"), idCurrentLib);

        loadLibrary(idCurrentLib);
        fillLanguages();
        FillAuthors();
        FillSerials();
        FillGenres();
        switch(ui->tabWidget->currentIndex()){
        case TabAuthors:
            onSerachAuthorsChanded(ui->searchAuthor->text());
            break;
        case TabSeries:
            onSerachSeriesChanded(ui->searchSeries->text());
            break;
        case TabGenres:
            SelectGenre();
            break;
        }

        setWindowTitle(QStringLiteral("freeLib") + ((idCurrentLib == 0||libs[idCurrentLib].name.isEmpty() ?QStringLiteral("") :QStringLiteral(" — ")) + libs[idCurrentLib].name));
                           FillLibrariesMenu();

        future.waitForFinished();
    }

    QApplication::restoreOverrideCursor();
}

/*
    выбор текущего жанра
*/
void MainWindow::SelectGenre()
{
    ui->Books->clear();
    ExportBookListBtn(false);
    if(ui->GenreList->selectedItems().count() == 0)
        return;
    QTreeWidgetItem* cur_item = ui->GenreList->selectedItems()[0];
    ushort idGenre = cur_item->data(0, Qt::UserRole).toUInt();
    listBooks_.clear();
    SLib& lib = libs[idCurrentLib];
    auto iBook = lib.books.constBegin();
    while(iBook != lib.books.constEnd()){
        if((idCurrentLanguage_ == -1 || idCurrentLanguage_ == iBook->idLanguage)){
            for(auto iGenre: iBook->listIdGenres) {
                if(iGenre == idGenre){
                    listBooks_ << iBook.key();
                    break;
                }
            }
        }
        ++iBook;
    }
    idCurrentGenre_ = idGenre;
    FillListBooks(listBooks_, QList<uint>(), 0);
    auto settings = GetSettings();
    if(options.bStorePosition){
        settings->setValue(QStringLiteral("current_genre_id"), idCurrentGenre_);
    }
}

/*
    выбор Серии в списке Серий
*/
void MainWindow::SelectSeria()
{
    ui->Books->clear();
    ExportBookListBtn(false);
    if(ui->SeriaList->selectedItems().count() == 0)
        return;
    QListWidgetItem* cur_item = ui->SeriaList->selectedItems().at(0);
    uint idSerial = cur_item->data(Qt::UserRole).toUInt();
    listBooks_.clear();
    SLib& lib = libs[idCurrentLib];
    auto iBook = lib.books.constBegin();
    while(iBook != lib.books.constEnd()){
        if(iBook->idSerial == idSerial && (idCurrentLanguage_ == -1 || idCurrentLanguage_ == iBook->idLanguage)){
            listBooks_ << iBook.key();
        }
        ++iBook;
    }
    FillListBooks(listBooks_, QList<uint>(), 0);

    idCurrentSerial_= idSerial;
    if(options.bStorePosition){
        auto settings = GetSettings();
        settings->setValue(QStringLiteral("current_serial_id"), idSerial);
    }
}

/*
    выбор Автора в списке Авторов
*/
void MainWindow::SelectAuthor()
{
    ExportBookListBtn(false);
    if(ui->AuthorList->selectedItems().count() == 0)
        return;
    QListWidgetItem* cur_item =ui->AuthorList->selectedItems().at(0);
    idCurrentAuthor_ = cur_item->data(Qt::UserRole).toUInt();
    listBooks_.clear();
    listBooks_ = libs[idCurrentLib].authorBooksLink.values(idCurrentAuthor_);
    FillListBooks(listBooks_, QList<uint>(), idCurrentAuthor_);

    if(options.bStorePosition){
        auto settings = GetSettings();
        settings->setValue(QStringLiteral("current_author_id"), idCurrentAuthor_);
    }
}

void MainWindow::SelectBook()
{
    if(ui->Books->selectedItems().count() == 0)
    {
        ExportBookListBtn(false);
        ui->Review->setHtml(QStringLiteral(""));
        pCover->setBook(nullptr);
        return;
    }
    ExportBookListBtn(true);
    QTreeWidgetItem* item = ui->Books->selectedItems().at(0);
    if(item->type() != ITEM_TYPE_BOOK)
    {
        ui->btnOpenBook->setEnabled(false);
        ui->Review->setHtml(QStringLiteral(""));
        pCover->setBook(nullptr);
        return;
    }
    uint idBook = item->data(0,Qt::UserRole).toUInt();
    idCurrentBook_ = idBook;
    SLib& lib = libs[idCurrentLib];
    SBook &book = lib.books[idBook];
    ui->btnOpenBook->setEnabled(true);
    QDateTime book_date;
    QFileInfo fi = lib.getBookFile(idBook, nullptr, nullptr, &book_date);
    if(book.sAnnotation.isEmpty() && book.sImg.isEmpty())
        lib.loadAnnotation(idBook);
    if(fi.fileName().isEmpty())
    {
        QString file;
        QString sLibPath = lib.path;
        if(book.sArchive.trimmed().isEmpty() )
        {
            file = QStringLiteral("%1/%2.%3").arg(sLibPath, book.sFile, book.sFormat);
        }
        else
        {
            QString sArchive = book.sArchive;
            sArchive.replace(QStringLiteral(".inp"), QStringLiteral(".zip"));
            file = sLibPath + QStringLiteral("/") + sArchive;
        }
        file = file.replace('\\', '/');
    }

    QString seria;
    if(book.idSerial > 0){
        seria = QStringLiteral("<a href=seria_%3%1>%2</a>").arg(QString::number(book.idSerial),
                lib.serials[book.idSerial].sName, lib.serials[book.idSerial].sName.at(0).toUpper());
    }

    QString sAuthors;
    for(auto idAuthor: book.listIdAuthors)
    {
        QString sAuthor = lib.authors[idAuthor].getName();
        sAuthors += (sAuthors.isEmpty() ?QStringLiteral("") :QStringLiteral("; ")) + QStringLiteral("<a href='author_%3%1'>%2</a>")
                .arg(QString::number(idAuthor), sAuthor.replace(',', ' '), sAuthor.at(0));
    }
    QString sGenres;
    for(auto idGenre: book.listIdGenres)
    {
        QString sGenre = genres[idGenre].sName;
        sGenres += (sGenres.isEmpty() ?QStringLiteral("") :QStringLiteral("; ")) + QStringLiteral("<a href='genre_%3%1'>%2</a>")
                .arg(QString::number(idGenre), sGenre, sGenre.at(0));
    }
    QFile file_html(QStringLiteral(":/preview.html"));
    file_html.open(QIODevice::ReadOnly);
    QString content(file_html.readAll());
    QString sFilePath;
    qint64 size = 0;
    if(!fi.fileName().isEmpty())
    {
        QFileInfo arh;
        if(fi.absolutePath().startsWith(QDir::tempPath() + QStringLiteral("/freeLib/zip/"))){
            sFilePath = fi.path();
            sFilePath.chop(4);
            sFilePath.replace(QDir::tempPath() + QStringLiteral("/freeLib/zip/"), lib.path + QStringLiteral("/"));
            arh.setFile(sFilePath);
            while(!arh.exists())
            {
                arh.setFile(arh.absolutePath());
                if(arh.fileName().isEmpty())
                    break;
            }
        }
        else
        {
            arh = fi;
            while(!arh.exists())
            {
                arh.setFile(arh.absolutePath());
                if(arh.fileName().isEmpty())
                    break;
            }
            sFilePath = arh.filePath();
        }
        size = arh.size();
    }

    pCover->setBook(&book);

    QLocale locale;
    QColor colorBInfo = palette().color(QPalette::AlternateBase);
    content.replace(QStringLiteral("#annotation#"), book.sAnnotation).
            replace(QStringLiteral("#title#"), book.sName).
            replace(QStringLiteral("#author#"), sAuthors).
            replace(QStringLiteral("#genre#"), sGenres).
            replace(QStringLiteral("#series#"), seria).
            replace(QStringLiteral("#file_path#"), sFilePath).
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            replace(QStringLiteral("#file_size#"), size>0 ?locale.formattedDataSize(size, 1, QLocale::DataSizeTraditionalFormat) : QStringLiteral("")).
#else
            replace(QLatin1String("#file_size#"), size>0 ?sizeToString(size) : QLatin1String("")).
#endif
            replace(QStringLiteral("#file_data#"), book_date.toString(QStringLiteral("dd.MM.yyyy hh:mm:ss"))).
            replace(QStringLiteral("#file_name#"), fi.fileName()).
            replace(QStringLiteral("#infobcolor"), colorBInfo.name());
    ui->Review->setHtml(content);
}

/*
    заполнение списка языков в основном фильтре языков и фильтре языков на вкладке поиска
*/
void MainWindow::fillLanguages()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    SLib &currentLib = libs[idCurrentLib];

    ui->language->blockSignals(true);
    ui->findLanguage->blockSignals(true);
    ui->language->clear();
    ui->language->addItem(QStringLiteral("*"), -1);
    ui->language->setCurrentIndex(0);
    ui->findLanguage->clear();
    ui->findLanguage->addItem(QStringLiteral("*"), -1);
    ui->findLanguage->setCurrentIndex(0);

    auto settings = GetSettings();
    QString sCurrentLanguage = settings->value(QStringLiteral("BookLanguage"), QStringLiteral("*")).toString();
    for(int iLang = 0; iLang<currentLib.vLaguages.size(); iLang++){
        QString sLanguage = currentLib.vLaguages[iLang].toUpper();
        if(!sLanguage.isEmpty()){
            ui->language->addItem(sLanguage, iLang);
            ui->findLanguage->addItem(sLanguage, iLang);
            if(sLanguage == sCurrentLanguage){
                ui->language->setCurrentIndex(ui->language->count()-1);
                idCurrentLanguage_ = iLang;
            }
        }
    }
    ui->language->model()->sort(0);
    settings->setValue(QStringLiteral("BookLanguage"), ui->language->currentText());
    ui->language->blockSignals(false);
    ui->findLanguage->blockSignals(false);
    QApplication::restoreOverrideCursor();
}

void MainWindow::ManageLibrary()
{
    SaveLibPosition();
    LibrariesDlg al(this);
    al.exec();
    if(al.bLibChanged){
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        loadLibrary(idCurrentLib);

        UpdateTags();
        fillLanguages();
        switch(ui->tabWidget->currentIndex()){
        case TabAuthors:
            onSerachAuthorsChanded(ui->searchAuthor->text());
            break;
        case TabSeries:
            onSerachSeriesChanded(ui->searchSeries->text());
            break;
        }
        setWindowTitle(QStringLiteral("freeLib") + ((idCurrentLib==0||libs[idCurrentLib].name.isEmpty() ?QStringLiteral("") :QStringLiteral(" — ")) + libs[idCurrentLib].name));
        FillLibrariesMenu();
        FillGenres();
        QGuiApplication::restoreOverrideCursor();
    }
}

/*
    окно статистики библиотеки
*/
void MainWindow::onStatistics()
{
    StatisticsDialog dlg;
    dlg.exec();
}

void MainWindow::onAddBooks()
{
    SLib &lib = libs[idCurrentLib];

    QStringList listFiles = QFileDialog::getOpenFileNames(this, tr("Select books to add"), lib.path,
                                                          tr("Books (*.fb2 *.epub *.zip)"), nullptr, QFileDialog::ReadOnly);
    if(!listFiles.isEmpty()){
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        for(qsizetype i = 0 ; i < listFiles.size(); ++i) {
            QString sFile = listFiles.at(i);
            if(!sFile.startsWith(lib.path)){
                //перенос файлов книг в папку библиотеки
                QFileInfo fiSrc = QFileInfo(sFile);
                QString baseName = fiSrc.baseName(); // получаем имя файла без расширения
                QString extension = fiSrc.completeSuffix(); // получаем расширение файла
                QFileInfo fiDst = QFileInfo(lib.path + "/" + baseName + "." + extension);
                uint j = 1;
                while(fiDst.exists()){
                    QString sNewName = QStringLiteral("%1 (%2).%3").arg(baseName, QString::number(j), extension);
                    fiDst = QFileInfo(lib.path + "/" + sNewName);
                    j++;
                }
                QFile::copy(sFile, fiDst.absoluteFilePath());
                listFiles[i] = fiDst.absoluteFilePath();
            }
        }
        pThread_ = new QThread;
        pImportThread_ = new ImportThread();
        pImportThread_->init(idCurrentLib, lib, listFiles);
        pImportThread_->moveToThread(pThread_);
        connect(pThread_, &QThread::started, pImportThread_, &ImportThread::process);
        connect(pImportThread_, &ImportThread::End, pThread_, &QThread::quit);
        connect(pThread_, &QThread::finished, this, &MainWindow::addBooksFinished);
        pThread_->start();
    }
}

void MainWindow::addBooksFinished()
{
    pImportThread_->deleteLater();
    pThread_->deleteLater();
    loadLibrary(idCurrentLib);
    fillLanguages();
    FillAuthors();
    FillSerials();
    FillGenres();
    switch(ui->tabWidget->currentIndex()){
    case TabAuthors:
        onSerachAuthorsChanded(ui->searchAuthor->text());
        break;
    case TabSeries:
        onSerachSeriesChanded(ui->searchSeries->text());
        break;
    }
    QGuiApplication::restoreOverrideCursor();
}

void MainWindow::btnSearch()
{
    QToolButton *button = qobject_cast<QToolButton*>(sender());
    switch(ui->tabWidget->currentIndex()){
    case TabAuthors:
        ui->searchAuthor->setText(button->text());
        break;
    case TabSeries:
        ui->searchSeries->setText(button->text());
        break;
    }
}

void MainWindow::About()
{
    AboutDialog* dlg = new AboutDialog(this);
    dlg->exec();
    delete dlg;
}

void MainWindow::DoSearch()
{

}

void MainWindow::checkLetter(const QChar cLetter)
{
    QList<QToolButton*> allButtons = findChildren<QToolButton *>();
    bool find=false;
    for(QToolButton *tb: allButtons)
    {
        QString sButtonLetter = tb->text();
        if(!sButtonLetter.isEmpty() && sButtonLetter.at(0) == cLetter)
        {
            find = true;
            tb->setChecked(true);
            break;
        }
    }
    if(!find)
        btn_Hash->setChecked(true);
}

void MainWindow::onSerachAuthorsChanded(const QString &str)
{
    if(str.length() == 0)
    {
        ui->searchAuthor->setText(last_search_symbol);
        ui->searchAuthor->selectAll();
    }
    else
    {
        last_search_symbol = str.left(1);
        if((last_search_symbol == u"*" || last_search_symbol == u"#" ) && str.length()>1)
        {
            ui->searchAuthor->setText(str.right(str.length()-1));
        }
        checkLetter(last_search_symbol.at(0).toUpper());
        FillAuthors();
    }
    if(ui->AuthorList->count() > 0){
        if(ui->AuthorList->selectedItems().count() == 0)
            ui->AuthorList->setCurrentRow(0);
        auto item = ui->AuthorList->selectedItems()[0];
        ui->AuthorList->scrollToItem(item, QAbstractItemView::EnsureVisible);
    }
    ui->searchAuthor->setClearButtonEnabled(ui->searchAuthor->text().length() > 1);
}

void MainWindow::onSerachSeriesChanded(const QString &str)
{
    if(str.length() == 0)
    {
        ui->searchSeries->setText(last_search_symbol);
        ui->searchSeries->selectAll();
    }
    else
    {
        last_search_symbol=str.left(1);
        if((last_search_symbol == u"*" || last_search_symbol == u"#") && str.length()>1)
        {
            ui->searchSeries->setText(str.right(str.length()-1));
        }
        checkLetter(last_search_symbol.at(0).toUpper());
        FillSerials();
    }
    if(ui->SeriaList->count() > 0){
        if(ui->SeriaList->selectedItems().count() == 0)
            ui->SeriaList->setCurrentRow(0);
        auto item = ui->SeriaList->selectedItems()[0];
        ui->SeriaList->scrollToItem(item, QAbstractItemView::EnsureVisible);
    }
    ui->searchSeries->setClearButtonEnabled(ui->searchSeries->text().length()>1);
}

void MainWindow::HelpDlg()
{
    if(pHelpDlg == nullptr)
        pHelpDlg = new HelpDialog();
    pHelpDlg->show();
}

void MainWindow::ContextMenu(QPoint point)
{
    if(QObject::sender() == ui->Books && !ui->Books->itemAt(point))
        return;
    if(QObject::sender() == ui->AuthorList && !ui->AuthorList->itemAt(point))
        return;
    if(QObject::sender() == ui->SeriaList && !ui->SeriaList->itemAt(point))
        return;
    QMenu menu;
    currentListForTag_ = QObject::sender();
    QList<QTreeWidgetItem*> listItems;
    if(QObject::sender() == ui->Books)
    {
        QMenu *save = menu.addMenu(tr("Save as"));
        listItems  = checkedItemsBookList();
        if(listItems.isEmpty())
            listItems = ui->Books->selectedItems();
        for (QAction* i: ui->btnExport->menu()->actions())
        {
            QAction *action = new QAction(i->text(), save);
            action->setData(i->data().toInt());
            connect(action, &QAction::triggered, this, &MainWindow::ExportAction);
            save->addAction(action);
        }
        if(listItems.size() == 1){
            auto item = listItems.at(0);
            if(item->type() == ITEM_TYPE_BOOK){
                QMenu *rate = menu.addMenu(tr("Mark"));
                auto starSize = 16;//rate->fontInfo().pixelSize();
                QString sStyle = QStringLiteral("QWidget:hover {background: %1;}").arg(palette().color(QPalette::QPalette::Highlight).name());
                for(int i=0; i<=5; i++){
                    QWidgetAction *labelAction = new QWidgetAction(rate);
                    QLabel *label = new QLabel(rate);
                    label->setStyleSheet(sStyle);
                    label->setMargin(starSize/3);
                    QPixmap mypix (QIcon(QStringLiteral(":/icons/img/icons/stars/%1star.svg").arg(i)).pixmap(QSize(starSize*5, starSize)));
                    label->setPixmap(mypix);
                    labelAction->setDefaultWidget(label);
                    connect(labelAction, &QAction::triggered, this, [this, item, i](){onSetRating(item, i);});
                    rate->addAction(labelAction);
                }
            }
        }
    }
    if(menu.actions().count() > 0)
        menu.addSeparator();
    if(options.bUseTag){
        if(QObject::sender() == ui->Books){
            QMap<uint, uint> countTags;
            QList<uint> listTags;
            for(int iItem=0; iItem<listItems.size(); iItem++){
                auto &item = listItems.at(iItem);
                uint id = item->data(0, Qt::UserRole).toUInt();
                switch(item->type()){
                case ITEM_TYPE_BOOK:
                    listTags = libs[idCurrentLib].books[id].listIdTags;
                    break;
                case ITEM_TYPE_SERIA:
                    listTags = libs[idCurrentLib].serials[id].listIdTags;
                    break;
                case ITEM_TYPE_AUTHOR:
                    listTags = libs[idCurrentLib].authors[id].listIdTags;
                }
                for(int iTag=0; iTag<listTags.size(); iTag++){
                    uint idTag = listTags.at(iTag);
                    if(countTags.contains(idTag))
                            countTags[idTag] = 1;
                    else
                        countTags[idTag]++;
                }
            }
            const auto &actions = TagMenu.actions();
            for(int iAction=0; iAction<actions.size(); iAction++){
                uint idTag = actions.at(iAction)->data().toUInt();
                if(!countTags.contains(idTag))
                    actions.at(iAction)->setChecked(false);
                else
                    actions.at(iAction)->setChecked(true);
            }
        }

        if(QObject::sender() == ui->AuthorList){
            const auto &actions = TagMenu.actions();
            for(int iAction=0; iAction<actions.size(); iAction++){
                uint idTag = actions.at(iAction)->data().toUInt();
                uint idAuthor = ui->AuthorList->itemAt(point)->data(Qt::UserRole).toUInt();
                actions.at(iAction)->setChecked(libs[idCurrentLib].authors[idAuthor].listIdTags.contains(idTag));
            }
        }

        if(QObject::sender() == ui->SeriaList){
            const auto &actions = TagMenu.actions();
            for(int iAction=0; iAction<actions.size(); iAction++){
                uint idTag = actions.at(iAction)->data().toUInt();
                uint idSerial = ui->SeriaList->itemAt(point)->data(Qt::UserRole).toUInt();
                actions.at(iAction)->setChecked(libs[idCurrentLib].serials[idSerial].listIdTags.contains(idTag));
            }
        }

        menu.addActions(TagMenu.actions());
    }
    if(menu.actions().count() > 0)
        menu.exec(QCursor::pos());
}

void MainWindow::HeaderContextMenu(QPoint /*point*/)
{
    QMenu menu;
    QAction *action;

    action = new QAction(tr("Name"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(0));
    connect(action, &QAction::triggered,this, [action, this]{ui->Books->setColumnHidden(0, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Author"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(1));
    connect(action, &QAction::triggered,this, [action, this]{ui->Books->setColumnHidden(1, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Serial"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(2));
    connect(action, &QAction::triggered,this, [action, this]{ui->Books->setColumnHidden(2, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("No."), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(3));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(3, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Size"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(4));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(4, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Mark"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(5));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(5, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Import date"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(6));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(6, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Genre"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(7));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(7, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Language"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(8));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(8, !action->isChecked());});
    menu.addAction(action);

    action = new QAction(tr("Format"), this);
    action->setCheckable(true);
    action->setChecked(!ui->Books->isColumnHidden(9));
    connect(action, &QAction::triggered, this, [action, this]{ui->Books->setColumnHidden(9, !action->isChecked());});
    menu.addAction(action);

    menu.exec(QCursor::pos());
}

void MainWindow::MoveToSeria(uint id, const QString &FirstLetter)
{
    ui->searchSeries->setText(FirstLetter);
    ui->tabWidget->setCurrentIndex(TabSeries);
    ui->SeriaList->clearSelection();
    for (int i=0; i<ui->SeriaList->count(); i++)
    {
        if(ui->SeriaList->item(i)->data(Qt::UserRole).toUInt() == id)
        {
            ui->SeriaList->item(i)->setSelected(true);
            ui->SeriaList->scrollToItem(ui->SeriaList->item(i));
            SelectSeria();
            return;
        }
    }
}

void MainWindow::MoveToGenre(uint id)
{
    ui->tabWidget->setCurrentIndex(TabGenres);
    ui->GenreList->clearSelection();
    for (int i=0; i<ui->GenreList->topLevelItemCount(); i++)
    {
        for (int j=0; j<ui->GenreList->topLevelItem(i)->childCount(); j++)
        {
            if(ui->GenreList->topLevelItem(i)->child(j)->data(0, Qt::UserRole).toUInt() == id)
            {
                ui->GenreList->topLevelItem(i)->child(j)->setSelected(true);
                ui->GenreList->scrollToItem(ui->GenreList->topLevelItem(i)->child(j));
                SelectGenre();
                return;
            }
        }
    }
}

void MainWindow::MoveToAuthor(uint id, const QString &FirstLetter)
{
    ui->searchAuthor->setText(FirstLetter);
    ui->tabWidget->setCurrentIndex(TabAuthors);
    ui->AuthorList->clearSelection();
    for (int i=0; i<ui->AuthorList->count(); i++)
    {
        if(ui->AuthorList->item(i)->data(Qt::UserRole).toUInt() == id)
        {
            ui->AuthorList->item(i)->setSelected(true);
            ui->AuthorList->scrollToItem(ui->AuthorList->item(i));
            SelectAuthor();
            break;
        }
    }
}

void MainWindow::FillLibrariesMenu()
{
    if(!QSqlDatabase::database(QStringLiteral("libdb"), false).isOpen())
        return;
    QMenu *lib_menu = new QMenu(this);
    auto i = libs.constBegin();
    while(i != libs.constEnd()){
        uint idLib = i.key();
        if(idLib>0){
            QAction *action = new QAction(i->name, this);
            action->setData(idLib);
            action->setCheckable(true);
            lib_menu->insertAction(nullptr,action);
            connect(action, &QAction::triggered, this, &MainWindow::SelectLibrary);
            action->setChecked(idLib == idCurrentLib);
        }
        ++i;
    }
    ui->actionLibraries->setMenu(lib_menu);
    ui->actionLibraries->setEnabled(lib_menu->actions().count()>0);
}

void MainWindow::FillAuthors()
{
    if(idCurrentLib == 0)
        return;
    qint64 timeStart;
    if(bVerbose)
        timeStart = QDateTime::currentMSecsSinceEpoch();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    const bool wasBlocked = ui->AuthorList->blockSignals(true);
    QListWidgetItem *item;
    ui->AuthorList->clear();
    SLib &currentLib = libs[idCurrentLib];
    QListWidgetItem *selectedItem = nullptr;
    QString sSearch = ui->searchAuthor->text();
    auto iAuthor = currentLib.authors.constBegin();
    static const QRegularExpression re(QStringLiteral("[A-Za-zа-яА-ЯЁё]"));

    while(iAuthor != currentLib.authors.constEnd()){
        if(sSearch == u"*" || (sSearch == u"#" &&
          !iAuthor->getName().left(1).contains(re)) || iAuthor->getName().startsWith(sSearch, Qt::CaseInsensitive))
        {
            QList<uint> booksId = currentLib.authorBooksLink.values(iAuthor.key());
            int count =0;
            for( uint idBook: booksId) {
                SBook &book = currentLib.books[idBook];
                if(IsBookInList(book))
                    count++;
            }
            if(count>0){
                item = new QListWidgetItem(QStringLiteral("%1 (%2)").arg(iAuthor->getName()).arg(count));
                item->setData(Qt::UserRole, iAuthor.key());
                if(options.bUseTag)
                    item->setIcon(getTagIcon(iAuthor->listIdTags));
                ui->AuthorList->addItem(item);
                if(idCurrentAuthor_ == iAuthor.key()){
                    item->setSelected(true);
                    selectedItem = item;
                }
            }
        }
        ++iAuthor;
    }
    if(selectedItem != nullptr)
        ui->AuthorList->scrollToItem(selectedItem);

    ui->AuthorList->blockSignals(wasBlocked);
    if(bVerbose){
        qint64 timeEnd = QDateTime::currentMSecsSinceEpoch();
        qDebug()<< "FillAuthors " << timeEnd - timeStart << "msec";
    }
    QApplication::restoreOverrideCursor();
}

void MainWindow::FillSerials()
{
    if(idCurrentLib == 0)
        return;
    qint64 timeStart;
    if(bVerbose)
        timeStart = QDateTime::currentMSecsSinceEpoch();
    const bool wasBlocked = ui->SeriaList->blockSignals(true);
    ui->SeriaList->clear();
    QString sSearch = ui->searchSeries->text();
    static const QRegularExpression re(QStringLiteral("[A-Za-zа-яА-ЯЁё]"));

    QMap<uint,uint> mCounts;
    const SLib& lib = libs[idCurrentLib];

    auto iBook = lib.books.constBegin();
    while(iBook != lib.books.constEnd()){
        if(iBook->idSerial != 0 &&
           IsBookInList(*iBook) && (sSearch == u"*" || (sSearch == u"#" &&
           !lib.serials[iBook->idSerial].sName.left(1).contains(re)) ||
           lib.serials[iBook->idSerial].sName.startsWith(sSearch, Qt::CaseInsensitive)))
        {
            if(mCounts.contains(iBook->idSerial))
                mCounts[iBook->idSerial]++;
            else
                mCounts[iBook->idSerial] = 1;
        }
        ++iBook;
    }

    QListWidgetItem *item;
    auto iSerial = mCounts.constBegin();
    while(iSerial != mCounts.constEnd()){
        item = new QListWidgetItem(QStringLiteral("%1 (%2)").arg(lib.serials[iSerial.key()].sName).arg(iSerial.value()));
        item->setData(Qt::UserRole, iSerial.key());
        if(options.bUseTag)
            item->setIcon(getTagIcon(lib.serials[iSerial.key()].listIdTags));
        ui->SeriaList->addItem(item);
        if(iSerial.key() == idCurrentSerial_)
        {
            item->setSelected(true);
            ui->SeriaList->scrollToItem(item);
         }
        ++iSerial;
    }

    ui->SeriaList->blockSignals(wasBlocked);
    if(bVerbose){
        qint64 timeEnd = QDateTime::currentMSecsSinceEpoch();
        qDebug()<< "FillSerials " << timeEnd - timeStart << "msec";
    }
}

void MainWindow::FillGenres()
{
    if(idCurrentLib == 0)
        return;
    qint64 timeStart;
    if(bVerbose)
        timeStart = QDateTime::currentMSecsSinceEpoch();
    const bool wasBlocked = ui->GenreList->blockSignals(true);
    ui->GenreList->clear();
    ui->s_genre->clear();
    ui->s_genre->addItem(QStringLiteral("*"), 0);
    QFont fontBold(ui->AuthorList->font());
    fontBold.setBold(true);

    QMap<ushort, uint> mCounts;
    SLib& lib = libs[idCurrentLib];

    auto iBook = lib.books.constBegin();
    while(iBook != lib.books.constEnd()){
        if(IsBookInList(*iBook))
        {
            for(auto iGenre: iBook->listIdGenres) {
                if(mCounts.contains(iGenre))
                    mCounts[iGenre]++;
                else
                    mCounts[iGenre] = 1;
            }
        }
        ++iBook;
    }

    QMap<ushort, QTreeWidgetItem*> mTopGenresItem;
    auto iGenre = mCounts.constBegin();
    while(iGenre != mCounts.constEnd()){
        auto idGenre = iGenre.key();
        auto idParrentGenre = genres[idGenre].idParrentGenre;
        QTreeWidgetItem *pTopItem;

        if(!mTopGenresItem.contains(idParrentGenre)){
            const QString &sTopName = genres[idParrentGenre].sName;
            pTopItem = new QTreeWidgetItem(ui->GenreList);
            pTopItem->setFont(0, fontBold);
            pTopItem->setText(0, sTopName);
            pTopItem->setData(0, Qt::UserRole, idParrentGenre);
            pTopItem->setExpanded(false);
            mTopGenresItem[idParrentGenre] = pTopItem;
            ui->s_genre->addItem(sTopName, idParrentGenre);
        }else
            pTopItem = mTopGenresItem[idParrentGenre];

        QTreeWidgetItem *item = new QTreeWidgetItem(pTopItem);
        const QString &sName = genres[idGenre].sName;
        item->setText(0, QStringLiteral("%1 (%2)").arg(sName).arg(mCounts[idGenre]));
        item->setData(0, Qt::UserRole, idGenre);
        ui->s_genre->addItem(QStringLiteral("   ") + sName, idGenre);
        if(idGenre == idCurrentGenre_)
        {
            item->setSelected(true);
            ui->GenreList->scrollToItem(item);
        }

        ++iGenre;
    }

    ui->s_genre->model()->sort(0);

    ui->GenreList->blockSignals(wasBlocked);
    if(bVerbose){
        qint64 timeEnd = QDateTime::currentMSecsSinceEpoch();
        qDebug()<< "FillGenres " << timeEnd - timeStart << "msec";
    }
}

void MainWindow::FillListBooks()
{
    switch(ui->tabWidget->currentIndex()){
        case 0:
            SelectAuthor();
        break;

        case 1:
            SelectSeria();
        break;

        case 2:
            SelectGenre();
        break;

    }
}

void MainWindow::FillListBooks(const QList<uint> &listBook, const QList<uint> &listCheckedBooks, uint idCurrentAuthor)
{
    if(idCurrentLib == 0)
        return;
    qint64 timeStart;
    if(bVerbose)
        timeStart = QDateTime::currentMSecsSinceEpoch();
    QFont bold_font(ui->Books->font());
    bold_font.setBold(true);
    TreeBookItem* ScrollItem = nullptr;
    QLocale locale;

    TreeBookItem* item_seria = nullptr;
    TreeBookItem* item_book;
    TreeBookItem* item_author;
    QMap<uint, TreeBookItem*> authors;

    QMultiMap<uint, TreeBookItem*> mSerias;

    const bool wasBlocked = ui->Books->blockSignals(true);
    ui->Books->clear();
    SLib &lib = libs[idCurrentLib];

    for( uint idBook: listBook) {
        SBook &book = lib.books[idBook];
        if(IsBookInList(book))
        {
            uint idAuthor;
            QString sAuthor;
            bool bBookChecked = listCheckedBooks.contains(idBook);
            if(idCurrentAuthor > 0)
                idAuthor = idCurrentAuthor;
            else
                idAuthor = book.idFirstAuthor;
            sAuthor = lib.authors[idAuthor].getName();

            uint idSerial = book.idSerial;
            if(bTreeView_){
                if(!authors.contains(idAuthor)){
                    item_author = new TreeBookItem(ui->Books, ITEM_TYPE_AUTHOR);
                    item_author->setText(0, sAuthor);
                    item_author->setExpanded(true);
                    item_author->setFont(0, bold_font);
                    item_author->setCheckState(0, bBookChecked ?Qt::Checked  :Qt::Unchecked);
                    item_author->setData(0, Qt::UserRole, idAuthor);
                    item_author->setData(3, Qt::UserRole, -1);

                    if(options.bUseTag){
                        item_author->setIcon(0, getTagIcon(lib.authors[idAuthor].listIdTags));
                    }
                    authors[idAuthor] = item_author;
                }else{
                    item_author = authors[idAuthor];
                    auto checkedState = item_author->checkState(0);
                    if((checkedState == Qt::Checked && !bBookChecked) || (checkedState == Qt::Unchecked && bBookChecked))
                        item_author->setCheckState(0, Qt::PartiallyChecked);
                }

                if(idSerial > 0){
                    auto iSerial = mSerias.constFind(idSerial);
                    while(iSerial != mSerias.constEnd()){
                        item_seria = iSerial.value();
                        if(item_seria->parent()->data(0, Qt::UserRole) == idAuthor){
                            auto checkedState = item_seria->checkState(0);
                            if((checkedState == Qt::Checked && !bBookChecked) || (checkedState == Qt::Unchecked && bBookChecked))
                                item_seria->setCheckState(0, Qt::PartiallyChecked);
                            break;
                        }
                        ++iSerial;
                    }
                    if(iSerial == mSerias.constEnd()){
                        item_seria = new TreeBookItem(authors[idAuthor], ITEM_TYPE_SERIA);
                        item_seria->setText(0, lib.serials[idSerial].sName);
                        item_author->addChild(item_seria);
                        item_seria->setExpanded(true);
                        item_seria->setFont(0, bold_font);
                        item_seria->setCheckState(0, bBookChecked ?Qt::Checked  :Qt::Unchecked);
                        item_seria->setData(0, Qt::UserRole, idSerial);
                        item_seria->setData(3, Qt::UserRole, -1);
                        if(options.bUseTag)
                            item_seria->setIcon(0, getTagIcon(lib.serials[idSerial].listIdTags));

                        mSerias.insert(idSerial, item_seria);
                    }
                    item_book = new TreeBookItem(item_seria, ITEM_TYPE_BOOK);
                }else
                    item_book = new TreeBookItem(item_author, ITEM_TYPE_BOOK);
            }else
                item_book = new TreeBookItem(ui->Books, ITEM_TYPE_BOOK);

            item_book->setCheckState(0, bBookChecked ?Qt::Checked  :Qt::Unchecked);
            item_book->setData(0, Qt::UserRole, idBook);
            if(options.bUseTag){
                item_book->setIcon(0, getTagIcon(book.listIdTags));
            }

            item_book->setText(0, book.sName);

            item_book->setText(1, sAuthor);

            if(idSerial > 0)
                item_book->setText(2, lib.serials[idSerial].sName);

            if(book.numInSerial>0){
                item_book->setText(3, QString::number(book.numInSerial));
                item_book->setTextAlignment(3, Qt::AlignRight|Qt::AlignVCenter);
            }

            if(book.nSize>0){
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
                item_book->setText(4, locale.formattedDataSize(book.nSize, 1, QLocale::DataSizeTraditionalFormat));
#else
                item_book->setText(4, sizeToString(book.nSize));

#endif
                item_book->setTextAlignment(4, Qt::AlignRight|Qt::AlignVCenter);
            }

            item_book->setData(5, Qt::UserRole, book.nStars);

            item_book->setText(6, book.date.toString(QStringLiteral("dd.MM.yyyy")));
            item_book->setTextAlignment(6, Qt::AlignCenter);

            if(!book.listIdGenres.empty()){
                item_book->setText(7, genres[book.listIdGenres.first()].sName);
                item_book->setTextAlignment(7, Qt::AlignLeft|Qt::AlignVCenter);
            }

            item_book->setText(8, lib.vLaguages[book.idLanguage]);
            item_book->setTextAlignment(8, Qt::AlignCenter);

            item_book->setText(9, book.sFormat);
            item_book->setTextAlignment(9, Qt::AlignCenter);

            if(book.bDeleted)
            {
                QBrush brush(QColor::fromRgb(96, 96, 96));
                for (int i = 0; i != 7; ++i)
                     item_book->setForeground(i, brush);
            }

            if(idBook == idCurrentBook_)
                ScrollItem = item_book;
        }
    }
    if(ScrollItem)
    {
        ScrollItem->setSelected(true);
        ui->Books->scrollToItem(ScrollItem);
    }
    SelectBook();

    ui->Books->blockSignals(wasBlocked);
    if(bVerbose){
        qint64 timeEnd = QDateTime::currentMSecsSinceEpoch();
        qDebug()<< "FillListBooks " << timeEnd - timeStart << "msec";
    }
}

bool MainWindow::IsBookInList(const SBook &book)
{
    uint idTagFilter = ui->TagFilter->itemData(ui->TagFilter->currentIndex()).toUInt();
    uint idSerial = book.idSerial;

    return (idCurrentLanguage_ == -1 || idCurrentLanguage_ == book.idLanguage) &&
            (options.bShowDeleted || !book.bDeleted) &&
            (!options.bUseTag || idTagFilter == 0 || book.listIdTags.contains(idTagFilter) ||
            (idSerial>0 && libs[idCurrentLib].serials[idSerial].listIdTags.contains(idTagFilter)) ||
            (libs[idCurrentLib].authors[book.idFirstAuthor].listIdTags.contains(idTagFilter)));
}

void MainWindow::UpdateExportMenu()
{
    QMenu* menu = ui->btnExport->menu();
    if(menu)
    {
        ui->btnExport->menu()->clear();
    }
    else
    {
        menu = new QMenu(this);
        ui->btnExport->setMenu(menu);
    }
    ui->btnExport->setDefaultAction(nullptr);
    int count = options.vExportOptions.size();
    for(int i = 0; i<count; i++)
    {
        const ExportOptions &exportOptions = options.vExportOptions.at(i);
        QAction *action = new QAction(exportOptions.sName, this);
        action->setData(i);
        menu->addAction(action);
        if(exportOptions.bDefault)
        {
            ui->btnExport->setDefaultAction(action);
        }
    }
    if(count == 0)
    {
       QAction *action = new QAction(tr("Send to ..."), this);
       action->setData(-1);
       menu->addAction(action);
       ui->btnExport->setDefaultAction(action);
    }
    if(menu->actions().count() == 0)
    {
        return;
    }
    if(!ui->btnExport->defaultAction())
    {
        ui->btnExport->setDefaultAction(menu->actions().constFirst());
    }
    for(QAction *action: menu->actions())
    {
        connect(action, &QAction::triggered, this, &MainWindow::ExportAction);
    }
    QFont font(ui->btnExport->defaultAction()->font());
    font.setBold(true);
    ui->btnExport->defaultAction()->setFont(font);
    bool darkTheme = palette().color(QPalette::Window).lightness() < 127;
    QString sIconsPath = QStringLiteral(":/img/icons/") + (darkTheme ?QStringLiteral("dark/") :QStringLiteral("light/"));
    ui->btnExport->setIcon(QIcon::fromTheme(QStringLiteral("document-send"), QIcon(sIconsPath + QStringLiteral("send.svg"))));
    ui->btnExport->setEnabled(ui->Books->selectedItems().count() > 0);
}

void MainWindow::ExportAction()
{
    int id = qobject_cast<QAction*>(sender())->data().toInt();
    if(options.vExportOptions.at(id).sSendTo == QStringLiteral("device"))
       SendToDevice(options.vExportOptions.at(id));
   else
       SendMail(options.vExportOptions.at(id));
}

void MainWindow::onLanguageFilterChanged(int index)
{
    QString sLanguage = ui->language->itemText(index);
    auto settings = GetSettings();
    settings->setValue(QStringLiteral("BookLanguage"), sLanguage);
    idCurrentLanguage_ = ui->language->itemData(index).toInt();

    FillSerials();
    FillAuthors();
    FillGenres();
    FillListBooks();
}

void MainWindow::onChangeAlpabet(const QString &sAlphabetName)
{
    if(ui->abc->layout())
    {
        while(!(qobject_cast<QBoxLayout*>(ui->abc->layout())->itemAt(0))->isEmpty())
        {
            delete dynamic_cast<QBoxLayout*>(ui->abc->layout()->itemAt(0))->itemAt(0)->widget();
        }
        if(!qobject_cast<QBoxLayout*>(ui->abc->layout())->isEmpty())
        {
            while(!(dynamic_cast<QBoxLayout*>(ui->abc->layout()->itemAt(1)))->isEmpty())
            {
                delete dynamic_cast<QBoxLayout*>(ui->abc->layout()->itemAt(1))->itemAt(0)->widget();
            }
        }
        while(!ui->abc->layout()->isEmpty())
        {
            delete ui->abc->layout()->itemAt(0);
        }
        delete ui->abc->layout();
    }
    FillAlphabet(sAlphabetName);
    FirstButton->click();
}

void MainWindow::onTreeView()
{
    ui->btnTreeView->setChecked(true);
    ui->btnListView->setChecked(false);
    if(!bTreeView_){
        bTreeView_ = true;
        QList<uint> listCheckedBooks = getCheckedBooks(true);
        FillListBooks(listBooks_, listCheckedBooks, ui->tabWidget->currentIndex()==0 ? idCurrentAuthor_ :0);
        auto settings = GetSettings();
        settings->setValue(QStringLiteral("TreeView"), true);
        aHeadersList_ = ui->Books->header()->saveState();
        ui->Books->header()->restoreState(aHeadersTree_);
        ui->Books->header()->setSortIndicatorShown(true);
    }
}

void MainWindow::onListView()
{
    ui->btnTreeView->setChecked(false);
    ui->btnListView->setChecked(true);
    if(bTreeView_){
        bTreeView_ = false;
        QList<uint> listCheckedBooks = getCheckedBooks(true);
        FillListBooks(listBooks_, listCheckedBooks, ui->tabWidget->currentIndex()==0 ? idCurrentAuthor_ :0);
        auto settings = GetSettings();
        settings->setValue(QStringLiteral("TreeView"), false);
        aHeadersTree_ = ui->Books->header()->saveState();
        ui->Books->header()->restoreState(aHeadersList_);
        ui->Books->header()->setSortIndicatorShown(true);
    }
}

void MainWindow::onSetRating(QTreeWidgetItem *item, uchar nRating)
{
    auto dbase = QSqlDatabase::database(QStringLiteral("libdb"), false);
    if(!dbase.isOpen())
        return;
    uint idBook = item->data(0, Qt::UserRole).toUInt();
    QSqlQuery query(dbase);
    query.prepare(QStringLiteral("UPDATE book SET star=:nRating WHERE id=:idBook;"));
    query.bindValue(QStringLiteral(":nRating"), nRating);
    query.bindValue(QStringLiteral(":idBook"), idBook);
    if(!query.exec())
        MyDBG << query.lastError().text();
    else{
        item->setData(5, Qt::UserRole, nRating);
        libs[idCurrentLib].books[idBook].nStars = nRating;
    }
}

void MainWindow::onTabWidgetChanged(int index)
{
    switch(index){
    case TabAuthors: //Авторы
    {
        QString sFilter = ui->searchAuthor->text();
        if(sFilter.isEmpty()){
            ui->searchAuthor->setText(FirstButton->text());
        }else {
            checkLetter(sFilter.at(0).toUpper());
        }
        ui->frame_3->setEnabled(true);
        ui->language->setEnabled(true);
        SelectAuthor();
    }
        break;
    case TabSeries: //Серии
    {
        QString sFilter = ui->searchSeries->text();
        if(sFilter.isEmpty()){
            ui->searchSeries->setText(FirstButton->text());
        }else {
            checkLetter(sFilter.at(0).toUpper());
        }
        ui->frame_3->setEnabled(true);
        ui->language->setEnabled(true);
        SelectSeria();
    }
        break;
    case TabGenres: //Жанры
        ui->frame_3->setEnabled(false);
        ui->language->setEnabled(true);
        SelectGenre();
        break;
    case TabSearch: //Поиск
        ui->frame_3->setEnabled(false);
        ui->language->setEnabled(false);
        ui->Books->clear();
        ui->find_books->setText(QStringLiteral("0"));
        ExportBookListBtn(false);
        break;
    }
}

void MainWindow::ChangingTrayIcon(int index, int color)
{
    if(bTray)
        index = 2;
    if(index == 0)
    {
        if(pTrayIcon_)
        {
            pTrayIcon_->hide();
            pTrayIcon_->deleteLater();
            pTrayMenu_->deleteLater();
        }
        pTrayIcon_ = nullptr;
        pTrayMenu_ = nullptr;
        pHideAction_ = nullptr;
        pShowAction_ = nullptr;
    }
    else
    {
        if(!pTrayIcon_)
        {
            pTrayMenu_ = new QMenu;
            pHideAction_ = new QAction(tr("Minimize"), pTrayMenu_);
            pTrayMenu_->addAction(pHideAction_);
            pShowAction_ = new QAction(tr("Open freeLib"), pTrayMenu_);
            pTrayMenu_->addAction(pShowAction_);
            QAction *quitAction = new QAction(tr("Quit"), pTrayMenu_);
            pTrayMenu_->addAction(quitAction);
            pTrayIcon_ = new QSystemTrayIcon(pTrayMenu_);  //инициализируем объект
            pTrayIcon_->setContextMenu(pTrayMenu_);

            if(isVisible())
                pShowAction_->setVisible(false);
            else
                pHideAction_->setVisible(false);

            connect(pTrayIcon_, &QSystemTrayIcon::activated, this, &MainWindow::TrayMenuAction);
            connect(pHideAction_, &QAction::triggered, this, &MainWindow::hide);
            connect(pShowAction_, &QAction::triggered, this, &MainWindow::show);
            connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
        }
        QIcon icon(QStringLiteral(":/img/tray%1.png").arg(color));
        pTrayIcon_->setIcon(icon); //устанавливаем иконку
        pTrayIcon_->show();
    }
}

void MainWindow::TrayMenuAction(QSystemTrayIcon::ActivationReason reson)
{
    if(reson != QSystemTrayIcon::Trigger && reson != QSystemTrayIcon::Unknown)
        return;
#ifdef Q_OS_WIN
    if(this->isVisible())
    {
        this->setWindowState(this->windowState()|Qt::WindowMinimized);
        if(options.nIconTray !=0 )
            this->hide();
    }
    else
    {
        this->show();
        this->setWindowState(this->windowState() & ~Qt::WindowMinimized);
        this->raise();
        this->activateWindow();
        this->setFocus(Qt::ActiveWindowFocusReason);
    }
#else
    #ifdef Q_OS_OSX
        if(reson==QSystemTrayIcon::Unknown)
            return;
        if(this->isActiveWindow() && this->isVisible())
        {
            this->setWindowState(this->windowState()|Qt::WindowMinimized);
            if(options.nIconTray!=0)
                this->hide();
        }
        else
        {
            this->show();
            this->setWindowState(this->windowState() & ~Qt::WindowMinimized);
            this->activateWindow();
            this->raise();
            this->setFocus(Qt::ActiveWindowFocusReason);
        }
    #else
        if(this->isActiveWindow() && this->isVisible())
        {
            this->setWindowState(this->windowState()|Qt::WindowMinimized);
            if(options.nIconTray != 0)
                this->hide();
        }
        else
        {
            this->show();
            this->setWindowState(this->windowState() & ~Qt::WindowMinimized);
            this->raise();
            this->activateWindow();
            this->setFocus(Qt::ActiveWindowFocusReason);
        }
    #endif
#endif
}

void MainWindow::MinimizeWindow()
{
    this->setWindowState(this->windowState() | Qt::WindowMinimized);
}

void MainWindow::hide()
{
    if(pHideAction_){
        pHideAction_->setVisible(false);
        pShowAction_->setVisible(true);
    }
    QMainWindow::hide();
}

void MainWindow::show()
{
    if(pHideAction_){
        pHideAction_->setVisible(true);
        pShowAction_->setVisible(false);
    }
    QMainWindow::show();
}

void MainWindow::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::WindowStateChange)
    {
        if(isMinimized())
        {
            ChangingTrayIcon(options.nIconTray, options.nTrayColor);
            TrayMenuAction(QSystemTrayIcon::Unknown);
            event->ignore();
        }
    }
}