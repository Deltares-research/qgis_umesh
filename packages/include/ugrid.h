#ifndef UGRID_H
#define UGRID_H

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

//#include <boost/timer/timer.hpp>
#include "data_struct.h"
#include "netcdf.h"

using namespace std;
template<typename T>
struct DataValuesProvider2D
{
public:
    DataValuesProvider2D() {};
    T* m_arrayPtr = nullptr;
    int m_numTimes = 0;
    int m_numXY = 0;

    // copy the pointer and values
    DataValuesProvider2D(T* arrayPtr, int numTimes, int numXY) :
        m_arrayPtr(arrayPtr),
        m_numTimes(numTimes),
        m_numXY(numXY)
    {
    }

    inline T* GetValueAtIndex(int timeIndex, int xyIndex)
    {
        int index = GetIndex(timeIndex, xyIndex);

        return &m_arrayPtr[index];
    }

    inline int GetIndex(int timeIndex, int xyIndex)
    {
        int index = m_numXY * timeIndex + xyIndex;
        return index;
    }
};

template<typename T>
struct DataValuesProvider3D
{
public:
    DataValuesProvider3D() {};
    T* m_arrayPtr;
    int m_numTimes;
    int m_numLayer;
    int m_numXY;

    // copy the pointer and values
    DataValuesProvider3D(T* arrayPtr, int numTimes, int numLayer, int numXY) :
        m_arrayPtr(arrayPtr),
        m_numTimes(numTimes),
        m_numLayer(numLayer),
        m_numXY(numXY)
    {
    }

    inline T* GetValueAtIndex(int timeIndex, int layerIndex, int xyIndex)
    {
        int index = GetIndex(timeIndex, layerIndex, xyIndex);

        return &m_arrayPtr[index];
    }

    inline int GetIndex(int timeIndex, int layerIndex, int xyIndex)
    {
        int index = m_numXY * m_numLayer * timeIndex + m_numXY * layerIndex + xyIndex;
        return index;
    }
};


struct _time_series {
    size_t nr_times;
    QString * long_name;
    QString * unit;
    QString * dim_name;
    vector<double> times;  // vector of seconds
    map<string, string> map_dim_name;
};


struct _variable {
public:
    _variable() {};
    double fill_value;
    vector<long> dims;
    nc_type nc_type;
    int topology_dimension;
    string var_name;
    string location;
    string mesh;
    string coordinates;
    string cell_methods;
    string standard_name;
    string long_name;
    string units;
    string grid_mapping;
    string comment;
    //
    bool read;
    bool time_series;
    vector<string> dim_names;
    int nr_layers;
    vector<double> layer_center;
    vector<double> layer_interface;
    DataValuesProvider2D<double> data_2d;
    DataValuesProvider3D<double> data_3d;
    vector<vector<vector <double *>>> z_3d;
    map<string, string> map_dim_name;
};
struct _feature {
    size_t count;
    vector<long> branch;  // 1D, on which branch the node is
    vector<double> chainage;  // 1D, chainage of the node
    vector<double> x;
    vector<double> y;
    vector<string> name;
    vector<string> long_name;
};
////////////////////////////////////////////////////////////////////////////////
struct _mesh_variable {
    long nr_vars;
    struct _variable ** variable;
};

struct _mesh2d {
    long nr_mesh2d;
    struct _feature ** node;
    struct _edge ** edge;
    struct _feature ** face;  // face: the location of the mass centre
    vector<vector<int>> face_nodes;  // face: the location of the mass centre
};

struct _mesh1d {
    long nr_mesh1d;
    struct _feature ** node;
    struct _edge ** edge;
};

struct _mesh_contact {
    long nr_mesh_contact;
    string mesh_a;  // long_name of the first mesh
    string mesh_b;  // long_name of the second mesh
    string location_a;
    string location_b;
    struct _feature ** node;
    struct _edge ** edge;
};
////////////////////////////////////////////////////////////////////////////////
struct _ntw_geom {
    size_t nr_ntw;
    struct _geom ** geom;
};
struct _geom {
    size_t count;
    struct _feature ** nodes;
    vector<string> name;
    vector<string> long_name;
};

