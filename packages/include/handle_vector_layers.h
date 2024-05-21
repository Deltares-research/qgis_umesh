#ifndef HANDLE_VECTOR_LAYER_H
#define HANDLE_VECTOR_LAYER_H

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

#include <qgslayertree.h>
#include <QgsLayerTreeView.h>
#include <qgsvectorlayer.h>
#include <qgisinterface.h>
#include <qgsmultilinestring.h>
#include <qgslinesymbollayer.h>
#include <qgsmarkersymbol.h>
#include <qgsmarkersymbollayer.h>
#include <QgsSingleSymbolRenderer.h>
#include <qgssymbol.h>

#include "ugrid.h"
#include "his_cf.h"
#include "json_reader.h"
#include "perf_timer.h"

#include <direct.h> // for getcwd
#include <stdlib.h> // for MAX_PATH

#define MSG_LENGTH 101
#define PATH_LENGTH 1024

#define    WAIT_MODE 1
#define NO_WAIT_MODE 0

class QAction;
class UGRID;

class HVL
    : public QObject

{
    Q_OBJECT

    public:
        HVL(QgisInterface* iface);
        ~HVL();

        void set_ugrid_file(std::vector<UGRID *>);
        UGRID* get_active_ugrid_file(QString layer_id);

        //  qgis_umesh functions
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

        void create_vector_layer_1D2D_link(JSON_READER *, long);
        long compute_location_along_geometry(struct _ntw_geom *, struct _ntw_edges *, std::string, double, double *, double *, double *);
        long find_location_boundary(struct _ntw_nodes *, std::string, double *, double *);
        
        QgsLayerTreeGroup* get_subgroup(QgsLayerTreeGroup*, QString);
        void add_layer_to_group(QgsVectorLayer*, QgsLayerTreeGroup*);
        std::vector<std::string> tokenize(const std::string& s, char c);
        std::vector<std::string> tokenize(const std::string& s, std::size_t count);

    private slots:

    private:
        void CrsChanged();

        QgisInterface* m_QGisIface;
        std::vector<UGRID*> m_ugrid_file;
};
#endif  // HANDLE_VECTOR_LAYER_H
