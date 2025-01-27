#ifndef __SGRID_H__
#define __SGRID_H__

// Class to support structured grids

#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>

#ifdef NATIVE_C
#else
#include <qgsmessagelog.h>
#endif

#include "data_struct.h"
#include "netcdf.h"

struct _stime_series {
    size_t nr_times;
    QString* long_name;
    QString* unit;
    QString* dim_name;
    std::vector<double> times;  // vector of seconds
    std::map<std::string, std::string> map_dim_name;
};

struct _svariable {
public:
    _svariable() {};
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
    //DataValuesProvider2D<double> data_2d;
    //DataValuesProvider3D<double> data_3d;
    //DataValuesProvider4D<double> data_4d;
    std::vector<std::vector<std::vector <double*>>> z_3d;
    std::map<std::string, std::string> map_dim_name;
};
struct _smesh_variable {
    long nr_vars;
    struct _svariable** svariable;
};
struct _sfeature {
    size_t count;
    std::vector<long> branch;  // 1D, on which branch the node is
    std::vector<double> chainage;  // 1D, chainage of the node
    std::vector<double> x;
    std::vector<double> y;
    std::vector<std::string> name;
    std::vector<std::string> long_name;
};
struct _smesh2d {
    long nr_smesh2d;
    struct _sfeature** node;
};
struct _smesh2d_string {
    std::string var_name;
    std::string long_name;
    std::string node_coordinates;  // 2D
    std::string x_node_name;  // 2D
    std::string y_node_name;  // 2D
    size_t topology_dimension;
};


class SGRID
{
public:
    SGRID();
    ~SGRID();
    SGRID(QFileInfo filename, int, QProgressBar* pgBar);
    QFileInfo get_filename();

    long read();
    //QVector<QDateTime> get_qdt_times();  // qdt: Qt Date Time
    struct _mapping* get_grid_mapping();
    long set_grid_mapping_epsg(long, std::string);
    struct _smesh_variable* get_variables();
    struct _smesh2d* get_smesh_2d();


private:
    long read_mesh();
    long read_times();
    long read_variables();

    int get_attribute_by_var_name(int, std::string, std::string, std::string*);
    int get_attribute(int, int, char*, char**);
    int get_attribute(int, int, char*, std::string*);
    int get_attribute(int, int, std::string, std::string*);
    int get_attribute(int, int, char*, double*);
    int get_attribute(int, int, char*, int*);
    int get_attribute(int, int, char*, long*);
    int get_dimension(int, char*, size_t*);
    int get_dimension(int, std::string, size_t*);
    int get_dimension_var(int, std::string, size_t*);

    std::vector<std::string> tokenize(const std::string&, char);
    std::vector<std::string> tokenize(const std::string&, std::size_t);

    struct _smesh2d* m_smesh2d = nullptr;
    struct _smesh2d_string** m_smesh2d_strings = nullptr;
    struct _smesh_variable* m_smesh_vars = nullptr;
    int m_ncid;
    QFileInfo m_fname;
    QString m_sgrid_file_name;
    int m_nr_smesh_var;
    QProgressBar* m_pgBar; 
    struct _mapping * m_mapping;
    size_t* m_dimids;
    std::vector<std::string> m_dim_names;
    std::map<std::string, long> m_map_dim;
    std::map<std::string, std::string> m_map_dim_name;
    std::vector<_stime_series> time_series;
    QVector<QDateTime> qdt_times;

};
#endif __SGRID_H__