struct _ntw_edges {
    size_t nr_ntw;
    struct _edge ** edge;
};
struct _ntw_nodes {
    size_t nr_ntw;
    struct _feature ** node;
};

struct _edge {
    size_t count;
    vector<double> x;
    vector<double> y;
    vector<vector<double>> x_bounds;  // begin- and endpoint of edge when drawing quantities, not necessarily the begin an end point
    vector<vector<double>> y_bounds;  // begin- and endpoint of edge when drawing quantities, not necessarily the begin an end point
    int ** edge_nodes;
    vector<long> edge_branch;
    vector<double> edge_length;
    vector<string> name;
    vector<string> long_name;
};
struct _faces {
    size_t count;
    vector<double> x;
    vector<double> y;
    vector<vector<double>> x_bounds;  // polygon used when drawing quantities, not necessarily the total face
    vector<vector<double>> y_bounds;  // polygon used when drawing quantities, not necessarily the total face
};

struct _ntw_string {
    string var_name;
    string cf_role;
    string edge_dimension;
    string edge_geometry;
    string edge_length;
    string edge_node_connectivity;
    string long_name;
    string node_coordinates;
    size_t toplogy_dimension;
    //
    string x_ntw_name;
    string y_ntw_name;
    //
    string node_names;
    string node_long_names;
    string edge_names;
    string edge_long_names;
    string edge_order;
};
struct _geom_string {
    string var_name;
    string cf_role;
    string long_name;
    string node_count;
    string node_coordinates;
    //
    string x_geom_name;
    string y_geom_name;
    //
    string node_dimension;
};
struct _mesh1d_string {
    string var_name;
    string cf_role;
    string coordinate_space;
    string edge_node_connectivity;
    string long_name;
    string node_coordinates;
    string node_dimension;
    string node_names;
    string node_long_names;
    string edge_coordinates;  // 1D optional required
    string edge_names;
    string edge_long_names;
    size_t topology_dimension;
    //
    string node_branch;
    string node_chainage;
    string x_node_name;
    string y_node_name;
    //
    string edge_dimension;
    string edge_branch;
    string edge_chainage;
    string x_edge_name;  //  mid point of edge (in general)
    string y_edge_name;  //  mid point of edge (in general)
    string node_edge_exchange;
    //
    vector<string> dim_name;;  // in case of 2DV simualtion
};
struct _mesh2d_string {
    string var_name;
    string long_name;
    string edge_coordinates;  // 2D
    string x_edge_name;  // 2D
    string y_edge_name;  // 2D
    string face_coordinates;  // 2D
    string x_face_name;  // 2D
    string y_face_name;  // 2D
    string node_coordinates;  // 1D, 2D
    string x_node_name;  // 2D
    string y_node_name;  // 2D
    size_t topology_dimension;
    //
    string edge_dimension;
    string edge_geometry;
    string edge_length;
    string edge_face_connectivity;
    string edge_node_connectivity;
    string face_dimension;
    string face_edge_connectivity;
    string face_face_connectivity;
    string face_node_connectivity;
    string max_face_nodes_dimension;
    string node_edge_exchange;
    string layer_dimension;
    string layer_interface_dimension;
    //
    vector<string> dim_name;  // in case of 3D-sigma simualtion
    string x_bound_edge_name;
    string y_bound_edge_name;
    string x_bound_face_name;
    string y_bound_face_name;
};

struct _mesh_contact_string {
    string var_name;
    string meshes;
    string mesh_a;
    string mesh_b;
    string mesh_contact;
    //
    vector<string> dim_name;  // in case of 2DV + 3D-sigma simulation
};

class UGRID
{
public:
#ifdef NATIVE_C
    UGRID(char *);
    char * get_filename();
#else
    UGRID(QFileInfo, QProgressBar *);
    QFileInfo get_filename();
#endif
    ~UGRID();
    long read();
    long read_global_attributes();
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
    struct _mesh1d * get_mesh1d();
    struct _mesh2d * get_mesh2d();
    struct _mapping * get_grid_mapping();

