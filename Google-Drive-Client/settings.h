#ifndef SETTINGS_H
#define SETTINGS_H

#include <QWidget>

namespace Ui {
class settings;
}

extern const char* defaultStylesheet;
extern const char* catppuccinStylesheet;

class settings : public QWidget
{
    Q_OBJECT


signals:
    void settingsFinished();
public:
    explicit settings(QWidget *parent = nullptr);
    ~settings();

private:
    Ui::settings *ui;
    void writeToConfig();
};

#endif // SETTINGS_H
