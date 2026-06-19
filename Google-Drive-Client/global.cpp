#include "global.h"

QOAuth2AuthorizationCodeFlow* oauth = nullptr;
QByteArray aes_key;
bool catppuccinTheme = false;
QByteArray iv;

bool encryptFileNames;
bool debug;

QString encryption_method;
