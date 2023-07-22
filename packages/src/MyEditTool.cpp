//
// Copied from MyCanvas.cpp, project mfe_qgis dd 2019-05-19
//

#include <stdlib.h>
#include <string.h>

#define DLL_EXPORT
#include "MyEditTool.h"

#include "qgsmapcanvas.h"
#include "qgsmapcanvasmap.h"
//#include "qgscursors.h"
#include "qgsmaptopixel.h"
#include "qgsrubberband.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsmaptool.h"
#include "qgspoint.h"
//janm #include "qgsapplication.h"

#if defined(WIN32) || defined(WIN64)
#  include <windows.h>
#  define strdup _strdup
#endif

#define DRAW_CACHES false

//
// caches
// 0: drawing cache
// 5, 4, 3, 2, 1: stapled caches
// 6 contains the background picture
//
//-----------------------------------------------------------------------------
//
MyEditTool::MyEditTool(QgsMapCanvas* mapCanvas) :
    QgsMapTool( mapCanvas ),
    QgsMapCanvasItem( mapCanvas )
{
    //janm QgsMapTool::setCursor(QgsApplication::getThemeCursor(QgsApplication::Cursor::CrossHair));
    //QgsMapTool::setCursor(QgsApplication::getThemeCursor(QgsApplication::Cursor::CapturePoint));

    mMapCanvas = mapCanvas;
    mMapCanvasItem = mapCanvas;

    //
    // Render events
    // This calls the renderer everytime the canvas has drawn itself
    //
    connect(mMapCanvas, SIGNAL(renderComplete(QPainter *)), this, SLOT(renderCompletePlugin(QPainter *)));
    //
    // Key events
    //
    connect(mMapCanvas, SIGNAL(keyPressed(QKeyEvent *)), this, SLOT(MyKeyPressEvent(QKeyEvent *)));
    connect(mMapCanvas, SIGNAL(keyReleased(QKeyEvent *)), this, SLOT(MyKeyReleasedEvent(QKeyEvent *)));
    //
    // Mouse events
    //
    connect(this, SIGNAL(MouseDoubleClickEvent(QMouseEvent *)), this, SLOT(MyMouseDoubleClickEvent(QMouseEvent *)));
    connect(this, SIGNAL(MouseMoveEvent(QMouseEvent *)), this, SLOT(MyMouseMoveEvent(QMouseEvent *)));
    connect(this, SIGNAL(MousePressEvent(QMouseEvent *)), this, SLOT(MyMousePressEvent(QMouseEvent *)));
    connect(this, SIGNAL(MouseReleaseEvent(QMouseEvent *)), this, SLOT(MyMouseReleaseEvent(QMouseEvent *)));
    connect(this, SIGNAL(WheelEvent(QWheelEvent * we)), this, SLOT(MyWheelEvent(QWheelEvent * we)));

}
//
//-----------------------------------------------------------------------------
//
MyEditTool::~MyEditTool()
{
}


//
//-----------------------------------------------------------------------------
//
void MyEditTool::paint(QPainter * p)
{
    Q_UNUSED(p);
}

//
//-----------------------------------------------------------------------------
//
void MyEditTool::canvasDoubleClickEvent(QgsMapMouseEvent * me )
{
    emit MouseDoubleClickEvent( me );
}
void MyEditTool::canvasMoveEvent(QgsMapMouseEvent * me )
{
    //QMessageBox::warning(0, "Message", QString(tr("MyEditTool::canvasMoveEvent: %1").arg(me->button())));
    emit MouseMoveEvent( me );
}
void MyEditTool::canvasPressEvent(QgsMapMouseEvent * me )
{
    emit MousePressEvent( me );
}
void MyEditTool::canvasReleaseEvent(QgsMapMouseEvent * me )
{
    //QMessageBox::warning(0, "Message", QString(tr("MyEditTool::canvasReleaseEvent: %1").arg(me->button())));
    emit MouseReleaseEvent( me );
}
void MyEditTool::wheelEvent( QWheelEvent * we )
{
    //QMessageBox::warning(0, "Message", QString(tr("MyEditTool::wheelEvent")));
    mMapCanvas->refresh();
    mMapCanvas->update();
    emit WheelEvent( we );
}
//
//-----------------------------------------------------------------------------
// ********* Events a widget must handle or forward to plugin **********
//-----------------------------------------------------------------------------
//
//

void MyEditTool::MyWheelEvent ( QWheelEvent * we )
{
    //QMessageBox::warning( 0, "Message", QString(tr("MyEditTool::MyWheelEvent")));
    Q_UNUSED(we);
    mMapCanvas->update();
}

//
//-----------------------------------------------------------------------------
//
void MyEditTool::MyMouseDoubleClickEvent( QMouseEvent * me)
{
    QMessageBox::warning( 0, "Message", QString(tr("MyEditTool::mouseDoubleClickEvent: %1").arg(me->button())));
}
//
//-----------------------------------------------------------------------------
//
void MyEditTool::MyMouseMoveEvent      ( QMouseEvent * me)
{
    //QMessageBox::warning(0, "Message", QString(tr("MyEditTool::MyMouseMoveEvent: %1").arg(me->button())));
    Q_UNUSED(me);
}
//
//-----------------------------------------------------------------------------
//
void MyEditTool::MyMousePressEvent     ( QMouseEvent * me)
{
    QMessageBox::warning(0, "Message", QString(tr("MyEditTool::MyMousePressEvent: %1").arg(me->button())));
}
//
//-----------------------------------------------------------------------------
//
void MyEditTool::MyMouseReleaseEvent   (QMouseEvent * me)
{
    QMessageBox::warning(0, "Message", QString(tr("MyEditTool::MyMouseReleaseEvent: %1").arg(me->button())));
}
//
//-----------------------------------------------------------------------------
//
void MyEditTool::MyKeyPressEvent( QKeyEvent* ke)
{
    if (ke->modifiers() & Qt::ShiftModifier)
    {
        int pressed_key = ke->key() & Qt::ShiftModifier;
        Q_UNUSED(pressed_key);
        {
            QMessageBox::warning(0, "Message", QString(tr("MyEditTool::MyKeyPressEvent: %1").arg(ke->key())));
        }
    }

    //QMessageBox::warning(0, "Message", QString(tr("MyEditTool::MyKeyPressEvent: %1").arg(ke->key())));
}
//
//-----------------------------------------------------------------------------
//
void MyEditTool::MyKeyReleasedEvent( QKeyEvent * ke)
{
    //QMessageBox::warning(0, "Message", QString(tr("MyEditTool::MyKeyReleasedEvent: %1").arg(ke->key())));
    Q_UNUSED(ke);
}
