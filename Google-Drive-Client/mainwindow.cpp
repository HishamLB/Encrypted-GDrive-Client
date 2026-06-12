#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "setup.h"
#include "global.h"
#include "env.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QDirIterator>
#include <QPushButton>
#include <QProcess>
#include <QIcon>
#include <QFile>
#include <QDir>
#include <QLabel>
#include <QVBoxLayout>
#include <QCoreApplication>
#include <QGridLayout>
#include <QScrollArea>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <quazip.h>
#include <quazipfile.h>
#include <qtimer.h>
#include "settings.h"

static QString tokenPath()
{
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("token.json");
}

static QString aesPath()
{
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("aes.key");
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
    oauth->setAccessTokenUrl(QUrl("https://oauth2.googleapis.com/token"));
    oauth->setAuthorizationUrl(QUrl("https://accounts.google.com/o/oauth2/v2/auth"));
    oauth->setScope("https://www.googleapis.com/auth/drive.file");
    oauth->setToken(accessToken);

    QString refreshToken = obj["refresh_token"].toString();
    if (!refreshToken.isEmpty())
        oauth->setRefreshToken(refreshToken);

    return true;

}

void MainWindow::readConfig(){
    QFile configFile(configPath());


    if (!configFile.open(QIODevice::ReadOnly))
        return;

    QJsonObject obj = QJsonDocument::fromJson(configFile.readAll()).object();

    qint32 theme = obj["theme"].toInt();

    bool readEncryptFileNames = obj["encryptfilenames"].toBool();
    encryptFileNames = readEncryptFileNames;

    bool debugMode = obj["debug"].toBool();
    debug = debugMode;

    catppuccinTheme = (theme == 1);
    if (catppuccinTheme) {
        qApp->setStyleSheet(catppuccinStylesheet);
        QIcon dlIcon(":/download_catpp.png");
        QIcon delIcon(":/trash_catpp.png");
        for (auto* tlw : qApp->topLevelWidgets()) {
            for (auto* btn : tlw->findChildren<QPushButton*>("downloadBtn"))
                btn->setIcon(dlIcon);
            for (auto* btn : tlw->findChildren<QPushButton*>("deleteBtn"))
                btn->setIcon(delIcon);
        }
    } else {
        qApp->setStyleSheet(defaultStylesheet);
        QIcon dlIcon(":/download.png");
        QIcon delIcon(":/trash.png");
        for (auto* tlw : qApp->topLevelWidgets()) {
            for (auto* btn : tlw->findChildren<QPushButton*>("downloadBtn"))
                btn->setIcon(dlIcon);
            for (auto* btn : tlw->findChildren<QPushButton*>("deleteBtn"))
                btn->setIcon(delIcon);
        }
    }
}

void MainWindow::refreshIndicator(){
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
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    qApp->setStyleSheet("* { font-family: \"JetBrains Mono\"; font-weight: bold; } QWidget#driveItemCell { border: 1px solid gray; }");

    iv = "IDONTCAREIDONTCA"; // IV
    auto *setupPage = new setup(this);
    settingsPage = new settings;
    ui->stackedWidget->addWidget(setupPage);
    ui->stackedWidget->addWidget(settingsPage);
    connect(ui->setup_button, &QPushButton::clicked, this, &MainWindow::showSetupPage);
    connect(ui->upload_button, &QPushButton::clicked, this, &MainWindow::upload);
    
    auto *folderBtn = new QPushButton("Upload Folder", ui->page_main);
    ui->horizontalLayout->insertWidget(1, folderBtn);
    connect(folderBtn, &QPushButton::clicked, this, &MainWindow::uploadFolder);
    connect(setupPage, &setup::finished, this, &MainWindow::showMainPage);
    connect(settingsPage, &settings::settingsFinished, this, [this](){
            readConfig();
            ui->stackedWidget->setCurrentIndex(0);
            });

    connect(ui->settings_button, &QPushButton::clicked, this, &MainWindow::showSettingsPage);

    authed = restoreToken();

    indicator = new QLabel(this);
    indicator->setFixedSize(16, 16);
    indicator->move(10, 10);
    
    refreshIndicator();

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

    readConfig();
}

