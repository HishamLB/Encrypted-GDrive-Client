#ifndef SETTINGS_H
#define SETTINGS_H

#include <QWidget>

namespace Ui {
class settings;
}

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
};

#endif // SETTINGS_H
