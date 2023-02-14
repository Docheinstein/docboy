#include <iostream>
#include <QApplication>
#include "window.h"
#include "log/log.h"
#include "core/gible.h"

int main(int argc, char **argv) {
    Gible gible;
    QApplication a(argc, argv);
    Window w(gible);
    w.show();
    return a.exec();
}
