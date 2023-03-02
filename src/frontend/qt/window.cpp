#include "window.h"
#include "ui_window.h"
#include "utils/log.h"
#include "core/core.h"
#include <QFileDialog>

Window::Window(Core &core, QWidget *parent)
    : QMainWindow(parent), core(core), ui(new Ui::Window) {
    ui->setupUi(this);
    connect(ui->actionLoadROM, &QAction::triggered, this, &Window::onLoadROM);
}

void Window::onLoadROM() {
    DEBUG(1) << "Selecting ROM..." << std::endl;
    QString result = QFileDialog::getOpenFileName(this, tr("Select ROM"), QDir::currentPath(), tr("GameBoy ROMs (*.gb)"));
    DEBUG(1) << "Selected ROM: " << result.toStdString() << std::endl;
    if (result.isEmpty())
        return;

    core.loadROM(result.toStdString());
    core.start();
}


Window::~Window() {
    delete ui;
}
