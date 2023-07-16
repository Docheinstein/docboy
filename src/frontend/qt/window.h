#ifndef WINDOW_H
#define WINDOW_H

#include "core/core.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class Window;
}
QT_END_NAMESPACE

class Window : public QMainWindow {
    Q_OBJECT

public:
    explicit Window(Core& core, QWidget* parent = nullptr);
    ~Window() override;

private slots:
    void onLoadROM();

private:
    Core& core;
    Ui::Window* ui;
};
#endif // WINDOW_H
