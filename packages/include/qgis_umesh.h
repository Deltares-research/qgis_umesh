#ifndef QGIS_UMESH_H
#define QGIS_UMESH_H

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <QObject>
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

#include <qgsstatusbar.h>

//#include "MyEditTool.h"
#include "handle_vector_layers.h"
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
        static const QString s_ident, s_name, s_description, s_category, s_version, s_icon, s_plugin_version;
        static const QgisPlugin::PluginType s_plugin_type;

        qgis_umesh(QgisInterface* iface);
        ~qgis_umesh();
        void initGui();
        void unload();

        //  qgis_umesh functions
        char* stripSpaces(char *);

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

        HISCF * get_active_his_cf_file(QString);
        void set_show_map_output();
        void start_plotcfts();
        void dummy_slot();
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
        QgsCoordinateReferenceSystem m_crs;

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

        //MyEditTool * mMyEditTool;
        MyCanvas * mMyCanvas;

        char * msgtxt = (char *)malloc(MSG_LENGTH * sizeof(char *));
        char * pluginsHome = (char *)malloc(PATH_LENGTH * sizeof(char *));
        char * actionIcon_c = (char *)malloc(PATH_LENGTH * sizeof(char *));

        int _fil_index;
        std::vector<UGRID *> m_ugrid_file;
        int _his_cf_fil_index;
        std::vector<HISCF *> m_his_cf_file;
        int _mdu_fil_index;
        std::vector<JSON_READER *> m_mdu_files;
        HVL* m_hvl = nullptr; 
};
#endif  // QGIS_UMESH_H
