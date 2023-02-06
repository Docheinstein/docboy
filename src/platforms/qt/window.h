#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
#include "core/gible.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Window; }
QT_END_NAMESPACE

class Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Window(Gible &gible, QWidget *parent = nullptr);
    ~Window() override;


private slots:
    void onLoadROM();

private:
    Gible &gible;
    Ui::Window *ui;
};
#endif // WINDOW_H
