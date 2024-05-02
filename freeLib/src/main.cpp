#define QT_USE_QSTRINGBUILDER
#include <iostream>
#include <QApplication>
#include <QNetworkProxy>
#include <QSplashScreen>
#include <QPainter>
#include <QStringBuilder>
#include <QDir>
#include <QSqlError>
#include <QThread>
#include <QString>

#include "mainwindow.h"
#include "aboutdialog.h"
#include "utilites.h"
#include "config-freelib.h"
#include "opds_server.h"
#include "importthread.h"

uint idCurrentLib;
bool bTray;
bool bVerbose;
Options options;

void UpdateLibs()
{
    if(!QSqlDatabase::database(QStringLiteral("libdb"), false).isOpen())
        idCurrentLib = 0;
    else{
        auto settings = GetSettings();
        idCurrentLib = settings->value(QStringLiteral("LibID"), 0).toInt();
        QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
        query.exec(QStringLiteral("SELECT id,name,path,inpx,version,firstauthor,woDeleted FROM lib ORDER BY name"));
        //                                0  1    2    3    4       5           6
        libs.clear();
        while(query.next())
        {
            int idLib = query.value(0).toUInt();
            libs[idLib].name = query.value(1).toString().trimmed();
            libs[idLib].path = query.value(2).toString().trimmed();
            libs[idLib].sInpx = query.value(3).toString().trimmed();
            libs[idLib].sVersion = query.value(4).toString().trimmed();
            libs[idLib].bFirstAuthor = query.value(5).toBool();
            libs[idLib].bWoDeleted = query.value(6).toBool();
        }
        if(libs.empty())
            idCurrentLib = 0;
        else{
            if(idCurrentLib == 0)
                idCurrentLib = libs.constBegin().key();
            if(!libs.contains(idCurrentLib))
                idCurrentLib = 0;
        }
    }
}

QString parseOption(int argc, char* argv[], const char* option)
{
    QString sRet;
    if(argc >= 2){
        for(int j=0; j+1<argc; j++){
            if(!strcmp(argv[j], option)){
                sRet = QString::fromUtf8(argv[j+1]);
                break;
            }
        }
    }
    return sRet;
}

