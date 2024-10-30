#ifndef __GRID_H__
#define __GRID_H__

//#include <QObject>
#include <QDateTime>
#include <QDate>
#include <QFileInfo>
#include <QTime>
#include <qtextcodec.h>
#include <qdebug.h>

#include <QMessageBox>
#include <QProgressBar>

#include <QIcon>

#ifdef NATIVE_C
#else
#include <qgsmessagelog.h>
#endif

//#include <boost/timer/timer.hpp>
#include "data_struct.h"
#include "netcdf.h"

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
    T* m_arrayPtr = nullptr;
    int m_numTimes = 0;
    int m_numLayer = 0;
    int m_numXY = 0;

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

template<typename T>
struct DataValuesProvider4D
{
public:
    DataValuesProvider4D() {};
    T* m_arrayPtr = nullptr;
    int m_numTimes = 0;
    int m_numLayer = 0;
    int m_numSed = 0;
    int m_numXY = 0;

    // copy the pointer and values
    DataValuesProvider4D(T* arrayPtr, int numTimes, int numLayer, int numSed, int numXY) :
        m_arrayPtr(arrayPtr),
        m_numTimes(numTimes),
        m_numLayer(numLayer),
        m_numSed(numSed),
        m_numXY(numXY)
    {
    }

    inline T* GetValueAtIndex(int timeIndex, int layerIndex, int sedIndex, int xyIndex)
    {
        int index = GetIndex(timeIndex, layerIndex, sedIndex, xyIndex);
        return &m_arrayPtr[index];
    }

    inline int GetIndex(int timeIndex, int layerIndex, int sedIndex, int xyIndex)
    {
        int index = m_numXY * m_numLayer * m_numSed * timeIndex + m_numXY * m_numSed * layerIndex + m_numXY * sedIndex + xyIndex;
        return index;
    }
};
////////////////////////////////////////////////////////////////////////////////
enum class FILE_TYPE {
    UGRID,
    SGRID,
    KISS,
    UNKNOWN
};

struct _time_series {
    size_t nr_times;
    QString * long_name;
    QString * unit;
    QString * dim_name;
    std::vector<double> times;  // vector of seconds
    std::map<std::string, std::string> map_dim_name;
};

struct _variable {
public:
    _variable() {};
    double fill_value;
    std::vector<long> dims;
    nc_type nc_type;
    int topology_dimension;
    std::string var_name;
    std::string location;
    std::string mesh;
    std::string coordinates;
    std::string cell_methods;
    std::string standard_name;
    std::string long_name;
    std::string units;
    std::string grid_mapping;
    std::string comment;
    std::string description;
    std::vector<int> flag_values;
    std::vector<std::string> flag_meanings;
    //
    bool draw;
    bool read;
    bool time_series;
    std::vector<std::string> dim_names;
    int nr_hydro_layers = -1;
    int nr_bed_layers = -1;
    int sediment_index = -1;
    int sediment_array = -1;
    std::vector<double> layer_center;
    std::vector<double> layer_interface;
    DataValuesProvider2D<double> data_2d;
    DataValuesProvider3D<double> data_3d;
    DataValuesProvider4D<double> data_4d;
    std::vector<std::vector<std::vector <double *>>> z_3d;
    std::map<std::string, std::string> map_dim_name;
};

struct _feature {
    size_t count;
    std::vector<long> branch;  // 1D, on which branch the node is
    std::vector<double> chainage;  // 1D, chainage of the node
    std::vector<double> x;
    std::vector<double> y;
    std::vector<std::string> name;
    std::vector<std::string> long_name;
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
    std::vector<std::vector<int>> face_nodes;  // face: the location of the mass centre
};

struct _mesh1d {
    long nr_mesh1d;
    struct _feature ** node;
    struct _edge ** edge;
};

struct _mesh_contact {
    long nr_mesh_contact;
    std::string mesh_a;  // long_name of the first mesh
    std::string mesh_b;  // long_name of the second mesh
    std::string location_a;
    std::string location_b;
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
    std::vector<std::string> name;
    std::vector<std::string> long_name;
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
    std::vector<double> x;
    std::vector<double> y;
    std::vector<std::vector<double>> x_bounds;  // begin- and endpoint of edge when drawing quantities, not necessarily the begin an end point
    std::vector<std::vector<double>> y_bounds;  // begin- and endpoint of edge when drawing quantities, not necessarily the begin an end point
    int ** edge_nodes;
    std::vector<long> edge_branch;
    std::vector<double> edge_length;
    std::vector<std::string> name;
    std::vector<std::string> long_name;
};

