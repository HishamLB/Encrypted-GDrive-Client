#include "settings.h"
#include <QButtonGroup>
#include "ui_settings.h"
#include "global.h"
#include <QApplication>
#include <QIcon>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>

const char* defaultStylesheet = R"(
    * {
        font-family: "JetBrains Mono";
        font-weight: bold;
    }
    QWidget#driveItemCell {
        border: 1px solid gray;
    }
    QFrame#formFrame {
        border: 1px solid palette(mid);
        border-radius: 8px;
        background: palette(window);
    }
    QRadioButton {
        spacing: 8px;
    }
    QRadioButton::indicator {
        width: 18px;
        height: 18px;
        border: 2px solid palette(shadow);
        border-radius: 10px;
        background: palette(base);
    }
    QRadioButton::indicator:checked {
        background: palette(highlight);
        border: 2px solid palette(highlight);
    }
)";

const char* catppuccinStylesheet = R"(
    * {
        font-family: "JetBrains Mono";
        font-weight: bold;
    }
    QMainWindow, QWidget#centralwidget, QWidget#gridLayoutWidget {
        background-color: #24273a;
    }
    QLabel {
        color: #cad3f5;
        background: transparent;
    }
    QWidget#driveItemCell {
        background-color: #24273a;
        border: 1px solid #363a4f;
    }
    QFrame#formFrame {
        border: 1px solid #363a4f;
        border-radius: 8px;
        background-color: #1e2030;
    }
    QPushButton {
        background-color: #363a4f;
        color: #cad3f5;
        border: 1px solid #494d64;
        border-radius: 6px;
        padding: 6px 16px;
        font-size: 13px;
    }
    QPushButton:hover {
        background-color: #494d64;
        border: 1px solid #5b6078;
    }
    QPushButton:pressed {
        background-color: #5b6078;
    }
    QTableView {
        background-color: #24273a;
        alternate-background-color: #1e2030;
        color: #cad3f5;
        gridline-color: #363a4f;
        border: 1px solid #363a4f;
        selection-background-color: #363a4f;
        selection-color: #cad3f5;
        font-size: 15px;
    }
    QHeaderView::section {
        background-color: #1e2030;
        color: #8aadf4;
        border: 1px solid #363a4f;
        padding: 8px;
        font-weight: bold;
        font-size: 14px;
    }
    QTextEdit, QTextBrowser {
        background-color: #24273a;
        color: #cad3f5;
        border: 1px solid #363a4f;
        border-radius: 6px;
        padding: 8px;
        font-size: 13px;
        selection-background-color: #363a4f;
    }
    QTextEdit#siteURL_text, QTextEdit#site_text {
        min-height: 30px;
        max-height: 30px;
    }
    QScrollArea {
        background-color: #24273a;
        border: 1px solid #363a4f;
        border-radius: 6px;
    }
    QComboBox {
        background-color: #363a4f;
        color: #cad3f5;
        border: 1px solid #494d64;
        border-radius: 6px;
        padding: 6px 12px;
        font-size: 13px;
    }
    QComboBox:hover {
        border: 1px solid #5b6078;
    }
    QComboBox::drop-down {
        border: none;
        background: #363a4f;
        border-radius: 6px;
    }
    QComboBox QAbstractItemView {
        background-color: #24273a;
        color: #cad3f5;
        selection-background-color: #363a4f;
        border: 1px solid #494d64;
    }
    QScrollBar:vertical {
        background: #1e2030;
        width: 10px;
        margin: 0;
        border-radius: 5px;
    }
    QScrollBar::handle:vertical {
        background: #494d64;
        min-height: 30px;
        border-radius: 5px;
    }
    QScrollBar::handle:vertical:hover {
        background: #5b6078;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
        height: 0;
    }
    QScrollBar:horizontal {
        background: #1e2030;
        height: 10px;
        margin: 0;
        border-radius: 5px;
    }
    QScrollBar::handle:horizontal {
        background: #494d64;
        min-width: 30px;
        border-radius: 5px;
    }
    QScrollBar::handle:horizontal:hover {
        background: #5b6078;
    }
    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
        width: 0;
    }
    QMessageBox {
        background-color: #24273a;
        color: #cad3f5;
    }
    QMessageBox QLabel {
        color: #cad3f5;
    }
    QMessageBox QPushButton {
        min-width: 80px;
    }
    QRadioButton {
        color: #cad3f5;
        spacing: 8px;
        font-size: 13px;
    }
    QRadioButton::indicator {
        width: 18px;
        height: 18px;
        border: 2px solid #5b6078;
        border-radius: 10px;
        background: #24273a;
    }
    QRadioButton::indicator:checked {
        background: #8aadf4;
        border: 2px solid #8aadf4;
    }
    QRadioButton::indicator:hover {
        border: 2px solid #8aadf4;
    }
    QCheckBox {
        color: #cad3f5;
        spacing: 8px;
        font-size: 13px;
    }
    QCheckBox::indicator {
        width: 18px;
        height: 18px;
        border: 2px solid #5b6078;
        border-radius: 4px;
        background: #24273a;
    }
    QCheckBox::indicator:checked {
        background: #8aadf4;
        border: 2px solid #8aadf4;
    }
    QCheckBox::indicator:hover {
        border: 2px solid #8aadf4;
    }
    QMenuBar {
        background-color: #1e2030;
        color: #cad3f5;
        border-bottom: 1px solid #363a4f;
    }
    QMenuBar::item:selected {
        background: #363a4f;
    }
    QStatusBar {
        background-color: #1e2030;
        color: #6e738d;
        border-top: 1px solid #363a4f;
    }
)";

void settings::writeToConfig(){

    QFile configFile(configPath());
    QJsonObject obj;
    int theme = 1;
    if(ui->default_radio->isChecked()){
        theme = 0;
    }
    if(ui->debugMode->isChecked()){
        debug = 1;
    }

    obj["theme"] = theme;
    obj["encryptfilenames"] = ui->encryptFileName->isChecked();
    obj["debug"] = ui->debugMode->isChecked();

    QByteArray jsonData = QJsonDocument(obj).toJson();

    QProcess process;
    QStringList args;
    args << "enc"
         << "-aes-256-cbc"
         << "-K" << aes_key.toHex()
         << "-iv" << iv.toHex();
    process.start("openssl", args);
    if (!process.waitForStarted())
        return;
    process.write(jsonData);
    process.closeWriteChannel();
    if (!process.waitForFinished())
        return;

    QByteArray encrypted = process.readAllStandardOutput();

    if (configFile.open(QIODevice::WriteOnly))
        configFile.write(encrypted);
}

settings::settings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::settings)
{
    ui->setupUi(this);

    QButtonGroup* group = new QButtonGroup(this);
    group->addButton(ui->default_radio);
    group->addButton(ui->catppuccin_radio);

    ui->encryptFileName->setChecked(encryptFileNames);       // horrible naming
    ui->debugMode->setChecked(debug);                        // less-horrible naming
    connect(ui->back_button, &QPushButton::clicked, this, [this](){
        writeToConfig();
        emit settingsFinished();
    });

    connect(ui->encryptFileName, &QCheckBox::clicked, this, &settings::checkBoxEncryptedFileNames);
}

void settings::setEncryptFileNamesBox(bool state){
    ui->encryptFileName->setChecked(state);
}

void settings::setDebugBox(bool state){
    ui->debugMode->setChecked(state);
}

void settings::checkBoxEncryptedFileNames()
{
    bool isChecked = ui->encryptFileName->isChecked();
    encryptFileNames = isChecked;

}
settings::~settings()
{
    delete ui;
}
