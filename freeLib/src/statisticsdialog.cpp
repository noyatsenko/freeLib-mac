#include <QLocale>

#include "statisticsdialog.h"
#include "ui_statisticsdialog.h"
#include "library.h"

StatisticsDialog::StatisticsDialog(QWidget *parent) :
    QDialog(parent, Qt::Dialog | Qt::WindowTitleHint),
    ui(new Ui::StatisticsDialog)
{
    ui->setupUi(this);

    QLocale locale;
    SLib& lib = libs[idCurrentLib];
    QString sText;
    sText = QStringLiteral("<table>"
                           "<tr><td width=\"50%\">%1</td><td>%2</td></tr>"
                           "<tr><td>%3</td><td>%4</td></tr>"
                           "<tr><td>%5</td><td>%6</td></tr>"
                           "<tr><td>%7</td><td>%8</td></tr>"
                           "<tr><td>%9</td><td>%10</td></tr>"
                           "</table>").arg(
                tr("Library name"), lib.name,
                tr("Version"), lib.sVersion,
                tr("Book count"), locale.toString(lib.books.count()),
                tr("Author count"), locale.toString(lib.authors.count())
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                ,
#else
                ).arg(
#endif
                tr("Seria count"), locale.toString(lib.serials.count()));
    ui->textEdit->setText(sText);
}

StatisticsDialog::~StatisticsDialog()
{
    delete ui;
}