struct _faces {
    size_t count;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<std::vector<double>> x_bounds;  // polygon used when drawing quantities, not necessarily the total face
    std::vector<std::vector<double>> y_bounds;  // polygon used when drawing quantities, not necessarily the total face
};

struct _ntw_string {
    std::string var_name;
    std::string cf_role;
    std::string edge_dimension;
    std::string edge_geometry;
    std::string edge_length;
    std::string edge_node_connectivity;
    std::string edge_type;  // not yet used
    std::string long_name;
    std::string node_coordinates;
    size_t toplogy_dimension;
    //
    std::string x_ntw_name;
    std::string y_ntw_name;
    //
    std::string node_names;
    std::string node_long_names;
    std::string edge_names;
    std::string edge_long_names;
    std::string edge_order;
};
struct _geom_string {
    std::string var_name;
    std::string cf_role;
    std::string long_name;
    std::string node_count;
    std::string node_coordinates;
    //
    std::string x_geom_name;
    std::string y_geom_name;
    //
    std::string node_dimension;
};
struct _mesh1d_string {
    std::string var_name;
    std::string cf_role;
    std::string coordinate_space;
    std::string edge_node_connectivity;
    std::string edge_type;  // open or closed 1D-edge
    std::string long_name;
    std::string node_coordinates;
    std::string node_dimension;
    std::string node_names;
    std::string node_long_names;
    std::string edge_coordinates;  // 1D optional required
    std::string edge_names;
    std::string edge_long_names;
    size_t topology_dimension;
    //
    std::string node_branch;
    std::string node_chainage;
    std::string x_node_name;
    std::string y_node_name;
    //
    std::string edge_dimension;
    std::string edge_branch;
    std::string edge_chainage;
    std::string x_edge_name;  //  mid point of edge (in general)
    std::string y_edge_name;  //  mid point of edge (in general)
    std::string node_edge_exchange;
    //
    std::vector<std::string> dim_name;;  // in case of 2DV simualtion
};
struct _mesh2d_string {
    std::string var_name;
    std::string long_name;
    std::string edge_coordinates;  // 2D
    std::string x_edge_name;  // 2D
    std::string y_edge_name;  // 2D
    std::string face_coordinates;  // 2D
    std::string x_face_name;  // 2D
    std::string y_face_name;  // 2D
    std::string node_coordinates;  // 1D, 2D
    std::string x_node_name;  // 2D
    std::string y_node_name;  // 2D
    size_t topology_dimension;
    //
    std::string edge_dimension;
    std::string edge_geometry;
    std::string edge_length;
    std::string edge_face_connectivity;
    std::string edge_node_connectivity;
    std::string edge_type;  // internal_closed, internal, boundary, boundary_closed
    std::string face_dimension;
    std::string face_edge_connectivity;
    std::string face_face_connectivity;
    std::string face_node_connectivity;
    std::string max_face_nodes_dimension;
    std::string node_edge_exchange;
    std::string layer_dimension;
    std::string layer_interface_dimension;
    //
    std::vector<std::string> dim_name;  // in case of 3D-sigma simualtion
    std::string x_bound_edge_name;
    std::string y_bound_edge_name;
    std::string x_bound_face_name;
    std::string y_bound_face_name;
};

struct _mesh_contact_string {
    std::string var_name;
    std::string meshes;
    std::string mesh_a;
    std::string mesh_b;
    std::string mesh_contact;
    //
    std::vector<std::string> dim_name;  // in case of 2DV + 3D-sigma simulation
};


class GRID
{
public:
#ifdef NATIVE_C
    GRID();
    char * get_filename();
#else
    GRID();
    GRID(QFileInfo filename, int, QProgressBar* pgBar);
    QFileInfo get_filename();
#endif
    ~GRID();
    long open(QFileInfo, QProgressBar* );
    long read();
    long read_global_attributes();
    long get_global_attribute_value(std::string, std::string*);

    long read_ugrid_mesh();
    long read_sgrid_mesh();
    long read_kiss_mesh();
    long read_times();
    long read_ugrid_variables();
    long read_sgrid_variables();
    long read_kiss_variables();
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

    FILE_TYPE get_file_type();

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

    FILE_TYPE m_ftype;
    int m_ncid;
    int * topo_edge_nodes;
    int * mesh1d_edge_nodes;
    int * mesh2d_edge_nodes;
    int * contact_edge_nodes;
#ifdef NATIVE_C
    char * fname;
    char * grid_file_name;
#else
    QFileInfo m_fname;
    QString m_grid_file_name;
    QProgressBar * m_pgBar;
#endif
    struct _global_attributes * global_attributes;
};

#endif  // __GRID_H__
