#include <iostream>
#include <QApplication>
#include "window.h"
#include "utils/log.h"
#include "core/core.h"

int main(int argc, char **argv) {
    Core core;
    QApplication a(argc, argv);
    Window w(core);
    w.show();
    return a.exec();
}
