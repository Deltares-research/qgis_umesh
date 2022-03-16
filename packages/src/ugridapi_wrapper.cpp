#define NOMINMAX
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <limits>
#include <array>
#include <netcdf.h>

#if defined(WIN32) || defined(WIN64)
#  include <direct.h>
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#if defined(WIN32) || defined(WIN64)
#  define strdup _strdup
#  define strcat strcat_s
#  define getcwd _getcwd
#  define FILE_NAME_MAX _MAX_PATH
#else
#  define FILE_NAME_MAX FILENAME_MAX
#endif

#include "ugridapi_wrapper.h"
#include "UGridApi/UGrid.hpp"
#include "perf_timer.h"
#include "netcdf.h"

//------------------------------------------------------------------------------
#ifdef NATIVE_C
UGRIDAPI_WRAPPER::UGRIDAPI_WRAPPER(std::string ncfile) : m_ncid(-1)
#else
UGRIDAPI_WRAPPER::UGRIDAPI_WRAPPER(QFileInfo filename, QProgressBar* pgBar) : m_ncid(-1)
#endif
{
#ifdef NATIVE_C
    m_filename = ncfile;  // filename without path, just the name
    std::string fname = m_filename;  // copy needed for NON native_C code
#else
    m_filename = filename;
    m_ugrid_filename = filename.absoluteFilePath();
    std::string fname = m_ugrid_filename.toStdString(); // copy needed for native_C code
    m_pgBar = pgBar;
#endif 
    m_mapping = new _mapping();

    int file_mode = -1;  // 0 is invalid because that is the default read mode
    auto error_code = ugridapi::ug_file_read_mode(file_mode);  // indpendent on filename
    error_code = ugridapi::ug_file_open(fname.c_str(), file_mode, m_ncid);
    if (error_code != 0) {
        const char* janm;
        ugridapi::ug_error_get(janm);
        std::cout << janm << std::endl;
        std::cout << "error: while opening file: " << fname << std::endl;
        return;
    }
}
//------------------------------------------------------------------------------
UGRIDAPI_WRAPPER::~UGRIDAPI_WRAPPER()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::~UGRIDAPI_WRAPPER (destructor)" << std::endl;
#endif    
    ugridapi::ug_file_close(m_ncid);
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::read()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::read()" << std::endl;
#endif    
    long status = -1;
    status = read_global_attributes();
    status = read_grid_mapping();

    int ndims, nvars, natts, nunlimited;
    char* var_name_c = (char*)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);
    m_dimids = (size_t*)malloc(sizeof(size_t) * ndims);
    for (int i = 0; i < ndims; i++)
    {
        status = nc_inq_dimlen(this->m_ncid, i, &m_dimids[i]);
        status = nc_inq_dimname(this->m_ncid, i, var_name_c);
        m_dim_names.push_back(std::string(var_name_c));
        m_map_dim[std::string(var_name_c)] = m_dimids[i];
    }
    free(var_name_c);
    var_name_c = nullptr;

    START_TIMERN(Read mesh);
    status = read_network();
    status = read_mesh_1d();
    status = read_mesh_2d();
    status = read_mesh_contacts();
    STOP_TIMER(Read mesh);

    START_TIMER(Read times);
    status = read_times();
    STOP_TIMER(Read times);

    START_TIMER(Read variables);
    status = read_variables();
    STOP_TIMER(Read variables);

    return 0;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::read_grid_mapping()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::read_grid_mapping()" << std::endl;
