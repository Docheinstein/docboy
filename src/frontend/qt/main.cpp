#include "core/core.h"
#include "utils/log.h"
#include "window.h"
#include <QApplication>
#include <iostream>

int main(int argc, char** argv) {
    Core core;
    QApplication a(argc, argv);
    Window w(core);
    w.show();
    return a.exec();
}
