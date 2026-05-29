#ifndef SETUP_H
#define SETUP_H

#include <QWidget>
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
QT_BEGIN_NAMESPACE
namespace Ui {
class setup;
}
QT_END_NAMESPACE

class setup : public QWidget
{
    Q_OBJECT

public:
    explicit setup(QWidget *parent = nullptr);
    ~setup();

signals:
    void finished();

private:
    Ui::setup *ui;
    void googlesignin();
};

#endif // SETUP_H
