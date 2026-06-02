#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "setup.h"
#include "global.h"
#include "env.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QDirIterator>
#include <QPushButton>
#include <QIcon>
#include <QFile>
#include <QDir>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QAESEncryption>
#include <quazip.h>
#include <quazipfile.h>
#include <qtimer.h>

static QString tokenPath()
{
    return QDir(PROJECT_SOURCE_DIR).absoluteFilePath("token.json");
}

static QString aesPath()
{
    return QDir(PROJECT_SOURCE_DIR).absoluteFilePath("aes.key");
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

    auto *folderBtn = new QPushButton("Upload Folder", ui->page_main);
    ui->horizontalLayout->insertWidget(1, folderBtn);
    connect(folderBtn, &QPushButton::clicked, this, &MainWindow::uploadFolder);
    connect(setupPage, &setup::finished, this, &MainWindow::showMainPage);

    auto *scrollArea = new QScrollArea(ui->page_main);
    scrollArea->setGeometry(90, 90, 651, 460);
    scrollArea->setWidget(ui->gridLayoutWidget);
    scrollArea->setWidgetResizable(true);

    bool authed = restoreToken();

    auto *indicator = new QLabel(this);
    indicator->setFixedSize(16, 16);
    indicator->move(10, 10);
    // doesn't work for some reason: Green when not authed/token not refreshed successfully
  
    if (authed) {
        indicator->setStyleSheet("background-color: #00ff00; border-radius: 8px;");
        indicator->setToolTip("Authenticated");
        QTimer::singleShot(300, this, [this]() {
                getAllFiles();
                });
    } else {
        indicator->setStyleSheet("background-color: #ff0000; border-radius: 8px;");
        indicator->setToolTip("Not authenticated");
    }

    // open file for aes key
    QFile encFile(aesPath());
    if (encFile.open(QIODevice::ReadOnly))
        aes_key = encFile.readAll();

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
    QNetworkReply *reply = apiCall(QUrl("https://www.googleapis.com/drive/v3/files?fields=files(id,name,mimeType,parents)"));
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
            item.mimetype = obj["mimeType"].toString();
            bool isFolder = (item.mimetype == "application/vnd.google-apps.folder");

            driveItems.push_back(item);
            if(isFolder) qDebug() << "aa";
            auto *cell = new QWidget(ui->gridLayoutWidget);
            auto *vl = new QVBoxLayout(cell);
            auto *nameLabel = new QLabel(item.name, cell);
            nameLabel->setAlignment(Qt::AlignCenter);
            vl->addWidget(nameLabel);

            auto *btnRow = new QHBoxLayout;
            auto *dlBtn = new QPushButton(cell);
            dlBtn->setIcon(QIcon(QDir(PROJECT_SOURCE_DIR).absoluteFilePath("download.png")));
            auto *delBtn = new QPushButton(cell);
            delBtn->setIcon(QIcon(QDir(PROJECT_SOURCE_DIR).absoluteFilePath("trash.png")));
            connect(dlBtn, &QPushButton::clicked, this, [this, item]() {
                download(item.fileId, item.name);
            });
            connect(delBtn, &QPushButton::clicked, this, [this, item]() {
                deleteFile(item.fileId);
            });
            btnRow->addStretch();
            btnRow->addWidget(dlBtn);
            btnRow->addWidget(delBtn);
            vl->addLayout(btnRow);

            cell->setStyleSheet("border: 1px solid gray;");
            cell->setMinimumSize(160, 100);
            grid->addWidget(cell, i / cols, i % cols);
        }
        reply->deleteLater();
    });
}

void MainWindow::deleteFile(const QString fileId)
{
    auto result = QMessageBox::question(this, "Delete File",
        "Are you sure you want to delete this file?",
        QMessageBox::Yes | QMessageBox::No);

    if (result != QMessageBox::Yes)
        return;

    QUrl url(
        QString("https://www.googleapis.com/drive/v3/files/%1")
            .arg(fileId));

    QNetworkReply *reply = apiDelete(url);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {

        if (reply->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Success", "File deleted.");
            getAllFiles();
        }
        else {
            QMessageBox::warning(this, "Error",
                QString("Delete failed: %1").arg(reply->errorString()));
        }

        reply->deleteLater();
    });
}

QNetworkReply* MainWindow::apiDelete(const QUrl url)
{
    QNetworkRequest request(url);

    request.setRawHeader(
        "Authorization",
        QString("Bearer %1").arg(oauth->token()).toUtf8());

    return networkManager->deleteResource(request);
}

void MainWindow::download(QString fileId, QString name){
    // @param {string} fileId (ID to download)
    // @return {Blob} file content as a Blob.

    // set download location first for convenience:

    QString savePath =
        QFileDialog::getSaveFileName(
        this,
        "Save File",
        name);

    if (savePath.isEmpty())
        return;


    // construct
    QUrl url = "https://www.googleapis.com/drive/v3/files/" + fileId + "?alt=media";

    // send
    QNetworkReply *reply = apiCall(url);


    connect(reply, &QNetworkReply::finished,
            this,
            [reply, savePath]()
            {
        if (reply->error() != QNetworkReply::NoError)
        {
            qDebug() << reply->errorString();
            reply->deleteLater();
            return;
        }

        QFile out(savePath);

        if (!out.open(QIODevice::WriteOnly))
        {
            qDebug() << "Failed to open output file";
            reply->deleteLater();
            return;
        }

        out.write(reply->readAll());
        out.close();

        qDebug() << "Download complete";

        reply->deleteLater();
    });

}

