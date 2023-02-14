#include "openglwidget.h"
#include <iostream>
#include "log/log.h"
#include <QOpenGLFunctions>

OpenGLWidget::OpenGLWidget(QWidget *parent) {

}

void OpenGLWidget::initializeGL() {
    DEBUG(1) << "initializeGL" << std::endl;
}

void OpenGLWidget::resizeGL(int w, int h) {

}

void OpenGLWidget::paintGL() {
    QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
    DEBUG(2) << "paintGL" << std::endl;
    gl->glClearColor(1, 0.1, 0.1, 1);
    gl->glClear(GL_COLOR_BUFFER_BIT);
//    update();
}
