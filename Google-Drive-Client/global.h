#ifndef GLOBAL_H
#define GLOBAL_H
#include <QOAuth2AuthorizationCodeFlow>
#include <QApplication>
#include <QDir>


extern QOAuth2AuthorizationCodeFlow* oauth;
extern QByteArray aes_key;
extern QByteArray iv;
extern bool catppuccinTheme;

static QString configPath()
{
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("config.json");
}


#endif // GLOBAL_H