void MainWindow::uploadFile(const QString &filePath)
{
    QFile sourceFile(filePath);
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open source file";
        return;
    }

    QFileInfo info(filePath);

    QString zipPath = QDir::temp().filePath(info.fileName() + ".zip");

    QuaZip zip(zipPath);
    if (!zip.open(QuaZip::mdCreate)) {
        qDebug() << "Failed to create zip";
        return;
    }

    QuaZipFile zipFile(&zip);
    if (!zipFile.open(QIODevice::WriteOnly,
                      QuaZipNewInfo(info.fileName(), info.fileName()))) {
        qDebug() << "Failed to write zip entry";
        zip.close();
        return;
    }

    zipFile.write(sourceFile.readAll());
    zipFile.close();
    zip.close();
    sourceFile.close();

    // READ ZIP INTO MEMORY
    QFile zipIn(zipPath);
    if (!zipIn.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open zip";
        return;
    }

    QByteArray plainData = zipIn.readAll();
    zipIn.close();

    // ENCRYPT DATA
    QAESEncryption enc(QAESEncryption::AES_256,
                       QAESEncryption::CBC,
                       QAESEncryption::PKCS7);

    QByteArray iv = "IDONTCAREIDONTCA"; // IV

    QByteArray cipher = enc.encode(plainData, aes_key, iv);

    // PREPARE MULTIPART UPLOAD
    QHttpMultiPart *multiPart =
        new QHttpMultiPart(QHttpMultiPart::RelatedType);

    QJsonObject metadata;
    metadata["name"] = info.fileName() + ".zip.enc";

    QHttpPart metaPart;
    metaPart.setHeader(QNetworkRequest::ContentTypeHeader,
                       "application/json; charset=UTF-8");
    metaPart.setBody(QJsonDocument(metadata).toJson(QJsonDocument::Compact));

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
                       "application/octet-stream");

    filePart.setBody(cipher);
    multiPart->append(metaPart);
    multiPart->append(filePart);

    // SEND
    QNetworkReply *reply = apiCall(
        QUrl("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart"),
        multiPart);

    connect(reply, &QNetworkReply::finished, this, [this, reply, zipPath]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Upload success:";
            qDebug().noquote() << reply->readAll();

            QFile::remove(zipPath);
            getAllFiles();
        } else {
            qDebug() << "Upload failed:" << reply->errorString();
            qDebug().noquote() << reply->readAll();
        }

        reply->deleteLater();
    });
}

void MainWindow::upload()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    if (!dialog.exec())
        return;

    for (const QString &file : dialog.selectedFiles())
        uploadFile(file);
    getAllFiles();
}

void MainWindow::uploadFolder()
{
    QString folder = QFileDialog::getExistingDirectory(this, "Select Folder");
    if (folder.isEmpty())
        return;

    QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
    uploadFolderAsZip(folder);
}


// clean this later im lazy
void MainWindow::uploadFolderAsZip(const QString &folderPath)
{
    QFileInfo folderInfo(folderPath);
    QString zipPath = QDir::temp().filePath(folderInfo.fileName() + ".zip");
    QString encPath = zipPath + ".enc";

    QuaZip zip(zipPath);
    if (!zip.open(QuaZip::mdCreate)) {
        qDebug() << "Failed to create zip";
        return;
    }

    QDir dir(folderPath);
    QDirIterator it(folderPath,
                    QDir::Files | QDir::NoDotAndDotDot,    // what?
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filePath = it.next();

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            continue;

        QString relativePath = dir.relativeFilePath(filePath);

        QuaZipFile zipFile(&zip);

        QuaZipNewInfo info(relativePath, filePath);

        if (!zipFile.open(QIODevice::WriteOnly, info)) {
            qDebug() << "Failed to add file:" << filePath;
            continue;
        }

        zipFile.write(file.readAll());
        zipFile.close();
        file.close();
    }

    zip.close();
    QFile in(zipPath);
    if (!in.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open zip";
        return;
    }

    QByteArray zipData = in.readAll();
    in.close();

    QAESEncryption enc(QAESEncryption::AES_256,
                       QAESEncryption::CBC,
                       QAESEncryption::PKCS7);

    QByteArray iv = "IDONTCAREIDONTCA"; // IV

    QByteArray encrypted = enc.encode(zipData, aes_key, iv);

    QFile out(encPath);
    if (!out.open(QIODevice::WriteOnly)) {
        qDebug() << "Cannot write encrypted file";
        return;
    }

    out.write(encrypted);
    out.close();

    // upload zip
    QFile *zipFile = new QFile(encPath);
    if (!zipFile->open(QIODevice::ReadOnly)) {
        delete zipFile;
        return;
    }

    QJsonObject metadata;
    metadata["name"] = folderInfo.fileName() + ".zip";

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::RelatedType);

    QHttpPart metaPart;
    metaPart.setHeader(QNetworkRequest::ContentTypeHeader,
                       "application/json; charset=UTF-8");
    metaPart.setBody(QJsonDocument(metadata).toJson(QJsonDocument::Compact));

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
                       "application/zip");
    filePart.setBodyDevice(zipFile);

    zipFile->setParent(multiPart);

    multiPart->append(metaPart);
    multiPart->append(filePart);

    QNetworkReply *reply = apiCall(
        QUrl("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart"),
        multiPart
    );

    connect(reply, &QNetworkReply::finished, this, [this, reply, encPath]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Folder upload success";
            QFile::remove(encPath);
            getAllFiles();
        } else {
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
    getAllFiles();
}

MainWindow::~MainWindow()
{
    delete ui;
}
