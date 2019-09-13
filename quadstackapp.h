#ifndef STRING_GIS_APP_H
#define STRING_GIS_APP_H

#include <QMainWindow>
#include <memory>
#include "ui/scenewindow.h"

using std::unique_ptr;

namespace Ui {
	class QuadStackAppClass;
}

// Lower case namespaces to be consistent with our framework
namespace ui = Ui;

class QuadStackApp : public QMainWindow {

	Q_OBJECT

private:
	unique_ptr<ui::QuadStackAppClass> _ui;
	unique_ptr<SceneWindow> _glWindow;

public:
	QuadStackApp(QWidget *parent = nullptr);
	~QuadStackApp();
};

#endif