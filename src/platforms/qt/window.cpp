#include "window.h"
#include "ui_mainwindow.h"
#include "log/log.h"
#include "core/gible.h"
#include <QFileDialog>

Window::Window(Gible &gible, QWidget *parent)
    : QMainWindow(parent), gible(gible), ui(new Ui::Window) {
    ui->setupUi(this);
    connect(ui->actionLoadROM, &QAction::triggered, this, &Window::onLoadROM);
}

void Window::onLoadROM() {
    DEBUG(1) << "Selecting ROM..." << std::endl;
    QString result = QFileDialog::getOpenFileName(this, tr("Select ROM"), QDir::currentPath(), tr("GameBoy ROMs (*.gb)"));
    DEBUG(1) << "Selected ROM: " << result.toStdString() << std::endl;
    if (result.isEmpty())
        return;

    gible.loadROM(result.toStdString());
    gible.start();
}


Window::~Window() {
    delete ui;
}