void MainWindow::showSettingsPage(){
    settingsPage->setEncryptFileNamesBox(encryptFileNames);
    settingsPage->setDebugBox(debug);
    ui->stackedWidget->setCurrentIndex(2);
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
            dlBtn->setObjectName("downloadBtn");
            dlBtn->setIcon(QIcon(catppuccinTheme ? ":/download_catpp.png" : ":/download.png"));
            auto *delBtn = new QPushButton(cell);
            delBtn->setObjectName("deleteBtn");
            delBtn->setIcon(QIcon(catppuccinTheme ? ":/trash_catpp.png" : ":/trash.png"));
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

            cell->setObjectName("driveItemCell");
            cell->setMinimumSize(160, 100);
            grid->addWidget(cell, i / cols, i % cols);
        }
        reply->deleteLater();
    });
}

QWidget* MainWindow::createBlocking(){
    QWidget *overlay = new QWidget(this);
    overlay->setGeometry(this->rect());
    overlay->setStyleSheet("background-color: rgba(0,0,0,120);");
    overlay->show();
    overlay->raise();
    overlay->setEnabled(true);
    overlay->grabMouse();
    return overlay;
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

    QWidget* overlay = createBlocking();

    connect(reply, &QNetworkReply::finished, this, [this, reply, overlay]() {
        overlay->releaseMouse();
        overlay->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Success", "File deleted.");
            getAllFiles();          // we might get rate limited if Google cares
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

void MainWindow::decryptFileName(QString& name){

    QByteArray cipher = QByteArray::fromBase64(
        name.toLatin1(),
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals
    );

    QProcess process;
    QStringList args;
    args << "enc"
         << "-d"
         << "-aes-256-cbc"
         << "-K" << aes_key.toHex()
         << "-iv" << iv.toHex();
    process.start("openssl", args);
    if (!process.waitForStarted())
        return;
    process.write(cipher);
    process.closeWriteChannel();
    if (!process.waitForFinished())
        return;

    QByteArray plain = process.readAllStandardOutput();
    name = QString::fromUtf8(plain);}

void MainWindow::download(QString fileId, QString name){
    // @param {string} fileId (ID to download)
    // @return {Blob} file content as a Blob.

    // set download location first for convenience:
    // decrypt on download

    QString cleanName = name;
    if (cleanName.endsWith(".zip.enc"))
        cleanName.chop(8);
    else if (cleanName.endsWith(".zip"))
        cleanName.chop(4);

    if (cleanName.contains("enc_")){
        cleanName = cleanName.mid(4);
        decryptFileName(cleanName);
    }

    QString savePath =
        QFileDialog::getSaveFileName(
        this,
        "Save File",
        cleanName);

    if (savePath.isEmpty())
        return;


    // construct
    QUrl url = "https://www.googleapis.com/drive/v3/files/" + fileId + "?alt=media";

    // send
    QElapsedTimer timer;

    if(debug)
        timer.start();

    QNetworkReply *reply = apiCall(url);

    QWidget* overlay = createBlocking();
    connect(reply, &QNetworkReply::finished,
            this,
            [this, reply, savePath, name, overlay, timer]() mutable
            {
        overlay->releaseMouse();
        overlay->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << reply->errorString();
            reply->deleteLater();
            return;
        }

        QByteArray encryptedFile = reply->readAll();

        if(debug)
            qDebug() << "Downloaded in " << timer.elapsed() << "ms";

        if(debug)
            timer.start();

        QProcess process;

        QStringList args;
        args << "enc"
             << "-d"
             << "-aes-256-cbc"
             << "-K" << aes_key.toHex()
             << "-iv" << iv.toHex();

        process.start("openssl", args);

        if (!process.waitForStarted()) {
            qDebug() << "Failed to start OpenSSL";
            return;
        }

        process.write(encryptedFile);   // encrypted data
        process.closeWriteChannel();

        if (!process.waitForFinished()) {
            qDebug() << "OpenSSL failed";
            return;
        }

        QByteArray zipData = process.readAllStandardOutput();

        QByteArray errorOutput = process.readAllStandardError();
        if (!errorOutput.isEmpty()) {
            qDebug() << "OpenSSL error:" << errorOutput;
        }

        if(debug)
            qDebug() << "Decryption done in " << timer.elapsed() << "ms";

        if(debug)
            timer.start();

        // always zip
        QString tempZip = savePath + ".zip";

        QFile zipFile(tempZip);
        if (!zipFile.open(QIODevice::WriteOnly)) {
            qDebug() << "Failed writing zip";
            reply->deleteLater();
            return;
        }

        zipFile.write(zipData);
        zipFile.close();

        QuaZip zip(tempZip);
        if (!zip.open(QuaZip::mdUnzip)) {
            qDebug() << "Failed opening zip";
            QFile::remove(tempZip);
            reply->deleteLater();
            return;
        }

        // one file
        bool isSingleFile = name.endsWith(".zip.enc");

        if (isSingleFile) {
            QDir().mkpath(QFileInfo(savePath).absolutePath());
            for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {
                QuaZipFile in(&zip);
                if (!in.open(QIODevice::ReadOnly))
                    continue;
                QFile out(savePath);
                if (out.open(QIODevice::WriteOnly)) {
                    out.write(in.readAll());
                    out.close();
                }
                in.close();
                break;
            }
        } else {            // folder
            QString extractDir = savePath;
            QDir().mkpath(extractDir);
            for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {
                QuaZipFile in(&zip);
                if (!in.open(QIODevice::ReadOnly))
                    continue;
                QuaZipFileInfo64 info;
                zip.getCurrentFileInfo(&info);
                QString outPath = extractDir + "/" + info.name;
                QDir().mkpath(QFileInfo(outPath).absolutePath());
                QFile out(outPath);
                if (out.open(QIODevice::WriteOnly)) {
                    out.write(in.readAll());
                    out.close();
                }
                in.close();
            }
        }

        zip.close();
        QFile::remove(tempZip);

        if(debug)
            qDebug() << "Zip extracted in " << timer.elapsed() << "ms";

        qDebug() << "Restore complete";

        reply->deleteLater();
    });

}

static QString encryptedFileName(const QString &name)
{
    if (!encryptFileNames)
        return name;
    QProcess process;
    QStringList args;
    args << "enc"
         << "-aes-256-cbc"
         << "-K" << aes_key.toHex()
         << "-iv" << iv.toHex();
    process.start("openssl", args);
    if (!process.waitForStarted())
        return name;
    process.write(name.toUtf8());
    process.closeWriteChannel();
    if (!process.waitForFinished())
        return name;
    QByteArray encName = process.readAllStandardOutput();
    QString b64 = QString::fromLatin1(encName.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    return "enc_" + b64;
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

    QElapsedTimer timer;

    if(debug)
        timer.start();

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

    if(debug)
        qDebug() << "Temp zip created in " << timer.elapsed() << "ms";

    // READ ZIP INTO MEMORY
    QFile zipIn(zipPath);
    if (!zipIn.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open zip";
        return;
    }

    if(debug)
        timer.start();

    QByteArray plainData = zipIn.readAll();
    zipIn.close();

    if(debug)
        qDebug() << "Temp zip read in " << timer.elapsed() << "ms";

    if(debug)
        timer.start();


    QProcess process;

    QStringList args;
    args << "enc"
         << "-aes-256-cbc"
         << "-K" << aes_key.toHex()
         << "-iv" << iv.toHex();
    process.start("openssl", args);

    if (!process.waitForStarted()) {
        qDebug() << "Failed to start OpenSSL";
        return;
    }
    process.write(plainData);
    process.closeWriteChannel();
    if (!process.waitForFinished()) {
        qDebug() << "OpenSSL failed";
        return;
    }

    QByteArray cipher = process.readAllStandardOutput();
    QByteArray errorOutput = process.readAllStandardError();
    if (!errorOutput.isEmpty()) {
        qDebug() << "OpenSSL error:" << errorOutput;
    }

    if(debug)
        qDebug() << "Encryption done in " << timer.elapsed() << "ms";


    if(debug)
        timer.start();

    // PREPARE MULTIPART UPLOAD
    QHttpMultiPart *multiPart =
        new QHttpMultiPart(QHttpMultiPart::RelatedType);

    QJsonObject metadata;
    metadata["name"] = encryptedFileName(info.fileName()) + ".zip.enc";

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

    if(debug)
        qDebug() << "Request assembled in " << timer.elapsed() << "ms";

    if(debug)
        timer.start();

    // SEND
    QNetworkReply *reply = apiCall(
                QUrl("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart"),
                multiPart);

    QWidget* overlay = createBlocking();
    connect(reply, &QNetworkReply::finished, this, [this, reply, zipPath, overlay, timer]() {
        overlay->releaseMouse();
        overlay->deleteLater();
        if(debug)
            qDebug() << "Uploaded in " << timer.elapsed() << "ms";
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

    QElapsedTimer timer;

    if(debug)
        timer.start();

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

    if(debug)
        qDebug() << "Temp zip created in " << timer.elapsed() << "ms";

    QFile in(zipPath);
    if (!in.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open zip";
        return;
    }

    if(debug)
        timer.start();

    QByteArray zipData = in.readAll();
    in.close();

    if(debug)
        qDebug() << "Temp zip read in " << timer.elapsed() << "ms";

    if(debug)
        timer.start();

    QProcess process;

    QStringList args;
    args << "enc"
         << "-aes-256-cbc"
         << "-K" << aes_key.toHex()
         << "-iv" << iv.toHex();
    process.start("openssl", args);

    if (!process.waitForStarted()) {
        qDebug() << "Failed to start OpenSSL";
        return;
    }
    process.write(zipData);
    process.closeWriteChannel();
    if (!process.waitForFinished()) {
        qDebug() << "OpenSSL failed";
        return;
    }

    QByteArray encrypted = process.readAllStandardOutput();
    QByteArray errorOutput = process.readAllStandardError();
    if (!errorOutput.isEmpty()) {
        qDebug() << "OpenSSL error:" << errorOutput;
    }
    QFile out(encPath);
    if (!out.open(QIODevice::WriteOnly)) {
        qDebug() << "Cannot write encrypted file";
        return;
    }

    out.write(encrypted);
    out.close();

    if(debug)
        qDebug() << "Encryption done in " << timer.elapsed() << "ms";

    if(debug)
        timer.start();

    // upload zip
    QFile *zipFile = new QFile(encPath);
    if (!zipFile->open(QIODevice::ReadOnly)) {
        delete zipFile;
        return;
    }

    QJsonObject metadata;
    metadata["name"] = encryptedFileName(folderInfo.fileName()) + ".zip";

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

    if(debug)
        qDebug() << "Request assembled in " << timer.elapsed() << "ms";

    if(debug)
        timer.start();

    QNetworkReply *reply = apiCall(
        QUrl("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart"),
        multiPart
        );

    connect(reply, &QNetworkReply::finished, this, [this, reply, encPath, timer]() {
        if(debug)
            qDebug() << "Uploaded in " << timer.elapsed() << "ms";
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
    refreshIndicator();
    ui->stackedWidget->setCurrentIndex(0);
    getAllFiles();
}

MainWindow::~MainWindow()
{
    delete ui;
}
