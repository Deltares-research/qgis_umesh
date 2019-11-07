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

#include "data_struct.h"

using namespace std;

struct _time_series {
    size_t nr_times;
    QString * long_name;
    QString * unit;
    QString * dim_name;
    double * times;
};


struct _variable {
    double fill_value;
    vector<long> dims;
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
    int nr_sigma_layers;
    vector<vector <double *>> z_value;
    vector<vector<vector <double *>>> z_3d;
};
struct _feature {
    size_t count;
    long * branch;  // 1D, on which branch the node is
    double * chainage;  // 1D, chainage of the node
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
    int ** edge_nodes;
    vector<double> branch_length;
    vector<string> name;
    vector<string> long_name;
};
struct _faces {
    size_t count;
    vector<double> x;
    vector<double> y;
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
    string edge_names;
    string edge_long_names;
    size_t topology_dimension;
    //
    string branch;
    string chainage;
    string x_node_name;
    string y_node_name;
    //
    string edge_dimension;
    string node_edge_exchange;
    //
    string sigma_dim_name;  // in case of 2DV simualtion
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
    //
    string sigma_dim_name;  // in case of 3D-sigma simualtion
};

struct _mesh_contact_string {
    string var_name;
    string meshes;
    string mesh_a;
    string mesh_b;
    string mesh_contact;
    //
    string sigma_dim_name;  // in case of 2DV + 3D-sigma simualtion
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
    double * get_times();
    QVector<QDateTime> get_qdt_times();  // qdt: Qt Date Time
    QVector<QDateTime> qdt_times;

    struct _mesh_variable * get_variables();
    vector<vector <double *>> get_variable_values(const string);
    vector<vector<vector <double *>>> get_variable_3d_values(const string);

    struct _variable * get_edge_variables();

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

    struct _ntw_nodes * ntw_nodes = NULL;
    struct _ntw_edges * ntw_edges = NULL;
    struct _ntw_geom * ntw_geom = NULL;
    struct _mesh_contact * mesh_contact= NULL;
    struct _ntw_nodes * mesh1d_nodes = NULL;
    struct _ntw_edges * mesh1d_edges = NULL;
    struct _mesh1d * mesh1d = NULL;
    struct _mesh2d * mesh2d = NULL;
    struct _mapping * mapping = NULL;
    struct _mesh_variable * mesh_vars = NULL;
    struct _feature * nodes = NULL;
    struct _ntw_string ** ntw_strings = NULL;
    struct _geom_string ** geom_strings = NULL;
    struct _mesh1d_string ** mesh1d_strings = NULL;
    struct _mesh2d_string ** mesh2d_strings = NULL;
    struct _mesh_contact_string ** mesh_contact_strings = NULL;


    struct _time_series * time_series = NULL;

    long _nr_mesh_contacts;
    size_t _two;
    size_t * _dimids;
    vector<string> _dim_names;

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
    QFileInfo ugrid_file_name;
    QProgressBar * _pgBar;
#endif
    _global_attributes * global_attributes;
};

#endif  // UGRID_H
