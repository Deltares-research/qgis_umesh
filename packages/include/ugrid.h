#ifndef __UGRID_H__
#define __UGRID_H__

#include <QFileInfo>
#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QDate>
#include <QtCore/QFileInfo>
#include <QtCore/QTime>
#include <QtCore/qtextcodec.h>
#include <QtCore/qdebug.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>

#include <QtGui/QIcon>

#ifdef NATIVE_C
#else
#include <qgsmessagelog.h>
#endif

//#include <boost/timer/timer.hpp>
#include "data_struct.h"
#include "grid.h"
#include "netcdf.h"


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

struct _faces {
    size_t count;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<std::vector<double>> x_bounds;  // polygon used when drawing quantities, not necessarily the total face
    std::vector<std::vector<double>> y_bounds;  // polygon used when drawing quantities, not necessarily the total face
};



class UGRID
{
public:
#ifdef NATIVE_C
    UGRID();
    char * get_filename();
#else
    UGRID();
    UGRID(QFileInfo filename, int, QProgressBar* pgBar);
    QFileInfo get_filename();
#endif
    ~UGRID();
    long open(QFileInfo, QProgressBar* );
    long read();
    long read_global_attributes();
    long get_global_attribute_value(std::string, std::string*);

    long read_mesh();
    long read_times();
    long read_variables();
    struct _mesh1d_string ** get_mesh1d_string();
    struct _mesh2d_string ** get_mesh2d_string();
    struct _mesh_contact_string ** get_mesh_contact_string();

    struct _global_attributes * get_global_attributes(void);

    struct _mesh_contact * get_mesh_contact();
    struct _ntw_nodes * get_connection_nodes();
    struct _ntw_edges * get_network_edges();
    struct _ntw_geom * get_network_geometry();
    struct _mesh1d * get_mesh_1d();
    struct _mesh2d * get_mesh_2d();
    struct _mapping * get_grid_mapping();

    long set_grid_mapping_epsg(long, std::string);
    std::vector<std::string> get_names(int, std::string, size_t);

    std::vector<std::string> tokenize(const std::string & , const char );
    std::vector<std::string> tokenize(const std::string & , std::size_t );

    long get_count_times();
    QDateTime * m_RefDate;
    std::vector<double> get_times();
    QVector<QDateTime> get_qdt_times();  // qdt: Qt Date Time
    QVector<QDateTime> qdt_times;

    struct _mesh_variable * get_variables();
    DataValuesProvider2D<double> get_variable_values(const std::string);
    DataValuesProvider2D<double> get_variable_values(const std::string, int);

    DataValuesProvider3D<double> get_variable_3d_values(const std::string);
    DataValuesProvider4D<double> get_variable_4d_values(const std::string);

    struct _variable * get_var_by_std_name(struct _mesh_variable *, std::string, std::string);

#if defined (DEBUG)
    char  janm;
#endif

private:
    char * strndup(const char *, size_t);
    int read_network_attributes(struct _ntw_string *, int, std::string, size_t);
    int read_geometry_attributes(struct _geom_string *, int, std::string, int);
    int read_mesh1d_attributes(struct _mesh1d_string *, int, std::string, int);
    int read_mesh2d_attributes(struct _mesh2d_string *, int, std::string, int);
    int read_composite_mesh_attributes(struct _mesh_contact_string *, int, std::string);

    int get_attribute_by_var_name(int, std::string, std::string, std::string *);
    int get_attribute(int, int, char *, char **);
    int get_attribute(int, int, char *, std::string *);
    int get_attribute(int, int, std::string, std::string *);
    int get_attribute(int, int, char *, double *);
    int get_attribute(int, int, char *, int *);
    int get_attribute(int, int, char *, long *);
    int get_dimension(int, char *, size_t *);
    int get_dimension(int, std::string, size_t *);
    int get_dimension_var(int, std::string, size_t *);
    std::vector<std::string> get_string_var(int, std::string);
    std::vector<std::string> get_dimension_names(int, std::string);
    int get_coordinate(char *, char *, int, double *, double *);

    int read_variables_with_cf_role(int, std::string, std::string, int, int *);
    int read_grid_mapping(int, std::string, std::string);
    int determine_mesh1d_edge_length(struct _mesh1d*, struct _ntw_edges*);
    int create_mesh1d_nodes(struct _mesh1d *, struct _ntw_edges *, struct _ntw_geom *);
    int create_mesh_contacts(struct _ntw_nodes *, struct _ntw_edges *, struct _ntw_geom *);

    double * permute_array(double *, std::vector<long>, std::vector<long>);
    long get_index_in_c_array(long, long, long, long, long, long, long, long);

    struct _ntw_nodes * m_ntw_nodes = nullptr;
    struct _ntw_edges * m_ntw_edges = nullptr;
    struct _ntw_geom * m_ntw_geom = nullptr;
    struct _mesh_contact * m_mesh_contact= nullptr;
    struct _ntw_nodes * m_mesh1d_nodes = nullptr;
    struct _ntw_edges * m_mesh1d_edges = nullptr;
    struct _mesh1d * m_mesh1d = nullptr;
    struct _mesh2d * m_mesh2d = nullptr;
    struct _mapping * m_mapping = nullptr;
    struct _mesh_variable * m_mesh_vars = nullptr;
    struct _feature * m_nodes = nullptr;
    struct _ntw_string ** m_ntw_strings = nullptr;
    struct _geom_string ** m_geom_strings = nullptr;
    struct _mesh1d_string ** m_mesh1d_strings = nullptr;
    struct _mesh2d_string ** m_mesh2d_strings = nullptr;
    struct _mesh_contact_string ** m_mesh_contact_strings = nullptr;

    std::vector<_time_series> time_series;

    long m_nr_mesh_contacts;
    long m_nr_mesh_var;
    size_t _two;
    size_t * m_dimids;
    std::vector<std::string> m_dim_names;
    std::map<std::string, long> m_map_dim;
    std::map<std::string, std::string> m_map_dim_name;

    int m_ncid;
    int * topo_edge_nodes;
    int * mesh1d_edge_nodes;
    int * mesh2d_edge_nodes;
    int * contact_edge_nodes;
#ifdef NATIVE_C
    char * fname;
    char * ugrid_file_name;
#else
    QFileInfo m_fname;
    QString m_ugrid_file_name;
    QProgressBar * m_pgBar;
#endif
    struct _global_attributes * global_attributes;
};

#endif  // __UGRID_H__
