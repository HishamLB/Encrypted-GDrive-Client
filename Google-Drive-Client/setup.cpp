#include "setup.h"
#include "ui_setup.h"
#include "env.h"
#include "global.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDir>
#include <QCoreApplication>

static QString tokenPath()
{
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("token.json");
}

static void saveToken()
{
    if (!oauth)
        return;
    QJsonObject obj;
    obj["access_token"] = oauth->token();
    obj["refresh_token"] = oauth->refreshToken();

    QFile file(tokenPath());
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(obj).toJson());
}

setup::setup(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::setup)
{
    ui->setupUi(this);
    connect(ui->signin_button, &QPushButton::clicked, this, &setup::googlesignin);
    connect(ui->finish_setup_button, &QPushButton::clicked, this, &setup::finished);
}

void setup::googlesignin()
{
    if (oauth) {
        delete oauth;
        oauth = nullptr;
    }
    oauth = new QOAuth2AuthorizationCodeFlow(this);

    oauth->setAuthorizationUrl(
        QUrl("https://accounts.google.com/o/oauth2/v2/auth"));

    oauth->setAccessTokenUrl(
        QUrl("https://oauth2.googleapis.com/token"));

    oauth->setClientIdentifier(QString::fromStdString(getEnv("CLIENT_ID")));
    oauth->setClientIdentifierSharedKey(QString::fromStdString(getEnv("CLIENT_SECRET")));

    oauth->setScope("https://www.googleapis.com/auth/drive.file");

    auto replyHandler =
        new QOAuthHttpServerReplyHandler(
            8089,
            this);

    oauth->setReplyHandler(replyHandler);

    connect(oauth,
            &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            &QDesktopServices::openUrl);

    connect(oauth,
            &QOAuth2AuthorizationCodeFlow::granted,
            this,
            [=]() {
                qDebug() << "TOKEN:" << oauth->token();
                saveToken();
            });

    oauth->grant();
}
setup::~setup()
{
    delete ui;
}
