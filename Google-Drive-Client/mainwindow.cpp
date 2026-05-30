#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "setup.h"
#include "global.h"
#include "env.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QLabel>
#include <QGridLayout>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

static QString tokenPath()
{
    return QDir(PROJECT_SOURCE_DIR).absoluteFilePath("token.json");
}

static bool restoreToken()
{
    QFile file(tokenPath());
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    QString accessToken = obj["access_token"].toString();
    if (accessToken.isEmpty())
        return false;

    // auth persistence
    oauth = new QOAuth2AuthorizationCodeFlow();
    oauth->setClientIdentifier(QString::fromStdString(getEnv("CLIENT_ID")));
    oauth->setClientIdentifierSharedKey(QString::fromStdString(getEnv("CLIENT_SECRET")));
    oauth->setTokenUrl(QUrl("https://oauth2.googleapis.com/token"));
    oauth->setAuthorizationUrl(QUrl("https://accounts.google.com/o/oauth2/v2/auth"));
    oauth->setRequestedScopeTokens({"https://www.googleapis.com/auth/drive.file"});
    oauth->setToken(accessToken);

    QString refreshToken = obj["refresh_token"].toString();
    if (!refreshToken.isEmpty()) {
        oauth->setRefreshToken(refreshToken);
        oauth->refreshTokens();
    }

    return true;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    auto *setupPage = new setup(this);
    ui->stackedWidget->addWidget(setupPage);

    connect(ui->setup_button, &QPushButton::clicked, this, &MainWindow::showSetupPage);
    connect(ui->upload_button, &QPushButton::clicked, this, &MainWindow::upload);
    connect(setupPage, &setup::finished, this, &MainWindow::showMainPage);

    auto *indicator = new QLabel(this);
    indicator->setFixedSize(16, 16);
    indicator->move(10, 10);
    bool authed = restoreToken() || (oauth && oauth->status() == QAbstractOAuth::Status::Granted);
    if (authed) {
        indicator->setStyleSheet("background-color: #00ff00; border-radius: 8px;");
        indicator->setToolTip("Authenticated");
        getAllFiles();
    } else {
        indicator->setStyleSheet("background-color: #ff0000; border-radius: 8px;");
        indicator->setToolTip("Not authenticated");
    }
}

QNetworkReply* MainWindow::apiCall(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setRawHeader("Authorization",
        QString("Bearer %1").arg(oauth->token()).toUtf8());
    return networkManager->get(request);
}

QNetworkReply* MainWindow::apiCall(const QUrl &url, QHttpMultiPart *multiPart)
{
    QNetworkRequest request(url);
    request.setRawHeader("Authorization",
        QString("Bearer %1").arg(oauth->token()).toUtf8());
    QNetworkReply *reply = networkManager->post(request, multiPart);
    multiPart->setParent(reply);
    return reply;
}

void MainWindow::getAllFiles()
{
    QNetworkReply *reply = apiCall(QUrl("https://www.googleapis.com/drive/v3/files"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "List failed:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray files = doc.object()["files"].toArray();

        QGridLayout *grid = ui->items;
        while (auto *item = grid->takeAt(0)) {
            if (auto *w = item->widget())
                delete w;
            delete item;
        }
        driveItems.clear();

        int cols = 3;
        for (int i = 0; i < files.size(); ++i) {
            QJsonObject obj = files[i].toObject();
            driveItem item;
            item.fileId = obj["id"].toString();
            item.name = obj["name"].toString();
            driveItems.push_back(item);

            auto *label = new QLabel(item.name, ui->gridLayoutWidget);
            label->setFrameStyle(QFrame::Box);
            label->setAlignment(Qt::AlignCenter);
            label->setMinimumSize(150, 80);
            grid->addWidget(label, i / cols, i % cols);
        }
        reply->deleteLater();
    });
}

void MainWindow::upload()
{
    QString filePath =
        QFileDialog::getOpenFileName(this, "Select file to upload");

    if (filePath.isEmpty())
        return;

    QFile *file = new QFile(filePath);

    if (!file->open(QIODevice::ReadOnly))
    {
        delete file;
        return;
    }

    QFileInfo info(filePath);

    QHttpMultiPart *multiPart =
        new QHttpMultiPart(QHttpMultiPart::RelatedType);

    QJsonObject metadata;
    metadata["name"] = info.fileName();

    QHttpPart metaPart;
    metaPart.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/json; charset=UTF-8");
    metaPart.setBody(
        QJsonDocument(metadata).toJson(QJsonDocument::Compact));

    QHttpPart filePart;
    filePart.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/octet-stream");
    filePart.setBodyDevice(file);
    file->setParent(multiPart);

    multiPart->append(metaPart);
    multiPart->append(filePart);

    QNetworkReply *reply = apiCall(
        QUrl("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart"),
        multiPart);

    connect(reply, &QNetworkReply::finished, this, [reply]() {
        if (reply->error() == QNetworkReply::NoError)
        {
            qDebug() << "Upload success:";
            qDebug().noquote() << reply->readAll();
        }
        else
        {
            qDebug() << "Upload failed:" << reply->errorString();
            qDebug().noquote() << reply->readAll();
        }

        reply->deleteLater();
    });
}

void MainWindow::showSetupPage()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::showMainPage()
{
    ui->stackedWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}
