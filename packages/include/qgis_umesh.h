#ifndef QGIS_UMESH_H
#define QGIS_UMESH_H

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

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
#include <qgsmaptool.h>
#include <qgsmessagelog.h>
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
#include <QgsGraduatedSymbolRenderer.h>
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
#include "json_reader.h"
#include "edit_observation_points_window.h"
#include "map_time_manager_window.h"
#include "map_property_window.h"

#include <direct.h> // for getcwd
#include <stdlib.h> // for MAX_PATH

#define MSG_LENGTH 101
#define PATH_LENGTH 1024

#define    WAIT_MODE 1
#define NO_WAIT_MODE 0

class QAction;
class UGRID;

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
        void create_vector_layer_data_on_edges(QString, _variable * var, struct _feature *, struct _edge *, double *, long, QgsLayerTreeGroup *);
        void create_vector_layer_edge_type(QString, _variable * var, struct _feature *, struct _edge *, double *, long, QgsLayerTreeGroup *);
        void create_vector_layer_data_on_nodes(QString, _variable * var, struct _feature *, double *, long, QgsLayerTreeGroup *);
        void create_vector_layer_nodes(QString, QString, struct _feature *, long, QgsLayerTreeGroup *);
        void create_vector_layer_edges(QString, QString, struct _feature *, struct _edge *, long, QgsLayerTreeGroup *);
        void create_vector_layer_geometry(QString, QString, struct _ntw_geom *, long, QgsLayerTreeGroup *);
        void create_vector_layer_observation_point(QString, QString, _location_type *, long, QgsLayerTreeGroup *);
        void create_vector_layer_observation_polyline(QString, QString, _location_type *, long, QgsLayerTreeGroup *);

        // Reading input files (ie JSON format)
        void create_vector_layer_1D_external_forcing(UGRID *, JSON_READER *, long, QgsLayerTreeGroup *);
        void create_vector_layer_1D_structure(UGRID *, JSON_READER *, long, QgsLayerTreeGroup *);
        void create_vector_layer_chainage_observation_point(UGRID *, JSON_READER *, long, QgsLayerTreeGroup *);
        void create_vector_layer_crs_observation_point(UGRID *, JSON_READER *, long, QgsLayerTreeGroup *);
        void create_vector_layer_fixed_weir(UGRID *, JSON_READER *, long, QgsLayerTreeGroup *);
        void create_vector_layer_thin_dams(UGRID *, JSON_READER *, long, QgsLayerTreeGroup *);
        void create_vector_layer_observation_point(UGRID *, JSON_READER *, long, QgsLayerTreeGroup *);
        void create_vector_layer_observation_cross_section(UGRID*, JSON_READER*, long, QgsLayerTreeGroup*);
        void create_vector_layer_1D_observation_cross_section(UGRID*, JSON_READER*, long, QgsLayerTreeGroup*);
        void create_vector_layer_2D_observation_cross_section(UGRID*, JSON_READER*, long, QgsLayerTreeGroup*);
        void create_vector_layer_structure(UGRID *, JSON_READER *, long, QgsLayerTreeGroup *);
        void create_vector_layer_drypoints(UGRID*, JSON_READER*, long, QgsLayerTreeGroup*);
        void create_vector_layer_1D_cross_section(UGRID *, JSON_READER *, long, QgsLayerTreeGroup *);
        void create_vector_layer_1D_retention(UGRID*, JSON_READER*, long, QgsLayerTreeGroup*);
        void create_vector_layer_sample_point(UGRID *, JSON_READER *, long, QgsLayerTreeGroup *);

        void create_1D2D_link_vector_layer(JSON_READER *, long);
        long compute_location_along_geometry(struct _ntw_geom *, struct _ntw_edges *, string, double, double *, double *, double *);
        long find_location_boundary(struct _ntw_nodes *, string, double *, double *);


        QgsLayerTreeGroup * get_subgroup(QgsLayerTreeGroup *, QString);
        void add_layer_to_group(QgsVectorLayer *, QgsLayerTreeGroup *);
        void show_map_output(UGRID *);
        void edit_1d_obs_points();
        void experiment();

        
    private slots:
        void openFile();
        void openFile(QFileInfo);
        void open_file_his_cf();
        void open_file_his_cf(QFileInfo);
        void open_file_mdu();
        void open_file_mdu(QFileInfo);
        void open_file_link1d2d_json();
        void open_file_link1d2d_json(QFileInfo);
        void open_file_obs_point_json();
        void open_file_obs_point_json(QFileInfo);
        void set_enabled();
        void about();
        void activate_layers();
        void activate_observation_layers();
        void ShowUserManual();

        UGRID * get_active_ugrid_file(QString);
        HISCF * get_active_his_cf_file(QString);
        void set_show_map_output();
        void start_plotcfts();
        void dummy_slot();
        void CrsChanged();
        void mapPropertyWindow();
        void onWillRemoveChildren(QgsLayerTreeNode *, int, int);
        void onRemovedChildren(QString);

    private:
        // windows
        MapTimeManagerWindow * mtm_widget = nullptr;

        // functions
        void unload_vector_layers();
        QIcon get_icon_file(QDir, QString);
        int QT_SpawnProcess(int, char *, char **);
        std::vector<std::string> tokenize(const std::string &, const char);
        std::vector<std::string> tokenize(const std::string &, std::size_t);

        // variables
        QgisInterface * mQGisIface; // Pointer to the QGIS interface object
        QgsMapCanvas  * mCanvas; // Pointer to the QGIS canvas
        QgsStatusBar * status_bar;

        QAction * mainAction;
        QAction * open_action_map;
        QAction * open_action_his_cf;
        QAction * open_action_mdu;
        QAction * open_action_link1d2d_json;
        QAction * open_action_obs_point_json;
        QAction * edit_action_1d_obs_points;
        QAction * trial_experiment;
        QAction * inspectAction;
        QAction * plotcftsAction;
        QAction * mapoutputAction;
        QAction * showUserManualAct;
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
        QIcon icon_edit_1d_obs_points;
        QIcon icon_experiment;
        QIcon icon_plotcfts;
        QIcon icon_mapoutput;
        QDir current_dir;
        QDir executable_dir;
        QString m_working_dir;
        QProgressBar * pgBar;

        MyEditTool * mMyEditTool;
        MyCanvas * mMyCanvas;

        char * msgtxt = (char *)malloc(MSG_LENGTH * sizeof(char *));
        char * pluginsHome = (char *)malloc(PATH_LENGTH * sizeof(char *));
        char * actionIcon_c = (char *)malloc(PATH_LENGTH * sizeof(char *));

        int _fil_index;
        vector<UGRID *> m_ugrid_file;
        int _his_cf_fil_index;
        vector<HISCF *> m_his_cf_file;
        int _mdu_fil_index;
        vector<JSON_READER *> _mdu_files;

        QgsCoordinateReferenceSystem m_crs;
};
#endif  // QGIS_UMESH_H