#endif   
    long status = -1;
    m_grid_mapping = new _mapping();
    // get variable which has at attribute: epsg
    int ndims, nvars, natts, nunlimited;
    status = nc_inq(m_ncid, &ndims, &nvars, &natts, &nunlimited);

    char* var_name_c = (char*)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    bool mapping_found = false;
    int att_int_value;
    for (int i_var = 0; i_var < nvars; ++i_var)
    {
        nc_type nc_type;
        std::string att_str_value;
        nc_inq_varndims(m_ncid, i_var, &ndims);
        int* var_dimids = NULL;
        if (ndims > 0) {
            var_dimids = (int*)malloc(sizeof(int) * ndims);
        }
        var_name_c[0] = '\0';
        status = nc_inq_var(m_ncid, i_var, var_name_c, &nc_type, &ndims, var_dimids, &natts);
        std::string var_name(var_name_c);

        if (status == NC_NOERR) 
        { 
            status = get_attribute(m_ncid, i_var, "name", &att_str_value);  // required attribute
            status = get_attribute(m_ncid, i_var, "epsg", &att_int_value);  // required attribute
            m_grid_mapping->name = att_str_value;
            m_grid_mapping->epsg = att_int_value;
        }
        if (status == NC_NOERR) 
        { 
            att_str_value = "-";
            status = get_attribute(m_ncid, i_var, "EPSG_code", &att_str_value); 
            m_grid_mapping->epsg_code = att_str_value;
            status = NC_NOERR;  // NOT required attribute
        }

        if (status == 0)
        {
            std::cerr << "\tVariable name: " << var_name << std::endl;
            std::cerr << "\t\tname     : " << m_grid_mapping->name << std::endl;
            std::cerr << "\t\tepsg     : " << m_grid_mapping->epsg << std::endl;
            std::cerr << "\t\tEPSG_code: " << m_grid_mapping->epsg_code << std::endl;
            mapping_found = true;
            break;
        }
    }
    free(var_name_c);
    var_name_c = nullptr;
    if (!mapping_found)
    {
#if defined NATIVE_C
        std::cerr << "\tNo grid mapping found in file: " << m_filename << std::endl;
#endif
    }
    return status;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::read_global_attributes()
{
#ifdef NATIVE_C
    fprintf(stderr, "UGRIDAPI_WRAPPER::read_global_attributes()\n");
#endif    
    long status;
    int ndims, nvars, natts, nunlimited;
    nc_type att_type;
    size_t att_length;

    char* att_name_c = (char*)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    att_name_c[0] = '\0';

    status = nc_inq(m_ncid, &ndims, &nvars, &natts, &nunlimited);

    m_global_attributes = (struct _global_attributes*)malloc(sizeof(struct _global_attributes));
    m_global_attributes->count = natts;
    m_global_attributes->attribute = (struct _global_attribute**)malloc(sizeof(struct _global_attribute*) * natts);
    for (long i = 0; i < natts; i++)
    {
        m_global_attributes->attribute[i] = new _global_attribute;
    }

    for (long i = 0; i < natts; i++)
    {
        status = nc_inq_attname(m_ncid, NC_GLOBAL, i, att_name_c);
        status = nc_inq_att(m_ncid, NC_GLOBAL, att_name_c, &att_type, &att_length);
        m_global_attributes->attribute[i]->name = std::string(strdup(att_name_c));
        m_global_attributes->attribute[i]->type = att_type;
        m_global_attributes->attribute[i]->length = att_length;
        if (att_type == NC_CHAR)
        {
            char* att_value_c = (char*)malloc(sizeof(char) * (att_length + 1));
            att_value_c[0] = '\0';
            status = nc_get_att_text(m_ncid, NC_GLOBAL, att_name_c, att_value_c);
            att_value_c[att_length] = '\0';
            m_global_attributes->attribute[i]->cvalue = std::string(strdup(att_value_c));
            free(att_value_c);
            att_value_c = nullptr;
        }
        else
        {
#ifdef NATIVE_C
            fprintf(stderr, "    Attribute nc_type: %d\n", att_type);
#endif    
        }
    }
    free(att_name_c);
    att_name_c = nullptr;

    return status;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::read_network()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::read_network()" << std::endl;
#endif   
    int error_code = -1;
    long status = -1;
    int topology_type;
    error_code = ugridapi::ug_topology_get_network1d_enum(topology_type);
    int nr_ntw;
    error_code = ugridapi::ug_topology_get_count(m_ncid, topology_type, nr_ntw);

    if (nr_ntw > 0)  // there is a 1D network in the file
    {
        size_t mem_length{ 0 };
        // Get the dimensions
        error_code = ug_network1d_inq(m_ncid, 0, m_network_1d);
        m_network_1d.name.resize(NC_MAX_NAME + 1);  // this contains the variable name, TODO var_name should be a better name
        error_code = ug_network1d_get(m_ncid, 0, m_network_1d);

        mem_length = get_memory_length(m_network_1d.name, "node_id");
        m_network_1d.node_id.resize(mem_length);
        mem_length = get_memory_length(m_network_1d.name, "node_long_name");
        m_network_1d.node_long_name.resize(mem_length);

        mem_length = get_memory_length(m_network_1d.name, "branch_id");
        if (mem_length == 0)
        {
            mem_length = get_memory_length(m_network_1d.name, "branch_name");
        }
        if (mem_length != 0)
        {
            m_network_1d.edge_id.resize(mem_length);
        }

        m_network_1d.node_x.resize(m_network_1d.num_nodes);
        m_network_1d.node_y.resize(m_network_1d.num_nodes);
        m_network_1d.geometry_nodes_x.resize(m_network_1d.num_geometry_nodes);
        m_network_1d.geometry_nodes_y.resize(m_network_1d.num_geometry_nodes);
        m_network_1d.edge_length.resize(m_network_1d.num_edges);
        m_network_1d.edge_node.resize(m_network_1d.num_edges * 2);

        error_code = ug_network1d_get(m_ncid, 0, m_network_1d);

        {
            // reading long_name
            std::string long_name;
            std::string tmp_name = trim(m_network_1d.name);
            status = get_attribute_by_var_name(m_ncid, tmp_name, "long_name", &long_name);
        }
        {
            // reading node_count
            int i_var;
            std::string var_edge_geometry;
            std::string var_node_count;

            std::string var_name = trim(m_network_1d.name);
            status = get_attribute_by_var_name(m_ncid, var_name, "edge_geometry", &var_edge_geometry);
            status = get_attribute_by_var_name(m_ncid, var_edge_geometry, "node_count", &var_node_count);
            status = nc_inq_varid(m_ncid, var_node_count.c_str(), &i_var);
            m_node_count.resize(m_network_1d.num_edges);
            status = nc_get_var(m_ncid, i_var, m_node_count.data());
            m_network_1d.node_count = m_node_count;
        }
        {
            // tokenize the node/edge ids and long names
            m_network_1d.vector_node_id = tokenize(m_network_1d.node_id, m_network_1d.node_id.size() / m_network_1d.num_nodes);
            m_network_1d.vector_node_long_name = tokenize(m_network_1d.node_long_name, m_network_1d.node_long_name.size() / m_network_1d.num_nodes);
            m_network_1d.vector_edge_id = tokenize(m_network_1d.edge_id, m_network_1d.edge_id.size() / m_network_1d.num_edges);
            m_network_1d.vector_edge_long_name = tokenize(m_network_1d.edge_long_name, m_network_1d.edge_long_name.size() / m_network_1d.num_edges);
        }
        if (m_network_1d.edge_node.size() > 0)
        {
            // reading start_index, needed for edge_nodes array
            std::string tmp_name = trim(m_network_1d.name);
            int start_index = get_start_index(m_ncid, tmp_name, "edge_node_connectivity");
            if (start_index == -2) { start_index = 0; }  // TODO Need nice log-message
            m_network_1d.start_index = start_index;

            if (m_network_1d.start_index > 0)
            {
                for (int i = 0; i < m_network_1d.num_edges * 2; ++i)
                {
                    m_network_1d.edge_node[i] -= m_network_1d.start_index;
                }
            }
        }
    }
    else
    {
#if defined NATIVE_C
        std::cout << "No network defined in file: " << m_filename << std::endl;
#endif
        status = 1;
    }
    return status;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::read_mesh_1d()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::read_mesh_1d()" << std::endl;
#endif   
    int error_code = -1;
    long status = -1;
    int start_index;
    int topology_type;
    error_code = ugridapi::ug_topology_get_mesh1d_enum(topology_type);
    int nr_mesh1d;
    error_code = ugridapi::ug_topology_get_count(m_ncid, topology_type, nr_mesh1d);

    if (nr_mesh1d > 0)  // there is a 1D network in the file
    {
        size_t mem_length{ 0 };
        // Get the dimensions
        error_code = ug_mesh1d_inq(m_ncid, 0, m_mesh_1d);
        m_mesh_1d.name.resize(NC_MAX_NAME + 1);  // this contains the variable name, TODO var_name should be a better name
        error_code = ug_mesh1d_get(m_ncid, 0, m_mesh_1d);

        mem_length = get_memory_length(m_mesh1d.name, "coordinate_space");
        m_mesh_1d.network_name.resize(mem_length);

        m_mesh_1d.node_x.resize(m_mesh_1d.num_nodes);
        m_mesh_1d.node_y.resize(m_mesh_1d.num_nodes);
        m_mesh_1d.edge_node.resize(m_mesh_1d.num_edges * 2);
        m_mesh_1d.node_edge_id.resize(m_mesh_1d.num_nodes, -1); // initialize at -1, needed for test if (x,y)-coordinates are given or (id,offset) on branch
        m_mesh_1d.node_edge_offset.resize(m_mesh_1d.num_nodes);
        m_mesh_1d.edge_edge_id.resize(m_mesh_1d.num_edges);
        m_mesh_1d.edge_edge_offset.resize(m_mesh_1d.num_edges);
        m_mesh_1d.edge_length.resize(m_mesh_1d.num_edges);

        error_code = ug_mesh1d_get(m_ncid, 0, m_mesh_1d);

        if (m_mesh_1d.edge_node.size() != 0)
        {
            std::string var_name = trim(m_mesh_1d.name);
            start_index = get_start_index(m_ncid, var_name, "edge_node_connectivity");
            if (start_index > 0)
            {
                for (int i = 0; i < m_mesh_1d.num_edges * 2; ++i)
                {
                    m_mesh_1d.edge_node[i] -= start_index;
                }
            }
        }
        if (m_mesh_1d.node_edge_id.size() != 0)
        {
            std::string var_name = trim(m_mesh_1d.name);
            start_index = get_start_index(m_ncid, var_name, "node_coordinates");
            if (start_index > 0)
                    {
                for (int i = 0; i < m_mesh_1d.num_nodes; ++i)
                {
                    m_mesh_1d.node_edge_id[i] -= start_index;
                    }
                }
        }
        {
            std::string var_name = trim(m_mesh_1d.name);
            start_index = get_start_index(m_ncid, var_name, "edge_coordinates");
            if (start_index > 0)
            {
                for (int i = 0; i < m_mesh_1d.num_edges; ++i)
                {
                    m_mesh_1d.edge_edge_id[i] -= start_index;
                }
            }
        }
        {
            if (m_mesh_1d.node_edge_id[0] != -1)  // check on value of initialization
            {
                // compute the (x,y)-coordinates of the nodes on the geometry
                status = create_mesh1d_nodes(m_mesh_1d, &m_network_1d);
            }
            else
        {
                // (x,y)_coordinates are read from file (not yet correct implemented in UGridApi, dd 20220115)
            }
        }
    }
    return status;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::read_mesh_2d()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::read_mesh_2d()" << std::endl;
#endif   
    int error_code = -1;
    long status = -1;
    int start_index;
    int topology_type;
    error_code = ugridapi::ug_topology_get_mesh2d_enum(topology_type);
    int nr_mesh2d;
    error_code = ugridapi::ug_topology_get_count(m_ncid, topology_type, nr_mesh2d);

    if (nr_mesh2d > 0)  // there is a 1D network in the file
    {
        // Get the dimensions
        error_code = ug_mesh2d_inq(m_ncid, 0, m_mesh_2d);
        m_mesh_2d.name.resize(NC_MAX_NAME + 1);  // this contains the variable name, TODO var_name should be a better name
        error_code = ug_mesh2d_get(m_ncid, 0, m_mesh_2d);
        if (m_mesh_2d.num_layers == 0)
        {
            // try reading again num_layers
            std::string tmp_str;
            error_code = ugridapi::ug_variable_get_attribute_value(m_ncid, m_mesh_2d.name, "layer_dimension", tmp_str);
            //error_code = ugridapi::ug_variable_get_attribute_value(m_ncid, m_mesh_2d.name, "interface_dimension", tmp_str);
            for (int i = 0; i < m_dim_names.size(); ++i)
            {
                if (m_dim_names[i] == tmp_str)
                {
                    m_mesh_2d.num_layers = m_map_dim[tmp_str];
                    break;
                }
            }
        }

        m_mesh_2d.node_x.resize(m_mesh_2d.num_nodes);
        m_mesh_2d.node_y.resize(m_mesh_2d.num_nodes);
        m_mesh_2d.face_x.resize(m_mesh_2d.num_faces);
        m_mesh_2d.face_y.resize(m_mesh_2d.num_faces);
        m_mesh_2d.edge_node.resize(m_mesh_2d.num_edges * 2);
        m_mesh_2d.edge_length.resize(m_mesh_2d.num_edges);
        m_mesh_2d.face_node.resize(m_mesh_2d.num_faces * m_mesh_2d.num_face_nodes_max); // needed for drawing patches of a face

        m_mesh_2d.layer_zs.resize(m_mesh_2d.num_layers);

        error_code = ug_mesh2d_get(m_ncid, 0, m_mesh_2d);

        {
            // reading start_index, needed for edge_nodes array
            std::string var_name = trim(m_mesh_2d.name);
            start_index = get_start_index(m_ncid, var_name, "edge_node_connectivity");
            if (start_index > 0)
            {
                for (int i = 0; i < m_mesh_2d.num_edges * 2; ++i)
                {
                    m_mesh_2d.edge_node[i] -= start_index;
                }
            }
        }
        {
            // reading start_index, needed for face_nodes array
            std::string var_name = trim(m_mesh_2d.name);
            start_index = get_start_index(m_ncid, var_name, "face_node_connectivity");
            if (start_index > 0)
            {
                for (int i = 0; i < m_mesh_2d.num_faces * m_mesh_2d.num_face_nodes_max; ++i)
                {
                    m_mesh_2d.face_node[i] -= start_index;
                }
            }
        }
        {
            for (int i = 0; i < m_mesh_2d.num_edges; ++i)
            {
                m_mesh_2d.edge_length[i] = 0.0;;
            }
        }
    }
    return status;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::read_mesh_contacts()
{
    int error_code = -1;
    long status = -1;
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::read_mesh_contacts()" << std::endl;
#endif   
    int topology_type;
    error_code = ugridapi::ug_topology_get_contacts_enum(topology_type);
    int nr_contacts;
    error_code = ugridapi::ug_topology_get_count(m_ncid, topology_type, nr_contacts);

    if (nr_contacts > 0)  // there is a 1D network in the file
    {
        size_t mem_length{ 0 };
        // Get the dimensions
        error_code = ug_contacts_inq(m_ncid, 0, m_mesh_contacts);
        m_mesh_contacts.name.resize(NC_MAX_NAME + 1);  // this contains the variable name, TODO var_name should be a better name
        error_code = ug_contacts_get(m_ncid, 0, m_mesh_contacts);

        mem_length = get_memory_length(m_mesh_contacts.name, "mesh_from_name");
        m_mesh_contacts.mesh_from_name.resize(mem_length);
 
        mem_length = get_memory_length(m_mesh_contacts.name, "mesh_to_name");
        m_mesh_contacts.mesh_from_name.resize(mem_length);

        m_mesh_contacts.edges.resize(m_mesh_contacts.num_contacts * 2);

        error_code = ug_contacts_get(m_ncid, 0, m_mesh_contacts);

        {
            // reading start_index, needed for edge_nodes array
            int i_var;
            int start_index = -2;
            std::string var_edge_node;
            std::string var_start_index;
            std::string tmp_name = trim(m_mesh_contacts.name);
            status = get_attribute_by_var_name(m_ncid, tmp_name, "edge_node_connectivity", &var_edge_node);
            status = nc_inq_varid(m_ncid, var_edge_node.c_str(), &i_var);
            status = nc_get_att_int(m_ncid, i_var, "start_index", &start_index);

            std::string from_name = trim(m_mesh_contacts.mesh_from_name);
            int start_from = get_start_index(m_ncid, from_name, "edge_node_connectivity");
            std::string to_name = trim(m_mesh_contacts.mesh_to_name);
            int start_to = get_start_index(m_ncid, to_name, "edge_node_connectivity");


            for (int i = 0; i < m_mesh_contacts.num_contacts; ++i)
                {
                {
                    m_mesh_contacts.edges[2 * i] -= start_from;
                    m_mesh_contacts.edges[2 * i + 1] -= start_to;
                    }
                }
        }
    }
    return status;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::read_variables_1d()
{
    // read the attributes of the variable and the dimensions
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::read_variables_1d()" << std::endl;
#endif 
    long status{ -1 };
    int error_code = -1;
    int topology_type = -1;
    int topology_id = 0;
    error_code = ugridapi::ug_topology_get_mesh1d_enum(topology_type);
    int nr_mesh1d;
    error_code = ugridapi::ug_topology_get_count(m_ncid, topology_type, nr_mesh1d);
    if (nr_mesh1d > 0)
    {
        // read_mesh1d
        std::vector<std::string> variables_names_1d;
        error_code = ugridapi::ug_topology_get_mesh1d_enum(topology_type);
        error_code = ugridapi::ug_topology_get_data_variables_names(m_ncid, topology_type, topology_id, variables_names_1d);

        for (int i = 0; i < variables_names_1d.size(); ++i)
        {
            m_vars_1d.emplace_back(new _variable_ugridapi);
            m_vars_1d.back()->draw = false;
            m_vars_1d.back()->time_series = false;

            m_vars_1d.back()->var_name = variables_names_1d.at(i);
            std::vector<int> dimension_value;
            std::vector<std::string> dimension_name;
            error_code = ugridapi::ug_variable_get_data_dimensions(m_ncid, variables_names_1d.at(i), dimension_name, dimension_value);

            if (dimension_name.size() == 1)  // xy-space
            {
            }
            else if (dimension_name.size() == 2)  // time, xy-xyspace
            {
                for (int j = 0; j < dimension_name.size(); ++j)
                {
                    if (QString::fromStdString(dimension_name.at(j)) == time_series[0].dim_name)
                    {
                        m_vars_1d.back()->time_series = true;
                        break;
                    }
                }
            }
            else
            {
            }
        }
#ifdef NATIVE_C
        else
        {
            std::cerr << "\tNo data on 1D mesh available" << std::endl;
        }
#endif
    }
    return status;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::read_variables_2d()
{
    // read the attributes of the variable and the dimensions
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::read_variables_2d()" << std::endl;
#endif 
    long status{ -1 };
    int error_code = -1;
    int topology_type = -1;
    int topology_id = 0;
    error_code = ugridapi::ug_topology_get_mesh2d_enum(topology_type);
    int nr_mesh2d;
    error_code = ugridapi::ug_topology_get_count(m_ncid, topology_type, nr_mesh2d);
    if (nr_mesh2d > 0)
    {
        // read mesh2d
        std::vector<std::string> variables_names_2d;
        error_code = ugridapi::ug_topology_get_mesh2d_enum(topology_type);
        error_code = ugridapi::ug_topology_get_data_variables_names(m_ncid, topology_type, topology_id, variables_names_2d);

        for (int i = 0; i < variables_names_2d.size(); ++i)
        {
            m_vars_2d.emplace_back(new _variable_ugridapi);
            m_vars_2d.back()->draw = false;
            m_vars_2d.back()->time_series = false;

            m_vars_2d.back()->var_name = variables_names_2d.at(i);
            std::vector<int> dimension_value;
            std::vector<std::string> dimension_name;
            error_code = ugridapi::ug_variable_get_data_dimensions(m_ncid, variables_names_2d.at(i), dimension_name, dimension_value);

            if (dimension_name.size() == 1)  // xy-space
            {
                // skippinig the time-axis: double time(time):
            }
            else if (dimension_name.size() >= 2)  // time, xy-xyspace
            {
                for (int j = 0; j < dimension_name.size(); ++j)
                {
                    if (QString::fromStdString(dimension_name.at(j)) == time_series[0].dim_name)
                    {
                        m_vars_2d.back()->time_series = true;
                        break;
                    }
                }
            }
    }
#ifdef NATIVE_C
        else
        {
            std::cerr << "\tNo data on 2D mesh available" << std::endl;
        }
#endif
    }
    return status;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::read_variables_contacts()
{
    // read the attributes of the variable and the dimensions
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::read_variables_contacts()" << std::endl;
#endif 
    long status{ -1 };
    int error_code = -1;
    int topology_type = -1;
    int topology_id = 0;
    error_code = ugridapi::ug_topology_get_contacts_enum(topology_type);
    int nr_contacts;
    error_code = ugridapi::ug_topology_get_count(m_ncid, topology_type, nr_contacts);
    if (nr_contacts > 0)
    {
        // reads_contacts
        std::vector<std::string> variables_names_contacts;
        error_code = ugridapi::ug_topology_get_contacts_enum(topology_type);
        error_code = ugridapi::ug_topology_get_data_variables_names(m_ncid, topology_type, topology_id, variables_names_contacts);
        for (int i = 0; i < variables_names_contacts.size(); ++i)
        {
            m_vars_1d2d.emplace_back(new _variable_ugridapi);
            m_vars_1d2d.back()->draw = false;
            m_vars_1d2d.back()->time_series = false;

            m_vars_1d2d.back()->var_name = variables_names_contacts.at(i);
            std::vector<int> dimension_value;
            std::vector<std::string> dimension_name;
            error_code = ugridapi::ug_variable_get_data_dimensions(m_ncid, variables_names_contacts.at(i), dimension_name, dimension_value);

            if (dimension_name.size() == 1)  // xy-space
            {
            }
            else if (dimension_name.size() == 2)  // time, xy-xyspace
            {
                for (int j = 0; j < dimension_name.size(); ++j)
                {
                    if (QString::fromStdString(dimension_name.at(j)) == time_series[0].dim_name)
                    {
                        m_vars_1d2d.back()->time_series = true;
                        break;
                    }
                }
            }
            else
            {
            }
        }
#ifdef NATIVE_C
        else
        {
            std::cerr << "\tNo contact data available" << std::endl;
        }
#endif
    }
    return status;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::read_variables()
{
    // read the attributes of the variable and the dimensions
#ifdef NATIVE_C
    fprintf(stderr, "UGRIDAPI_WRAPPER::read_variables()\n");
#else 
    m_pgBar->setValue(800);
#endif
    long status{ -1 };
    status = 0;

    START_TIMERN(Read variables 1d);
    status = read_variables_1d();
    STOP_TIMER(Read variables 1d);

    START_TIMER(Read variables contacts);
    status = read_variables_contacts();
    STOP_TIMER(Read variables contacts);

    START_TIMER(Read variables 2d);
    status = read_variables_2d();
    STOP_TIMER(Read variables 2d);

    std::vector<_variable_ugridapi*> vars_1d = get_variables_1d();
    std::vector<_variable_ugridapi*> vars_1d2d = get_variables_1d2d();
    std::vector<_variable_ugridapi*> vars_2d = get_variables_2d();

    m_vars.insert(std::end(m_vars), std::begin(vars_1d), std::end(vars_1d));
    m_vars.insert(std::end(m_vars), std::begin(vars_1d2d), std::end(vars_1d2d));
    m_vars.insert(std::end(m_vars), std::begin(vars_2d), std::end(vars_2d));

    return status;
}

//------------------------------------------------------------------------------
struct _global_attributes* UGRIDAPI_WRAPPER::get_global_attributes()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::get_global_attributes()" << std::endl;
#endif    
    return m_global_attributes;
}
//------------------------------------------------------------------------------
struct _network_1d UGRIDAPI_WRAPPER::get_network()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::get_network()" << std::endl;
#endif    
    return m_network_1d;
}
//------------------------------------------------------------------------------
struct _mesh_1d UGRIDAPI_WRAPPER::get_mesh_1d()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::get_mesh_1d()" << std::endl;
#endif    
    return m_mesh_1d;
}
//------------------------------------------------------------------------------
struct _mesh_2d UGRIDAPI_WRAPPER::get_mesh_2d()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::get_mesh_2d()" << std::endl;
#endif    
    return m_mesh_2d;
}
//------------------------------------------------------------------------------
struct _contacts UGRIDAPI_WRAPPER::get_mesh_contacts()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::get_mesh_contact()" << std::endl;
#endif    
    return m_mesh_contacts;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::get_count_times()
{
    size_t nr = time_series[0].nr_times;
    return (long)nr;
}
//------------------------------------------------------------------------------
std::vector<double> UGRIDAPI_WRAPPER::get_time_series()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::get_time_series()" << std::endl;
#endif    
    return time_series[0].times;
}

