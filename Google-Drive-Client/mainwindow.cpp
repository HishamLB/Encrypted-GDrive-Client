#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "setup.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto *setupPage = new setup(this);
    ui->stackedWidget->addWidget(setupPage);

    connect(ui->setup_button, &QPushButton::clicked, this, &MainWindow::showSetupPage);
    connect(ui->upload_button, &QPushButton::clicked, this, &MainWindow::upload);
    //connect(setupPage, &setup::finished, this, &MainWindow::showMainPage);
}

void MainWindow::upload()
{
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
