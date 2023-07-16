#include "openglwidget.h"
#include "utils/log.h"
#include <QOpenGLFunctions>
#include <iostream>

OpenGLWidget::OpenGLWidget(QWidget* parent) {
}

void OpenGLWidget::initializeGL() {
}

void OpenGLWidget::resizeGL(int w, int h) {
}

void OpenGLWidget::paintGL() {
    QOpenGLFunctions* gl = QOpenGLContext::currentContext()->functions();
    gl->glClearColor(1, 0.1, 0.1, 1);
    gl->glClear(GL_COLOR_BUFFER_BIT);
    //    update();
}
