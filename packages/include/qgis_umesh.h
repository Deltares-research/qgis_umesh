#ifndef QGIS_UMESH_H
#define QGIS_UMESH_H

#include "QObject"
#include <iostream>

#include <QAction>
#include <QComboBox>
#include <QDesktopWidget>
#include <QIcon>
#include <QLayout> 
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QProgressBar>
#include <QSizePolicy> 
#include <QSpacerItem> 
#include <QToolBar>
#include <QTreeWidget>
#include <QtMath>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>

#include <qgsapplication.h>
#include <qgsgeometry.h>
#include <qgslayertree.h>
#include <qgslayertreegroup.h>
#include <QgsLayerTreeNode.h>
#include <QgsLayerTreeView.h>
#include <qgsmapcanvas.h>
#include <qgsmaplayer.h>
#include <qgsvectorlayer.h>
#include <qgisplugin.h>
#include <qgisinterface.h>
#include <qgscoordinatereferencesystem.h>
#include <qgsstatusbar.h>
#include <qgslinestring.h>
#include <qgsmultilinestring.h>
#include <qgslinesymbollayer.h>
#include <qgsmarkersymbollayer.h>
#include <qgsabstract3drenderer.h>
#include <QgsSingleSymbolRenderer.h>
//#include <QgsLineSymbolLayer>
#include <QgsMarkerSymbolLayer.h>
#include <qgsproject.h>
#include <qgssymbol.h>
#include <qgsvectorlayer.h>

#include "MyEditTool.h"
#include "MyDrawingCanvas.h"
#include "netcdf.h"
#include "ugrid.h"
#include "his_cf.h"
#include "map_time_manager_window.h"
#include "map_property_window.h"

#include <direct.h> // for getcwd
#include <stdlib.h> // for MAX_PATH

#define MSG_LENGTH 101
#define PATH_LENGTH 1024

class QAction;
class qgis_umesh
    : public QObject, public QgisPlugin

{
    Q_OBJECT

    public:
        qgis_umesh(QgisInterface* iface);
        ~qgis_umesh();
        void initGui();
        void unload();

        //  qgis_umesh functions
        char* stripSpaces(char *);
        void create_data_on_edges_vector_layer(_variable * var, struct _feature *, struct _edge *, vector<double *>, long, QgsLayerTreeGroup *);
        void create_data_on_nodes_vector_layer(_variable * var, struct _feature *, vector<double *>, long, QgsLayerTreeGroup *);
        void create_nodes_vector_layer(QString, struct _feature *, long, QgsLayerTreeGroup *);
        void create_edges_vector_layer(QString, struct _feature *, struct _edge *, long, QgsLayerTreeGroup *);
        void create_geometry_vector_layer(QString, struct _ntw_geom *, long, QgsLayerTreeGroup *);
        void create_observation_point_vector_layer(QString, _location_type *, long, QgsLayerTreeGroup *);
        void create_observation_polyline_vector_layer(QString, _location_type *, long, QgsLayerTreeGroup *);

        void add_layer_to_group(QgsVectorLayer *, QgsLayerTreeGroup *);
        void show_map_output(UGRID *);

        
    private slots:
        void openFile();
        void openFile(QFileInfo);
        void open_file_his_cf();
        void open_file_his_cf(QFileInfo);
        void set_enabled();
        void about();
        void activate_layers();
        void activate_observation_layers();

        UGRID * get_active_ugrid_file(QString);
        HISCF * get_active_his_cf_file(QString);
        void set_show_map_output();
        void start_plotcfts();
        void dummy_slot();
        void CrsChanged();
        void mapPropertyWindow();
        void onWillRemoveChildren(QgsLayerTreeNode *, int, int);

    private:
        // windows
        MapTimeManagerWindow * mtm_widget = NULL;

        // functions
        void unload_vector_layers();
        QIcon get_icon_file(QDir, QString);

        // variables
        QgisInterface * mQGisIface; // Pointer to the QGIS interface object
        QgsMapCanvas  * mCanvas; // Pointer to the QGIS canvas
        QgsStatusBar * status_bar;

        QAction * mainAction;
        QAction * openAction;
        QAction * open_action_his_cf;
        QAction * inspectAction;
        QAction * plotcftsAction;
        QAction * mapoutputAction;
        QAction * aboutAction;
        QMenu * fileMenu;

        QToolBar * tbar;  // complete toolbar for plugin (ie menu and icons)
        QToolBar * _menuToolBar;
        QToolBar * _iconToolBar;

        QMenuBar * menu_bar;

        QIcon * icon_picture;
        QIcon icon_open;
        QIcon icon_open_his_cf;
        QIcon icon_inspect;
        QIcon icon_plotcfts;
        QIcon icon_mapoutput;
        QDir current_dir;
        QDir executable_dir;
        QProgressBar * pgBar;

        MyEditTool * mMyEditTool;
        MyCanvas * mMyCanvas;

        char * msgtxt = (char *)malloc(MSG_LENGTH * sizeof(char *));
        char * pluginsHome = (char *)malloc(PATH_LENGTH * sizeof(char *));
        char * actionIcon = (char *)malloc(PATH_LENGTH * sizeof(char *));

        int _fil_index;
        vector<UGRID *> _UgridFiles;
        int _his_cf_fil_index;
        vector<HISCF *> _his_cf_files;

        QgsCoordinateReferenceSystem m_crs;
};
#endif  // QGIS_UMESH_H