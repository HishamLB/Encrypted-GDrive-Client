#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QLabel>
#include "driveitem.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    void download(QString, QString);
    QNetworkReply* apiCall(const QUrl &url);
    QNetworkReply* apiCall(const QUrl &url, QHttpMultiPart *multiPart);
    void upload();
    std::vector<driveItem> driveItems;
    void getAllFiles();
    void showSetupPage();
    void showMainPage();
};
#endif // MAINWINDOW_H