//------------------------------------------------------------------------------
QVector<QDateTime> UGRIDAPI_WRAPPER::get_qdt_times()  // qdt: Qt Date Time
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::get_qdt_times()" << std::endl;
#endif    
    return qdt_times;
}

//------------------------------------------------------------------------------
struct _mapping* UGRIDAPI_WRAPPER::get_grid_mapping()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::get_grid_mapping()" << std::endl;
#else
    //QMessageBox::warning(0, QString("Warning"), QString("UGRIDAPI_WRAPPER::get_grid_mapping()"));
#endif 
    return m_grid_mapping;
}
//------------------------------------------------------------------------------
struct _variable_ugridapi * UGRIDAPI_WRAPPER::get_var_by_std_name(std::vector<_variable_ugridapi *> vars, std::string mesh, std::string standard_name)
{
    for (int i = 0; i < vars.size(); i++)
    {
        int error_code;
        std::string std_name_value;
        std::string mesh_value;
        error_code = ugridapi::ug_variable_get_attribute_value(m_ncid, vars[i]->var_name, "mesh", mesh_value);
        error_code = ugridapi::ug_variable_get_attribute_value(m_ncid, vars[i]->var_name, "standard_name", std_name_value);
        if (standard_name == std_name_value && mesh == mesh_value)
        {
            return vars[i];
        }
    }
    return nullptr;  // return an empty struct (nullptr)
}//------------------------------------------------------------------------------
int UGRIDAPI_WRAPPER::get_var(const std::string var_name, DataValuesProvider2<double>& data_2)
{
    // return: 1d dimensional value(x), ie time independent
    // return: 2d dimensional value(time, x)
    long error_code = -1;
    error_code = get_var(var_name, -1, data_2);
    return error_code;
}
//------------------------------------------------------------------------------
int UGRIDAPI_WRAPPER::get_var(const std::string var_name, int array_index, DataValuesProvider2<double>& data_2)
// return: 2d dimensional value(time, x)
{
#ifdef NATIVE_C
    fprintf(stderr, "UGRID::get_variable_values()\n");
#endif    
    int error_code;
    int i_var{ -1 };
    int i_var_tmp{ -1 };
    nc_type var_type;
    std::vector<double> data_vector;
    std::vector<int> dimension_value;
    std::vector<std::string> dimension_name;
    std::vector<int> data_vector_int;
    std::string location;

    for (int i = 0; i < m_vars.size(); ++i)
    {
        if (var_name == m_vars[i]->var_name)
        {
            i_var = i;
        }
    }

    if (m_vars[i_var]->read)  // are the z_values already read
    {
        return 0;
    }

    START_TIMERN(read_variable_dvp2);
#ifdef NATIVE_C
#else
    m_pgBar->setValue(0);
    m_pgBar->show();
#endif
    error_code = nc_inq_varid(m_ncid, m_vars[i_var]->var_name.c_str(), &i_var_tmp);
    error_code = nc_inq_var(m_ncid, i_var_tmp, NULL, &var_type, NULL, NULL, NULL);

    error_code = ugridapi::ug_variable_get_data_dimensions(m_ncid, var_name, dimension_name, dimension_value);
    error_code = ugridapi::ug_variable_get_attribute_value(m_ncid, var_name, "location", location);

    int total_dimension{ 1 };
    for (int i = 0; i < dimension_value.size(); ++i)
    {
        total_dimension *= dimension_value[i];
    }
    data_vector.resize(total_dimension);
    if (var_type == NC_DOUBLE)
    {
        error_code = ugridapi::ug_variable_get_data_double(m_ncid, var_name, data_vector);
    }
    else if (var_type == NC_INT)
    {
        data_vector_int.resize(total_dimension);
        error_code = ugridapi::ug_variable_get_data_int(m_ncid, var_name, data_vector_int);
        for (int i = 0; i < total_dimension; ++i)
        {
            data_vector[i] = (double)data_vector_int[i];
        }
    }
#ifdef NATIVE_C
#else
    m_pgBar->setValue(400);
#endif

    if (dimension_value.size() == 1)  // xy-space
    {
        long time_dim = 1;
        long xy_dim = dimension_value[0];
        error_code = data_2.set_dimensions(time_dim, xy_dim);
        error_code = data_2.set_data(var_name, data_vector);
    }
    else if (dimension_value.size() == 2)  // time, xy-space
    {
        long time_dim = dimension_value[0];  // TODO: Assumed to be the time dimension
        long xy_dim = dimension_value[1];  // TODO: Assumed to be the 2DH space dimension
        error_code = data_2.set_dimensions(time_dim, xy_dim);
        if (m_vars[i_var]->time_series)
        {
            error_code = data_2.set_data(var_name, data_vector);
        }
        else
        {
            // not a time series, one dimesion should be the xy-space
            // but the other dimension is a list of array quantities (ex. type of sediments)
            if (location == "face")
            {
                for (int j = 0; j < dimension_value.size(); j++)
                {
                    if (m_mesh_2d.face_x.size() == dimension_value[j])
                    {
                        long time_dim = 1;
                        if (array_index != -1)
                        {
                            time_dim = array_index + 1;
                        }
                        long xy_dim = (long)dimension_value[j];
                        error_code = data_2.set_dimensions(time_dim, xy_dim);
                        error_code = data_2.set_data(var_name, data_vector);
                    }
                }
            }
        }
    }
    else if (dimension_value.size() == 3)  // time, xy-space
    {
        // nothing
    }
    else if (dimension_value.size() == 4)  // time, xy-space
    {
        // nothing
    }
    m_vars[i_var]->read = true;
#ifdef NATIVE_C
#else
    m_pgBar->setValue(1000);
    m_pgBar->hide();
#endif
    STOP_TIMER(read_variable_dvp2);
    return error_code;
}

