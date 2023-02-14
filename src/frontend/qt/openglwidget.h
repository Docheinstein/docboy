#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include "QOpenGLWidget"

class OpenGLWidget : public QOpenGLWidget {
Q_OBJECT

public:
	explicit OpenGLWidget(QWidget* parent = nullptr);


protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

};

#endif // OPENGLWIDGET_H