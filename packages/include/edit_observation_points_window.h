#ifndef _INC_EditObsPoints
#define _INC_EditObsPoints

#include <QObject>
#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QToolBar>
#include <QVBoxLayout>

#include <qmath.h>
#include <QgsLayerTreeView.h>
#include <qgsmapmouseevent.h>

#include <qgisplugin.h>

#include <direct.h> // for getcwd
#include <stdlib.h> // for MAX_PATH

#include "MyDrawingCanvas.h"

class EditObsPoints
    : public QDockWidget

{
    Q_OBJECT

    public:
        static int object_count;

    public slots:
        void cb_clicked(int);
    public slots:
        void MyMouseReleaseEvent(QgsMapMouseEvent *);

    public:
        EditObsPoints(QgsMapLayer *, QgsMapLayer *, GRID *, QgisInterface *);
        ~EditObsPoints();
        static int get_count();

    public:
        QDockWidget * edit_obs_panel;

    private:
        QgisInterface * m_QGisIface;
        QgsMapCanvas * m_QgsMapcanvas;
        QgsMapLayer * m_obs_layer;
        QgsMapLayer * m_geom_layer;
        GRID * m_grid_files;
        MyCanvas * m_MyCanvas;
        struct _ntw_geom * m_ntw_geom;

        void create_window();
        void closeEvent(QCloseEvent *);
};

#endif
