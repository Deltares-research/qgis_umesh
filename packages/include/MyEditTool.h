//
// Copied from MyCanvas.cpp, project mfe_qgis dd 2019-05-19
//

#ifndef _INC_MYEDITTOOL
#define _INC_MYEDITTOOL

#include <assert.h>

#define GUI_EXPORT __declspec(dllimport)
#include "qgisinterface.h"
#include "qgsmaptool.h"
#include "qgsmaptoolemitpoint.h"
#include "qgspointxy.h"
#include "qgsmapcanvas.h"
#include "qgsmapcanvasitem.h"
#include "qgsmapmouseevent.h"

#include <QtCore/QVector>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <QtGui/QColor>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>

#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QVBoxLayout>

#include "qgsmaptool.h"
#include "qgsrubberband.h"

#define NR_CACHES 1
#define IMAGE_WIDTH  3840 // 1420 // 
#define IMAGE_HEIGHT 2140 // 1080 // 

class MyEditTool : public QgsMapTool, public QgsMapCanvasItem
{
    Q_OBJECT

public:  // function containing the emit-statement
    void canvasDoubleClickEvent(QgsMapMouseEvent *);
    void canvasMoveEvent(QgsMapMouseEvent *);
    void canvasPressEvent(QgsMapMouseEvent *);
    void canvasReleaseEvent(QgsMapMouseEvent *);
    void wheelEvent(QWheelEvent *);

signals:  // functions send by the emit-statement
    void MouseDoubleClickEvent(QMouseEvent*);
    void MouseMoveEvent(QMouseEvent*);
    void MousePressEvent(QMouseEvent*);
    void MouseReleaseEvent(QgsMapMouseEvent *);
    void WheelEvent(QWheelEvent*);

private slots:
/********** Events a widget must handle or forward to plugin **********/
    void MyMouseDoubleClickEvent(QMouseEvent*);
    void MyMouseMoveEvent(QMouseEvent*);
    void MyMousePressEvent(QMouseEvent*);
    void MyMouseReleaseEvent(QMouseEvent*);
    void MyWheelEvent(QWheelEvent* e);

    void MyKeyPressEvent(QKeyEvent*);
    void MyKeyReleasedEvent(QKeyEvent*);

public:
    MyEditTool(QgsMapCanvas*);
    ~MyEditTool();

protected:
    void paint(QPainter * p);

private:
    // variables
    QgisInterface* mQGisIface;
    QPointer<QgsMapCanvas> mMapCanvas;
    QPointer<QgsMapCanvas> mMapCanvasItem;
    QPixmap * mMapCanvasMap;

    QPaintDevice * pd;
    QWidget * w;
    QVBoxLayout * vb;
    QLabel * label[NR_CACHES];
};

#endif  /* _INC_MYEDITTOOL */
