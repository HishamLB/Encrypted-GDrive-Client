#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QLabel>
#include "driveitem.h"

class settings;

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
    QLabel* indicator;
    void readConfig();
    bool authed;
    settings* settingsPage;
    void writeFilesToConfig();
    QMap<QString, QString> files;
    void decryptFileName(QString&);
    QWidget* createBlocking();
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    void download(QString, QString);
    void refreshIndicator();
    void deleteFile(QString);
    QNetworkReply* apiDelete(const QUrl);
    QNetworkReply* apiCall(const QUrl &url);
    void uploadFolderAsZip(const QString &folderPath);

    QNetworkReply* apiCall(const QUrl &url, QHttpMultiPart *multiPart);
    void uploadFile(const QString &filePath);
    void showSettingsPage();
    void upload();
    void uploadFolder();
    std::vector<driveItem> driveItems;
    void getAllFiles();
    void showSetupPage();
    void showMainPage();
};
#endif // MAINWINDOW_H
