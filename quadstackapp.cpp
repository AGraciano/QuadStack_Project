#include "quadstackapp.h"
#include "ui_QuadStackapp.h"
#include "ui/scenewindow.h"
#include <QFileDialog>

#include <iostream>

QuadStackApp::QuadStackApp(QWidget *parent)
: QMainWindow(parent) {
	_ui.reset(new ui::QuadStackAppClass);
	_ui->setupUi(this);
	_glWindow.reset(new SceneWindow(statusBar(), dynamic_cast<QWindow*>(this)));
	_glWindow->setAnimating(true);
	

	QWidget *container = QWidget::createWindowContainer(_glWindow.get());
	container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setCentralWidget(container);
}

QuadStackApp::~QuadStackApp() {};