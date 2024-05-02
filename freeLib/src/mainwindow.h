#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QToolButton>
#include <QSqlQuery>
#include <QtWidgets/QTreeWidgetItem>
#include <QBuffer>
#include <QMenu>
#include <QTcpServer>
#include <QSystemTrayIcon>

#include "helpdialog.h"
#include "options.h"
#include "coverlabel.h"
#include "opds_server.h"
#include "importthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool error_quit;
    
private:
    Ui::MainWindow *ui;
    void fillLanguages();
    void UpdateTags();
    void SaveLibPosition();

    HelpDialog *pHelpDlg;
    QString last_search_symbol;
    QMenu TagMenu;
    QObject* currentListForTag_;
    QMap<uint, QIcon> iconsTags_;

    void uncheck_books(QList<qlonglong> list);
    QToolButton *FirstButton;
    QToolButton *btn_Hash;
    void UpdateExportMenu();
    void FillAuthors();
    void FillSerials();
    void FillGenres();
    void FillListBooks();
    void FillListBooks(const QList<uint> &listBook, const QList<uint> &listCheckedBooks, uint idCurrentAuthor);
    void FillAlphabet(const QString &sAlphabetName);
    bool IsBookInList(const SBook &book);
    void checkLetter(const QChar cLetter);
    QList<uint> getCheckedBooks(bool bCheckedOnly = false);
    void FillCheckedItemsBookList(const QTreeWidgetItem *item, bool send_all, QList<uint> *pList);
    QList<QTreeWidgetItem*> checkedItemsBookList(const QTreeWidgetItem *item = nullptr);
    void setTag(uint idTag, uint id, QList<uint> &listIdTags, QString sTable, bool bSet);
    void updateIcons();
    void updateItemIcon(QTreeWidgetItem *item);
    QIcon getTagIcon(const QList<uint> &listTags);


    QSystemTrayIcon *pTrayIcon_;
    QMenu *pTrayMenu_;
    QAction *pHideAction_;
    QAction *pShowAction_;
    int idCurrentLanguage_;
    uint idCurrentAuthor_;
    ushort idCurrentGenre_;
    uint idCurrentSerial_;
    uint idCurrentBook_;
    QList<uint> listBooks_;
    CoverLabel *pCover;
    bool bTreeView_;
    QByteArray aHeadersTree_;
    QByteArray aHeadersList_;
    enum TabIndex{TabAuthors = 0, TabSeries = 1, TabGenres = 2, TabSearch = 3};
    std::unique_ptr<opds_server> pOpds_;
    ImportThread *pImportThread_;
    QThread *pThread_;

protected:
    void showEvent(QShowEvent *ev) override;
    void closeEvent(QCloseEvent *event) override;
    void FillBookList(QSqlQuery &query);
    void CheckParent(QTreeWidgetItem* parent);
    void CheckChild(QTreeWidgetItem* parent);

    void ExportBookListBtn(bool Enable);
    void FillLibrariesMenu();
    void SendMail(const ExportOptions &exportOptions);
    void SendToDevice(const ExportOptions &exportOptions);
    void changeEvent(QEvent *event) override;
    void onSetRating(QTreeWidgetItem* item, uchar nRating);

private slots:
    void ExportAction();
    void ManageLibrary();
    void onStatistics();
    void onAddBooks();
    void addBooksFinished();
    void CheckBooks();
    void EditBooks();
    void Settings();
    void onSerachAuthorsChanded(const QString& str);
    void onSerachSeriesChanded(const QString& str);
    void btnSearch();
    void DoSearch();
    void SelectAuthor();
    void SelectBook();
    void SelectGenre();
    void SelectSeria();
    void onItemChanged(QTreeWidgetItem*,int);
    void BookDblClick();
    void About();
//    void LanguageChange();
    void StartSearch();
    void HelpDlg();
    void ContextMenu(QPoint point);
    void HeaderContextMenu(QPoint point);
    void MoveToAuthor(uint id, const QString &FirstLetter);
    void MoveToGenre(uint id);
    void MoveToSeria(uint id, const QString &FirstLetter);
    void onTagFilterChanged(int index);
    void onSetTag();
    void ChangingLanguage();
    void ReviewLink(const QUrl &url);
    void SelectLibrary();
    void onTabWidgetChanged(int index);
    void onLanguageFilterChanged(int index);
    void onChangeAlpabet(const QString &sAlphabetName);
    void onTreeView();
    void onListView();

    void ChangingTrayIcon(int index, int color);
    void TrayMenuAction(QSystemTrayIcon::ActivationReason reson);
    void MinimizeWindow();
    void hide();

public slots:
    void show();

signals:
    void window_loaded();
};

#endif // MAINWINDOW_H