    long set_grid_mapping_epsg(long, string);
    vector<string> get_names(int, string, size_t);

    std::vector<std::string> tokenize(const std::string & , const char );
    std::vector<std::string> tokenize(const std::string & , std::size_t );

    long get_count_times();
    QDateTime * RefDate;
    vector<double> get_times();
    QVector<QDateTime> get_qdt_times();  // qdt: Qt Date Time
    QVector<QDateTime> qdt_times;

    struct _mesh_variable * get_variables();
    DataValuesProvider2D<double> get_variable_values(const string);
    DataValuesProvider3D<double> get_variable_3d_values(const string);

    struct _variable * get_var_by_std_name(struct _mesh_variable *, string, string);

#if defined (DEBUG)
    char  janm;
#endif

private:
    char * strndup(const char *, size_t);
    int read_network_attributes(struct _ntw_string *, int, string, size_t);
    int read_geometry_attributes(struct _geom_string *, int, string, int);
    int read_mesh1d_attributes(struct _mesh1d_string *, int, string, int);
    int read_mesh2d_attributes(struct _mesh2d_string *, int, string, int);
    int read_composite_mesh_attributes(struct _mesh_contact_string *, int, string);

    int get_attribute_by_var_name(int, string, string, string *);
    int get_attribute(int, int, char *, char **);
    int get_attribute(int, int, char *, string *);
    int get_attribute(int, int, string, string *);
    int get_attribute(int, int, char *, double *);
    int get_attribute(int, int, char *, int *);
    int get_attribute(int, int, char *, long *);
    int get_dimension(int, char *, size_t *);
    int get_dimension(int, string, size_t *);
    int get_dimension_var(int, string, size_t *);
    vector<string>  get_dimension_names(int, string);
    int get_coordinate(char *, char *, int, double *, double *);

    int read_variables_with_cf_role(int, string, string, int, int *);
    int read_grid_mapping(int, string, string);
    int create_mesh1d_nodes(struct _mesh1d *, struct _ntw_edges *, struct _ntw_geom *);
    int create_mesh_contacts(struct _ntw_nodes *, struct _ntw_edges *, struct _ntw_geom *);

    struct _ntw_nodes * ntw_nodes = nullptr;
    struct _ntw_edges * ntw_edges = nullptr;
    struct _ntw_geom * ntw_geom = nullptr;
    struct _mesh_contact * mesh_contact= nullptr;
    struct _ntw_nodes * mesh1d_nodes = nullptr;
    struct _ntw_edges * mesh1d_edges = nullptr;
    struct _mesh1d * mesh1d = nullptr;
    struct _mesh2d * mesh2d = nullptr;
    struct _mapping * mapping = nullptr;
    struct _mesh_variable * mesh_vars = nullptr;
    struct _feature * nodes = nullptr;
    struct _ntw_string ** ntw_strings = nullptr;
    struct _geom_string ** geom_strings = nullptr;
    struct _mesh1d_string ** mesh1d_strings = nullptr;
    struct _mesh2d_string ** mesh2d_strings = nullptr;
    struct _mesh_contact_string ** mesh_contact_strings = nullptr;

    vector<_time_series> time_series;

    long m_nr_mesh_contacts;
    long m_nr_mesh_var;
    size_t _two;
    size_t * m_dimids;
    vector<string> m_dim_names;
    map<string, long> m_map_dim;
    map<string, string> m_map_dim_name;

    int ncid;
    int * topo_edge_nodes;
    int * mesh1d_edge_nodes;
    int * mesh2d_edge_nodes;
    int * contact_edge_nodes;
#ifdef NATIVE_C
    char * fname;
    char * ugrid_file_name;
#else
    QFileInfo fname;
    QString ugrid_file_name;
    QProgressBar * m_pgBar;
#endif
    struct _global_attributes * global_attributes;
};

#endif  // UGRID_H