//------------------------------------------------------------------------------
int UGRIDAPI_WRAPPER::get_var(const std::string var_name, DataValuesProvider3<double>& dvp)
// return: 3d dimensional value(time, layer, x)
{
#ifdef NATIVE_C
    fprintf(stderr, "UGRID::get_variable_3d_values()\n");
#endif    
    int error_code;
    int i_var;
    double* read_c;
    std::vector<double> data_vector;
    std::vector<int> dimension_value;
    std::vector<std::string> dimension_name;

    error_code = ugridapi::ug_variable_get_data_dimensions(m_ncid, var_name, dimension_name, dimension_value);

    error_code = nc_inq_varid(m_ncid, var_name.c_str(), &i_var);
    for (int i = 0; i < m_vars.size(); ++i)
    {
        if (var_name == m_vars[i]->var_name)
        {
            i_var = i;
            break;
        }
        }
    if (m_vars[i_var]->read)  // are the z_values already read
    {
        return 0;
    }
    START_TIMERN(read_variable_dvp3);
#ifdef NATIVE_C
#else
    m_pgBar->show();
    m_pgBar->setValue(0);
#endif

    int total_dimension{ 1 };
    for (int i = 0; i < dimension_value.size(); ++i)
    {
        total_dimension *= dimension_value[i];
    }
    data_vector.resize(total_dimension);
    // var_type == NC_DOUBLE
    error_code = ugridapi::ug_variable_get_data_double(m_ncid, var_name, data_vector);
#ifdef NATIVE_C
#else
    m_pgBar->setValue(100);
#endif
    if (dimension_value.size() == 3)  // xy-space
    {
        long time_dim = dimension_value[0];
        long layer_dim = dimension_value[1];
        long xy_dim = dimension_value[2];
        bool swap_loops = false;
        // Todo: HACK assumed is that the time is the first dimension
        // Todo: HACK just the variables at the layers, interfaces are skipped
        if (m_map_dim_name["zs_dim_layer"] == dimension_name[2] ||
            m_map_dim_name["zs_dim_interface"] == dimension_name[2] ||
            m_map_dim_name["zs_dim_bed_layer"] == dimension_name[2])
        {
            // loop over layers and nodes should be swapped
            swap_loops = true;
        }
        if (true)
        {
            double * values_c = (double*)malloc(sizeof(double) * total_dimension);
            if (swap_loops)
            {
                for (int t = 0; t < time_dim; t++)
                {
                    for (int xy = 0; xy < xy_dim; xy++)
                    {
                        for (int l = 0; l < layer_dim; l++)
                        {
                            int k_target = t * xy_dim * layer_dim + xy * layer_dim + l;
                            int k_source = t * xy_dim * layer_dim + l * xy_dim + xy;
                            values_c[k_target] = read_c[k_source];

                            double fraction = 100. + 900. * double(k_target) / double(total_dimension);
#ifdef NATIVE_C
#else
    m_pgBar->setValue(int(fraction));
#endif
                        }
                    }
                }
                int tmp_dim = layer_dim;
                layer_dim = xy_dim;
                xy_dim = tmp_dim;
                free(read_c);
            }
            else
            {
                memcpy(values_c, read_c, total_dimension * sizeof(double));
            }
        }
        dvp.set_dimensions(time_dim, layer_dim, xy_dim);

        for (int j = 0; j < dimension_name.size(); j++)
        {
            if (m_map_dim_name["z_sigma_interface"] == m_mesh_vars[i_var]->dim_names[j])
            {
#ifdef NATIVE_C
                fprintf(stderr, "    3D data on interfaces not yet supported.\n");
#else
                QMessageBox::warning(0, QString("Warning"), QString("3D data on interfaces not yet supported,\ndata: %1").arg(var_name.c_str()));
#endif
                return 0;
            }

        }
        error_code = dvp.set_data(var_name, data_vector);
        m_vars[i_var]->read = true;
#ifdef NATIVE_C
#else
        m_pgBar->setValue(1000);
        m_pgBar->hide();
#endif
    }
    STOP_TIMER(read_variable_dvp3);
    return error_code;
}
//------------------------------------------------------------------------------
int UGRIDAPI_WRAPPER::get_var(const std::string var_name, DataValuesProvider4<double>& dvp)
// return: 4d dimensional value(time, bed/hydro-layer, sediment, xy_space)
{
#ifdef NATIVE_C
    fprintf(stderr, "UGRID::get_variable_4d_values()\n");
#endif    
    int i_var;
    int error_code;
    std::vector<int> dimension_value;
    std::vector<std::string> dimension_name;
    std::vector<double> data_vector;
    std::string location;

    error_code = ugridapi::ug_variable_get_data_dimensions(m_ncid, var_name, dimension_name, dimension_value);
    error_code = ugridapi::ug_variable_get_attribute_value(m_ncid, var_name, "location", location);

    error_code = nc_inq_varid(m_ncid, var_name.c_str(), &i_var);

    if (m_vars[i_var]->read)  // are the z_values already read
    {
        return 0;
    }
    START_TIMERN(read_variable_dvp4);

#ifdef NATIVE_C
#else
    m_pgBar->show();
    m_pgBar->setValue(0);
#endif
    
    int total_dimension{ 1 };
    for (int i = 0; i < dimension_value.size(); ++i)
    {
        total_dimension *= dimension_value[i];
    }
    data_vector.resize(total_dimension);
    error_code = ugridapi::ug_variable_get_data_double(m_ncid, var_name, data_vector);
    //double* values_c = (double*)malloc(sizeof(double)* total_dimension);
#ifdef NATIVE_C
#else
    m_pgBar->setValue(100);
#endif
    if (dimension_value.size() == 4)  // xy-space
    {
        struct _mesh_2d m2d = this->get_mesh_2d();

        std::vector<long> dims(4);  // required order of dimensions: time, nbedlayers, nsedtot, nFaces
        bool correct_order = true;
        std::vector<long> dim_to;
        dim_to.resize(dimension_value.size());
        for (int j = 0; j < dimension_value.size(); j++)
        {
            if (dimension_name[j] == m_map_dim_name["time"])
            {
                dims[0] = m_map_dim[m_map_dim_name["time"]];
                dim_to[j] = 0;
                if (j != 0)
                {
                    correct_order = false;
                }
            }
            else if (dimension_name[j] == m_map_dim_name["zs_dim_bed_layer"])
            {
                dims[1] = m_map_dim[m_map_dim_name["zs_dim_bed_layer"]];
                dim_to[j] = 1;
                if (j != 1)
                {
                    correct_order = false;
                }
            }
            else if (dimension_name[j] == "nSedTot")  // Todo: HACK, use m_map_dim_name["nSedTot"])
            {
                dims[2] = m_map_dim["nSedTot"];
                dim_to[j] = 2;
                if (j != 2)
                {
                    correct_order = false;
                }
            }
            else
            {
                if (location == "edge")
                {
                    dims[3] = m2d.edge_x.size();
                }
                if (location == "face")
                {
                    dims[3] = m2d.face_x.size();
                }
                if (location == "node")
                {
                    dims[3] = m2d.node_x.size();
                }
                dim_to[j] = 3;
                if (j != 3)
                {
                    correct_order = false;
                }
            }
        }

        // set the array dimensions in the order: time, bed/hydro_layer, sediment, xy_dim
        if (correct_order)
        {
            //memcpy(values_c, read_c, length * sizeof(double));  // Todo: HACK, assumed to be the right order
        }
        else
        {
            //values_c = permute_array(read_c, dim_to, dims);
        }
        error_code = dvp.set_dimensions(dims[0], dims[1], dims[2], dims[3]);  //  time_dim, layer_dim, sed_dim, xy_dim);

        for (int j = 0; j < dimension_name.size(); j++)
        {
            if (m_map_dim_name["z_sigma_interface"] == m_mesh_vars[i_var]->dim_names[j])
            {
#ifdef NATIVE_C
                fprintf(stderr, "    3D data on interfaces not yet supported.\n");
#else
                QMessageBox::warning(0, QString("Warning"), QString("3D data on interfaces not yet supported,\ndata: %1").arg(var_name.c_str()));
#endif
                return 0;
            }

        }
        error_code = dvp.set_data(var_name, data_vector);
        m_mesh_vars[i_var]->read = true;
#ifdef NATIVE_C
#else
        m_pgBar->setValue(1000);
        m_pgBar->hide();
#endif
    }
    STOP_TIMER(read_variable_dvp4);
    return 0;  // if read, skip all remaining variables
}
//------------------------------------------------------------------------------
std::vector<_variable_ugridapi*>  UGRIDAPI_WRAPPER::get_variables()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::get_variables()" << std::endl;
#endif    
    return m_vars;
}
//------------------------------------------------------------------------------
std::vector<_variable_ugridapi*> UGRIDAPI_WRAPPER::get_variables_1d()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::get_variables_1d()" << std::endl;
#endif    
    return m_vars_1d;
}
//------------------------------------------------------------------------------
std::vector<_variable_ugridapi*> UGRIDAPI_WRAPPER::get_variables_1d2d()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::get_variables_1d2d()" << std::endl;
#endif    
    return m_vars_1d2d;
}
//------------------------------------------------------------------------------
std::vector<_variable_ugridapi*> UGRIDAPI_WRAPPER::get_variables_2d()
{
#ifdef NATIVE_C
    std::cerr << "UGRIDAPI_WRAPPER::get_variables_2d()" << std::endl;
#endif    
    return m_vars_2d;
}
//------------------------------------------------------------------------------
#ifdef NATIVE_C
std::string UGRIDAPI_WRAPPER::get_filename()
#else
QFileInfo UGRIDAPI_WRAPPER::get_filename()
#endif
{
    return m_ugrid_filename;
}