void cmdhelp(){

std::cout  << "freelib " << FREELIB_VERSION << "\n\nfreelib [Option [Parameters]\n"
             "Options:\n"
             "-t,\t--tray\t\tMinimize to tray on start\n"
             "-s,\t--server\tStart server\n"
             "-v,\t--version\tShow version and exit\n"
             "\t--verbose\tVerbose mode\n"
             "\t--lib-ls\tShow libraries\n"
             "\t--lib-db [path]\tSet database path\n"
             "\t--lib-in [id]\tLibrary information\n"
             "\t--lib-sp\tSet paths for a library\n"
                    "\t\t-id [id]\n"
                    "\t\t-inpx [inpx path]\n"
                    "\t\t-path [library directory]\n"
             "\t--lib-ad\tAdd library\n"
                    "\t\t-name [name]\n"
                    "\t\t-inpx [inpx path]\n"
                    "\t\t-path [library directory]\n"
             "\t--lib-dl [id]\tDelete library\n"
             "\t--lib-up [id]\tUpdate library\n"
			 ;
}

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(resource);
    bool bServer = false;
    bTray = false;
    bVerbose = false;
    QCoreApplication *a;
    QString cmdparam;
    
    // Check if any cmd parameter has been passed
    if (argc > 1) {

        for(int i=1; i<argc; i++){

            cmdparam = argv[i];

            if (cmdparam == u"--help" || cmdparam == u"-h"){
                cmdhelp();
                return 0;
            }

            if (cmdparam == u"--verbose")
                bVerbose = true;

            if (cmdparam == u"--server" || cmdparam == u"-s"){
                bServer = true;
            }else

                if (cmdparam == u"--tray" || cmdparam == u"-t"){
                    bTray = true;
                }else

                    if (cmdparam == u"--version" || cmdparam == u"-v"){
                        std::cout << "freelib " << FREELIB_VERSION << "\n";
                        return 0;
                    }

            // Edit libraries
                    if (cmdparam.contains(QStringLiteral("--lib"))){
                a = new QCoreApplication(argc, argv);
                a->setOrganizationName(QStringLiteral("freeLib"));
                a->setApplicationName(QStringLiteral("freeLib"));

                auto settings = GetSettings();
                options.Load(settings);


                openDB(QStringLiteral("libdb"));
                UpdateLibs();


                setLocale(options.sUiLanguageName);

                uint nId = 0;

                // List libraries
                if(cmdparam == u"--lib-ls"){
                    std::cout << "id\tlibrary\n"
                                 "----------------------------------------------------------\n";
                    auto iLib = libs.constBegin();
                    while(iLib != libs.constEnd()){
                        std::cout << iLib.key() << "\t" << iLib->name.toStdString() << "\n";
                        ++iLib;
                    }
                }

                // Set database path
                if(cmdparam == u"--lib-db"){
                    QString sDbpath = parseOption(argc-(i), &argv[i], "--lib-db");
                    sDbpath = QFileInfo{sDbpath}.absoluteFilePath();
                    if(!sDbpath.isEmpty()){

                        if (QFile::exists(sDbpath)) {
                            options.sDatabasePath = sDbpath;
                            settings->setValue(QStringLiteral("database_path"), options.sDatabasePath);
                            std::cout <<  options.sDatabasePath.toStdString() + " - Ok! \n";
                        }
                        else{
                            std::cout << "The path " + sDbpath.toStdString() + " does not exist! \n";
                        }
                    }
                    else{
                        cmdhelp();
                    }
                }

                // Get library information
                if(cmdparam == u"--lib-in"){
                    nId = (parseOption(argc-(i), &argv[i], "--lib-in")).toUInt();
                    if(libs.contains(nId)){

                        std::cout
                                << "Library:\t" << libs[nId].name.toStdString() << "\n"
                                << "Inpx file:\t" << libs[nId].sInpx.toStdString() << "\n"
                                << "Books dir:\t" << libs[nId].path.toStdString() << "\n"
                                << "Version:\t" << libs[nId].sVersion.toStdString() << "\n"
                                << QApplication::translate("LibrariesDlg", "OPDS server").toStdString()
                                << ":\thttp://localhost:" << options.nOpdsPort << "/opds_" << nId << "\n"
                                << QApplication::translate("LibrariesDlg", "HTTP server").toStdString()
                                << ":\thttp://localhost:"  << options.nOpdsPort << "/http_" << nId << "\n";
                    }
                    else{
                        std::cout << QApplication::translate("main", "Library not found!").toStdString() << "\n\n";
                    }
                }


                // Set library path
                if(cmdparam == u"--lib-sp"){
                    QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
                    QString sId = parseOption(argc-(i), &argv[i], "-id");
                    nId = sId.toUInt();
                    if(libs.contains(nId)){

                        QString inpxPath = parseOption(argc-(i), &argv[i], "-inpx");
                        inpxPath = QFileInfo{inpxPath}.absoluteFilePath();
                        QString libPath = parseOption(argc-(i), &argv[i], "-path");
                        libPath = QFileInfo{libPath}.absoluteFilePath();

                        if(!libPath.isEmpty() && !sId.isEmpty()) {

                            std::cout <<  "Updating paths for library: " << sId.toStdString() + "\n";
                            if ( QFile::exists(libPath)){
                                bool result = query.exec(QStringLiteral("UPDATE lib SET path = '%1' WHERE id='%2'")
                                                         .arg(libPath, sId));
                                if(!result)
                                    std::cout << query.lastError().databaseText().toStdString() << "\n";
                                else{
                                    std::cout <<libPath.toStdString() + " - Ok! \n";
                                }
                            }
                            else {
                                std::cout << "The lib path " + libPath.toStdString() + " does not exist\n";
                            }

                            if ( QFile::exists(inpxPath)){
                                bool result = query.exec(QStringLiteral("UPDATE lib SET inpx = '%1' WHERE id='%2'")
                                                         .arg(libPath, sId));
                                if(!result)
                                    std::cout << query.lastError().databaseText().toStdString() << "\n";
                                else{
                                    std::cout <<inpxPath.toStdString() + " - Ok! \n";
                                }
                            }
                            else {
                                std::cout << "The inpx path " + inpxPath.toStdString() + " does not exist\n";
                            }
                        }
                        else{
                            cmdhelp();
                        }
                    }else
                        std::cout << QApplication::translate("main", "Library not found!").toStdString() << "\n\n";
                }

                // Add library
                if(cmdparam == u"--lib-ad"){
                    QString sPath = parseOption(argc-(i), &argv[i], "-path");
                    sPath = QFileInfo{sPath}.absoluteFilePath();
                    QString sInpx = parseOption(argc-(i), &argv[i], "-inpx");
                    sInpx = QFileInfo{sInpx}.absoluteFilePath();
                    QString sName = parseOption(argc-(i), &argv[i], "-name");

                    if ( QFile::exists(sPath)){
                        if(sName.isEmpty()){
                            sName = sInpx.isEmpty() ? QApplication::translate("LibrariesDlg", "new") : SLib::nameFromInpx(sInpx);
                            if(sName.isEmpty())
                                std::cout <<  QApplication::translate("main", "Enter librarry name").toStdString() << ":";
                            else
                                std::cout << QApplication::translate("main", "Enter librarry name").toStdString() << " (" << sName.toStdString() << "):";
                            char newName[1024];
                            std::cin.get(newName, sizeof(newName));
                            QString sNewName = QString::fromUtf8(newName);
                            if(!sNewName.isEmpty())
                                sName = sNewName;
                        }

                        QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
                        bool result = query.exec(QStringLiteral("INSERT INTO lib(name,path,inpx) values('%1','%2','%3')")
                                                 .arg(sName, sPath, sInpx));
                        if(!result){
                            std::cout << query.lastError().databaseText().toStdString() << "\n";
                        }
                        else{
                            std::cout << "Name: \t" << sName.toStdString() << "\tPath: \t" << sPath.toStdString() << "\t - Ok!\n";
                        }
                    }
                    else{
                        std::cout << "\nThe lib path " + sPath.toStdString() + " does not exist\n\n";
                        cmdhelp();
                    }
                }


                // Delete library
                if(cmdparam == u"--lib-dl"){

                    nId = (parseOption(argc-(i), &argv[i], "--lib-dl")).toUInt();
                    if(libs.contains(nId)){
                        char a;
                        std::cout << QApplication::translate("LibrariesDlg", "Delete library ").toStdString()
                                  << "\"" << libs[nId].name.toStdString() << "\"? (y/N)";
                        std::cin.get(a);
                        if(a == 'y'){
                            QSqlQuery query(QSqlDatabase::database(QStringLiteral("libdb")));
                            query.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));
                            query.exec(QStringLiteral("DELETE FROM lib where ID=") + QString::number(nId));
                            query.exec(QStringLiteral("VACUUM"));
                        }
                    }
                    else{
                        std::cout << QApplication::translate("main", "Library not found!").toStdString() << "\n\n";
                    }
                }


                // Update libraries
                if(cmdparam == u"--lib-up"){
                    nId = (parseOption(argc-(i), &argv[i], "--lib-up")).toUInt();

                    if(libs.contains(nId)){
                        auto thread = new QThread;
                        auto imp_tr = new ImportThread();
                        const SLib &lib = libs[nId];

                        imp_tr->init(nId, lib, UT_NEW);
                        imp_tr->moveToThread(thread);
                        QObject::connect(imp_tr, &ImportThread::Message, [](const QString &msg)
                        {
                            std::cout << msg.toStdString() << std::endl;
                        });
                        QObject::connect(thread, &QThread::started, imp_tr, &ImportThread::process);
                        QObject::connect(imp_tr, &ImportThread::End, thread, &QThread::quit);
                        QObject::connect(imp_tr, &ImportThread::End, []()
                        {
                            std::cout << QApplication::translate("LibrariesDlg", "Ending").toStdString() << "\n";
                        });
                        thread->start();
                        while(!thread->wait(500)){
                            QCoreApplication::processEvents();
                        }
                        thread->deleteLater();
                        imp_tr->deleteLater();

                    }
                    else{
                        std::cout << QApplication::translate("main", "Library not found!").toStdString() << "\n\n";
                    }
                }
                delete a;
                return 0;
            }
        }
    }


    if(bServer){
        setenv("QT_QPA_PLATFORM", "offscreen", 1);
        a = new QGuiApplication(argc, argv);
    }else{
        a = new QApplication(argc, argv);
        static_cast<QApplication*>(a)->setStyleSheet(QStringLiteral("QComboBox { combobox-popup: 0; }"));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        a->setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    }

    a->setOrganizationName(QStringLiteral("freeLib"));
    a->setApplicationName(QStringLiteral("freeLib"));

    QString HomeDir;
    if(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).count() > 0)
        HomeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);

    auto  settings = GetSettings();
    options.Load(settings);
    setLocale(options.sUiLanguageName);
    if(options.vExportOptions.isEmpty())
        options.setExportDefault();
    
    QDir::setCurrent(HomeDir);
    QString sDirTmp = QStringLiteral("%1/freeLib").arg(QStandardPaths::standardLocations(QStandardPaths::TempLocation).constFirst());
    QDir dirTmp(sDirTmp);
    if(!dirTmp.exists())
        dirTmp.mkpath(sDirTmp);

    std::unique_ptr<QSplashScreen> splash;
    if(!bServer && options.bShowSplash){
        QPixmap pixmap(QStringLiteral(":/splash%1.png").arg(static_cast<QApplication*>(a)->devicePixelRatio()>=2? QStringLiteral("@2x") :QStringLiteral("")));
        QPainter painter(&pixmap);
        painter.setFont(QFont(painter.font().family(), VERSION_FONT, QFont::Bold));
        painter.setPen(Qt::white);
        painter.drawText(QRect(30, 140, 360, 111), Qt::AlignLeft|Qt::AlignVCenter, QStringLiteral(FREELIB_VERSION));
        splash = std::unique_ptr<QSplashScreen>(new QSplashScreen(pixmap));
#ifdef Q_OS_LINUX
        splash->setWindowIcon(QIcon(QStringLiteral(":/library_128x128.png")));
#endif
        splash->show();
    }

    if(!openDB(QStringLiteral("libdb"))){
        delete a;
        return 1;
    }

    a->processEvents();
    setProxy();
    UpdateLibs();

    MainWindow *pMainWindow = nullptr;

    std::unique_ptr<opds_server> pOpds;
    if(bServer){
        pOpds = std::unique_ptr<opds_server>( new opds_server(a) );
        loadGenres();
        loadLibrary(idCurrentLib);
        options.bOpdsEnable = true;
        pOpds->server_run();
    }else{
        pMainWindow = new MainWindow;
#ifdef Q_OS_OSX
        //  w.setWindowFlags(w.windowFlags() & ~Qt::WindowFullscreenButtonHint);
#endif

        if(!pMainWindow->error_quit)
        {
            if(!bTray && options.nIconTray != 2)
                pMainWindow->show();
        }
        else{
            delete pMainWindow;
            delete a;
            return 1;
        }
        if(options.bShowSplash)
            splash->finish(pMainWindow);
    }

    int result = a->exec();
    if(pMainWindow)
        delete pMainWindow;
    delete a;
    return result;
}
