#include "window.h"
#include "core/core.h"
#include "ui_window.h"
#include "utils/log.h"
#include <QFileDialog>

Window::Window(Core& core, QWidget* parent) :
    QMainWindow(parent),
    core(core),
    ui(new Ui::Window) {
    ui->setupUi(this);
    connect(ui->actionLoadROM, &QAction::triggered, this, &Window::onLoadROM);
}

void Window::onLoadROM() {
    QString result =
        QFileDialog::getOpenFileName(this, tr("Select ROM"), QDir::currentPath(), tr("GameBoy ROMs (*.gb)"));
    if (result.isEmpty())
        return;

    core.loadROM(result.toStdString());
    core.start();
}

Window::~Window() {
    delete ui;
}