//==============================================================================
// PRIVATE functions
//==============================================================================
int UGRIDAPI_WRAPPER::get_attribute(int ncid, int i_var, char* att_name, char** att_value)
{
    size_t length = 0;
    int status = -1;

    status = nc_inq_attlen(ncid, i_var, att_name, &length);
    *att_value = (char*)malloc(sizeof(char) * (length + 1));
    *att_value[0] = '\0';
    if (status != NC_NOERR)
    {
        *att_value = '\0';
    }
    else
    {
        status = nc_get_att(ncid, i_var, att_name, *att_value);
        att_value[0][length] = '\0';
    }
    return status;
}
//------------------------------------------------------------------------------
int UGRIDAPI_WRAPPER::get_attribute(int ncid, int i_var, char* att_name, std::string* att_value)
{
    size_t length = 0;
    int status = -1;

    status = nc_inq_attlen(ncid, i_var, att_name, &length);
    if (status != NC_NOERR)
    {
        *att_value = "";
    }
    else
    {
        char* tmp_value = (char*)malloc(sizeof(char) * (length + 1));
        tmp_value[0] = '\0';
        status = nc_get_att(ncid, i_var, att_name, tmp_value);
        tmp_value[length] = '\0';
        *att_value = std::string(tmp_value, length);
        free(tmp_value);
    }
    return status;
}
//------------------------------------------------------------------------------
int UGRIDAPI_WRAPPER::get_attribute(int ncid, int i_var, std::string att_name, std::string* att_value)
{
    size_t length = 0;
    int status = -1;

    status = nc_inq_attlen(ncid, i_var, att_name.c_str(), &length);
    if (status != NC_NOERR)
    {
        *att_value = "";
    }
    else
    {
        char* tmp_value = (char*)malloc(sizeof(char) * (length + 1));
        tmp_value[0] = '\0';
        status = nc_get_att(ncid, i_var, att_name.c_str(), tmp_value);
        tmp_value[length] = '\0';
        *att_value = std::string(tmp_value, length);
        free(tmp_value);
    }
    return status;
}
//------------------------------------------------------------------------------
int UGRIDAPI_WRAPPER::get_attribute(int ncid, int i_var, char* att_name, double* att_value)
{
    int status = -1;

    status = nc_get_att_double(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int UGRIDAPI_WRAPPER::get_attribute(int ncid, int i_var, char* att_name, int* att_value)
{
    int status = -1;

    status = nc_get_att_int(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int UGRIDAPI_WRAPPER::get_attribute(int ncid, int i_var, char* att_name, long* att_value)
{
    int status = -1;

    status = nc_get_att_long(ncid, i_var, att_name, att_value);

    return status;
}
std::string UGRIDAPI_WRAPPER::trim(std::string str)
{
    boost::algorithm::trim(str);
    return str;
}
int UGRIDAPI_WRAPPER::get_attribute_by_var_name(int ncid, std::string var_name, std::string att_name, std::string* att_value)
{
    int status = -1;
    int i_var;
    status = nc_inq_varid(ncid, var_name.c_str(), &i_var);
    status = get_attribute(ncid, i_var, att_name, att_value);
    return status;
}
long UGRIDAPI_WRAPPER::create_mesh1d_nodes(ugridapi::Mesh1D& mesh1d, struct _network_1d * ntw)
{
    long error_code = -1;

    double xp;
    double yp;
    std::vector<double> chainage;
    size_t start_address = 0;
    for (int branch = 0; branch < ntw->num_edges; branch++)  // loop over the geometries
    {
        double branch_length = ntw->edge_length[branch];
        size_t geom_nodes_count = ntw->node_count[branch];
        if (branch > 0) 
        {
            start_address += ntw->node_count[branch-1];
        }

        chainage.resize(geom_nodes_count);
        chainage[0] = 0.0;
        for (int i = 1; i < geom_nodes_count; i++)
        {
            double x1 = ntw->geometry_nodes_x[start_address + i - 1];
            double y1 = ntw->geometry_nodes_y[start_address + i - 1];
            double x2 = ntw->geometry_nodes_x[start_address + i];
            double y2 = ntw->geometry_nodes_y[start_address + i];

            chainage[i] = chainage[i - 1] + sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));  // Todo: HACK: this is just the euclidian distance
        }

        for (int i = 0; i < mesh1d.num_nodes; i++)  // loop over computational nodes
        {
            if (mesh1d.node_edge_id[i] == branch)  // is this node on this edge
            {
                double fraction = -1.0;
                double chainage_node = mesh1d.node_edge_offset[i];
                fraction = chainage_node / branch_length;
                if (fraction < 0.0 || fraction > 1.0)
                {
#ifdef NATIVE_C
                    fprintf(stderr, "UGRIDAPI_WRAPPER::determine_computational_node_on_geometry()\n    Branch(%d). Offset %f is larger then branch length %f.\n", branch + 1, chainage_node, branch_length);
#else
                    //QMessageBox::warning(0, "Message", QString("UGRIDAPI_WRAPPER::determine_computational_node_on_geometry()\nBranch(%3). Offset %1 is larger then branch length %2.\n").arg(offset).arg(branch_length).arg(branch+1));
#endif
                }
                double chainage_point = fraction * chainage[geom_nodes_count - 1];

                xp = ntw->geometry_nodes_x[start_address];
                yp = ntw->geometry_nodes_y[start_address];

                for (int j = 1; j < geom_nodes_count; j++)
                {
                    if (chainage_point <= chainage[j])
                    {
                        double alpha = (chainage_point - chainage[j - 1]) / (chainage[j] - chainage[j - 1]);
                        double x1 = ntw->geometry_nodes_x[start_address + j - 1];
                        double y1 = ntw->geometry_nodes_y[start_address + j - 1];
                        double x2 = ntw->geometry_nodes_x[start_address + j];
                        double y2 = ntw->geometry_nodes_y[start_address + j];

                        xp = x1 + alpha * (x2 - x1);
                        yp = y1 + alpha * (y2 - y1);
                        break;
                    }
                }
                mesh1d.node_x[i] = xp;
                mesh1d.node_y[i] = yp;
            }
        }
    }
    error_code = 0;
    return error_code;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::set_grid_mapping_epsg(long epsg, std::string epsg_code)
{
    long status = 0;
    m_mapping->epsg = epsg;
    m_mapping->epsg_code = epsg_code;
    return status;
}
//------------------------------------------------------------------------------
long UGRIDAPI_WRAPPER::read_times()
{
    long status{ -1 };
    int error_code = -1;

    // Look for variable with unit like: "seconds since 2017-02-25 15:26:00"
    double dt;

    QStringList date_time;
    QDateTime* RefDate;
    struct _time_series t_series;
    time_series.push_back(t_series);
    time_series[0].nr_times = 0;
    time_series[0].dim_name = nullptr;
    time_series[0].long_name = nullptr;
    time_series[0].unit = nullptr;

#ifndef NATIVE_C
    m_pgBar->setValue(700);
#endif

    // get the name of the time dimension 
    std::vector<std::string> variables;
    error_code = ugridapi::ug_variable_get_all_names(m_ncid, variables);
    for (int i = 0; i < variables.size(); ++i)
    {
        std::string att_value;
        error_code = ugridapi::ug_variable_get_attribute_value(m_ncid, variables[i], "units", att_value);
        if (att_value.find("since") != -1)
        {
            QString units = QString::fromStdString(att_value).replace("T", " ");  // "seconds since 1970-01-01T00:00:00" changed into "seconds since 1970-01-01 00:00:00"
            date_time = units.split(" ");
            if (date_time.at(1).toUtf8() == "since")
            {
                // now it is the time variable, can only be detected by the "seconds since 1970-01-01T00:00:00" character string
                // retrieve the long_name, standard_name -> var_name for the xaxis label

                error_code = ugridapi::ug_variable_get_attribute_value(m_ncid, variables[i], "long_name", att_value);
                if (error_code == 0)
                {
                    time_series[0].long_name = QString::fromStdString(att_value);
                }
                else
                {
                    time_series[0].long_name = QString::fromStdString(variables[i]);
                }
                std::vector<int> dimension_value;
                std::vector<std::string> dimension_name;
                error_code = ugridapi::ug_variable_get_data_dimensions(m_ncid, variables[i], dimension_name, dimension_value);
                // when it is a dimension: variables[i] == dimension_name
                time_series[0].dim_name = QString::fromStdString(dimension_name[0]);
                time_series[0].nr_times = dimension_value[0];
                // ex. date_time = "seconds since 2017-02-25 15:26:00"   year, yr, day, d, hour, hr, h, minute, min, second, sec, s and all plural forms
                time_series[0].unit = date_time.at(0);
                QDate date = QDate::fromString(date_time.at(2), "yyyy-MM-dd");
                QTime time = QTime::fromString(date_time.at(3), "hh:mm:ss");
                RefDate = new QDateTime(date, time, Qt::UTC);
#if defined(DEBUG)
                QString janm1 = date.toString("yyyy-MM-dd");
                QString janm2 = time.toString();
                QString janm3 = RefDate->toString("yyyy-MM-dd hh:mm:ss.zzz");
#endif
                time_series[0].times.resize(time_series[0].nr_times);
                error_code = ugridapi::ug_variable_get_data_double(m_ncid, variables[i], time_series[0].times);
                dt = 0.0;
                if (time_series[0].nr_times >= 2)
                {
                    dt = time_series[0].times[1] - time_series[0].times[0];
                }
                if (time_series[0].unit.contains("sec") ||
                    time_series[0].unit.trimmed() == "s")  // seconds, second, sec, s
                {
                    time_series[0].unit = QString("sec");
                }
                else if (time_series[0].unit.contains("min"))  // minutes, minute, min
                {
                    time_series[0].unit = QString("min");
                }
                else if (time_series[0].unit.contains("h"))  // hours, hour, hrs, hr, h
                {
                    time_series[0].unit = QString("hour");
                }
                else if (time_series[0].unit.contains("d"))  // days, day, d
                {
                    time_series[0].unit = QString("day");
                }
                for (int j = 0; j < time_series[0].nr_times; j++)
                {
                    if (time_series[0].unit.contains("sec") ||
                        time_series[0].unit.trimmed() == "s")  // seconds, second, sec, s
                    {
                        //QMessageBox::warning(NULL, "Warning", QString("RefDate: %1").arg(RefDate->addSecs(time_series[0].times[j]).toString()));
                        if (dt < 1.0)
                        {
                            qdt_times.append(RefDate->addMSecs(1000. * time_series[0].times[j]));  // milli seconds as smallest time unit
                        }
                        else
                        {
                            qdt_times.append(RefDate->addSecs(time_series[0].times[j]));  // seconds as smallest time unit
                        }
                    }
                    else if (time_series[0].unit.contains("min"))  // minutes, minute, min
                    {
                        qdt_times.append(RefDate->addSecs(time_series[0].times[j] * 60.0));
                    }
                    else if (time_series[0].unit.contains("h"))  // hours, hour, hrs, hr, h
                    {
                        qdt_times.append(RefDate->addSecs(time_series[0].times[j] * 3600.0));
                    }
                    else if (time_series[0].unit.contains("d"))  // days, day, d
                    {
                        qdt_times.append(RefDate->addSecs(time_series[0].times[j] * 24.0 * 3600.0));
                    }

#if defined (DEBUG)
                    QString janm;
                    if (dt < 1.0)
                    {
                        janm = qdt_times[j].toString("yyyy-MM-dd hh:mm:ss.zzz");
                    }
                    else
                    {
                        janm = qdt_times[j].toString("yyyy-MM-dd hh:mm:ss");
                    }
#endif
                }
                break;
            }
        }
    }
#ifndef NATIVE_C
    m_pgBar->setValue(800);
#endif

    return status;
}
//------------------------------------------------------------------------------
std::vector<std::string> UGRIDAPI_WRAPPER::tokenize(const std::string& s, char c) 
{
    auto end = s.cend();
    auto start = end;

    std::vector<std::string> v;
    for (auto it = s.cbegin(); it != end; ++it) {
        if (*it != c) {
            if (start == end)
                start = it;
            continue;
        }
        if (start != end) {
            v.emplace_back(start, it);
            start = end;
        }
    }
    if (start != end)
        v.emplace_back(start, end);
    return v;
}
//------------------------------------------------------------------------------
std::vector<std::string> UGRIDAPI_WRAPPER::tokenize(const std::string & str, std::size_t count)
{
    std::vector<std::string> tokens;
    if (str.size() == 0) { return tokens; }
    size_t parts = str.size() / count;
    for (size_t i = 0, offset = 0; i < parts; ++i)
    {
        size_t size = count;
        if ((offset + size) < str.size())
        {
            std::string tmp_str = str.substr(offset, size);

            auto end = std::remove(tmp_str.begin(), tmp_str.end(), '\0');
            tmp_str.erase(end, tmp_str.end());

            tokens.push_back(tmp_str);
        }
        else
        {
            std::string tmp_str = str.substr(offset, str.size() - offset);

            auto end = std::remove(tmp_str.begin(), tmp_str.end(), '\0');
            tmp_str.erase(end, tmp_str.end());

            tokens.push_back(tmp_str);
        }
        offset += size;
    }
    return tokens;
}
//------------------------------------------------------------------------------
std::vector<std::string> UGRIDAPI_WRAPPER::get_string_var(int ncid, std::string var_name)
{
    int varid;
    int ndims;
    int nr_names = 0;
    nc_type nc_type;
    int status = -1;
    std::vector<std::string> result;

    char* dim_name_c = (char*)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    dim_name_c[0] = '\0';

    if (var_name.size() != 0)
    {
        status = nc_inq_varid(ncid, var_name.c_str(), &varid);
        status = nc_inq_var(ncid, varid, NULL, &nc_type, &ndims, NULL, NULL);
        long* sn_dims = (long*)malloc(sizeof(long) * ndims);
        status = nc_inq_vardimid(ncid, varid, (int*)sn_dims);

        int mem_length = 1;
        size_t name_len = -1;
        for (long j = 0; j < ndims; j++)
        {
            size_t length = (size_t)-1;
            status = nc_inq_dim(this->m_ncid, sn_dims[j], dim_name_c, &length);

            if (strstr(dim_name_c, "len") || name_len == -1 && j == 1)  // second dimension is the string length if not already set
            {
                name_len = length;
            }
            else
            {
                nr_names = (long)length;
            }
            mem_length = mem_length * length;
        }
        result.reserve(nr_names);
        // reading 64 strings for each location, length of string??
        if (nc_type == NC_STRING)
        {
            char** location_strings = (char**)malloc(sizeof(char*) * (mem_length)+1);
            status = nc_get_var_string(ncid, varid, location_strings);
            QString janm = QString("JanM");
            for (int k = 0; k < nr_names; k++)
            {
                janm = QString("");
                for (int k2 = 0; k2 < name_len; k2++)
                {
                    QTextCodec* codec2 = QTextCodec::codecForName("UTF-8");
                    QString str = codec2->toUnicode(*(location_strings + k * name_len + k2));
                    janm = janm + str;
                }
                result.push_back(janm.toStdString());
            }
        }
        else if (nc_type == NC_CHAR)
        {
            char* location_chars = (char*)malloc(sizeof(char*) * (mem_length)+1);
            location_chars[0] = '\0';
            status = nc_get_var_text(ncid, varid, location_chars);
            char* janm = (char*)malloc(sizeof(char) * (name_len + 1));
            janm[name_len] = '\0';
            for (int k = 0; k < nr_names; k++)
            {
                strncpy(janm, location_chars + k * name_len, name_len);
                result.push_back(std::string(janm));
            }
            free(janm);
        }
        else
        {
            // trying to read unsupported variable
        }
        free(sn_dims);
    }
    free(dim_name_c);
    return result;
}
//------------------------------------------------------------------------------
size_t UGRIDAPI_WRAPPER::get_memory_length(std::string var_name, std::string attribute)
{
    size_t mem_length{ 1 };
    long status;
    int varid;
    int ndims;
    nc_type nc_type;

    std::string att_name;
    std::string tmp_name = trim(var_name);
    status = get_attribute_by_var_name(m_ncid, tmp_name, attribute, &att_name);
    status = nc_inq_varid(m_ncid, att_name.c_str(), &varid);

    if (status == NC_NOERR)
    {
        // att_name is a variable
        status = nc_inq_var(m_ncid, varid, NULL, &nc_type, &ndims, NULL, NULL);
        if (ndims != 0)
        {
            long* sn_dims = (long*)malloc(sizeof(long) * ndims);
            status = nc_inq_vardimid(m_ncid, varid, (int*)sn_dims);
            for (long j = 0; j < ndims; j++)
            {
                size_t length = (size_t)-1;
                status = nc_inq_dim(this->m_ncid, sn_dims[j], nullptr, &length);
                mem_length = mem_length * length;
            }
        }
        else
        {
            // att_name is a variable without dimension
            mem_length = att_name.size();
        }
    }
    else
    {
        // att_name is not a variable, just a string
        mem_length = att_name.size();
    }

    return mem_length;
}
//------------------------------------------------------------------------------
int UGRIDAPI_WRAPPER::get_start_index(int ncid, std::string var_name, std::string att_name)
{
    int i_var;
    int start_index = -2;
    long status = -1;
    std::string att_value;
    status = get_attribute_by_var_name(ncid, var_name, att_name, &att_value);
    std::vector<std::string> vars = tokenize(att_value, ' ');
    if (status == NC_NOERR)
    {
        for (int i = 0; i < vars.size(); ++i)
        {
            status = nc_inq_varid(ncid, vars[i].c_str(), &i_var);
            if (status == NC_NOERR)
            {
                status = nc_get_att_int(ncid, i_var, "start_index", &start_index);
                if (status == NC_NOERR) { break; }
            }
        }
    }
    return start_index;
}
double* UGRIDAPI_WRAPPER::permute_array(double* in_arr, std::vector<long> order, std::vector<long> dim)
{
    //
    // in_array: array which need to be permuted
    // order: order of the dimensions in the supplied array in_array
    // dim: dimensions in required order
    // return value: permuted array in required order
    //
    if (dim.size() != 4) {
        return nullptr;
    }
    long k_source;
    long k_target;
    long length = 1;
    std::vector<long> cnt(4);
    for (long i = 0; i < dim.size(); ++i)
    {
        length *= dim[i];
    }
    double* out_arr = (double*)malloc(sizeof(double) * length);
    for (long i = 0; i < dim[0]; i++) {
        cnt[0] = i;
        for (long j = 0; j < dim[1]; j++) {
            cnt[1] = j;
            for (long k = 0; k < dim[2]; k++) {
                cnt[2] = k;
                for (long l = 0; l < dim[3]; l++) {
                    cnt[3] = l;
                    k_source = get_index_in_c_array(cnt[order[0]], cnt[order[1]], cnt[order[2]], cnt[order[3]], dim[order[0]], dim[order[1]], dim[order[2]], dim[order[3]]);
                    k_target = get_index_in_c_array(i, j, k, l, dim[0], dim[1], dim[2], dim[3]);
                    out_arr[k_target] = in_arr[k_source];
                }
            }
        }
    }
    free(in_arr);
    return out_arr;
}
long UGRIDAPI_WRAPPER::get_index_in_c_array(long t, long l, long s, long xy, long time_dim, long layer_dim, long sed_dim, long xy_dim)
{
    Q_UNUSED(time_dim);
    return t * layer_dim * sed_dim * xy_dim + l * sed_dim * xy_dim + s * xy_dim + xy;
}
