#include "sgrid_ugrid_api.h"

extern "C" {
    //
    //namespace SGRID and UGRID (SUGRID)
    //
    UGRIDSGRID_API SUGRID* get_sugrid_obj_api(char* fname) {
        QString filename(_strdup(fname));
        return new SUGRID(filename, nullptr);
    }
    UGRIDSGRID_API long read_nc_file_api(SUGRID* grid, char* fname)
    {
        long ret_value = -1;
        //QString msg = QString("C++ function: \"%1\"; grid->read_nc_file_api()").arg(__FUNCTION__);
        //QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
        ret_value = grid->open(QFileInfo(fname), nullptr);
        ret_value = grid->read();
        return ret_value;
    }
    UGRIDSGRID_API long read_epsg_api(SUGRID* grid)
    {
        long epsg = -1;
        epsg = grid->get_epsg();
        return epsg;
    }
    UGRIDSGRID_API long get_edge_face_node_count_api(SUGRID* grid, int* n_edges, int* n_faces, int* n_nodes, int* n_face_nodes)
    {
        long status = -1;
        struct _mesh2d* m2d = grid->get_mesh_2d();
        *n_edges = (int)m2d->edge[0]->count;
        *n_faces = (int)m2d->face[0]->count;
        *n_nodes = (int)m2d->node[0]->count;
        *n_face_nodes = (int)m2d->face_nodes[0].size();
        return status;
    }
    UGRIDSGRID_API long get_xy_node_coordinate_api(SUGRID* grid, double** x, double** y)
    {
        long status = -1;
        struct _mesh2d* m2d = grid->get_mesh_2d();
        *x = m2d->node[0]->x.data();
        *y = m2d->node[0]->y.data();
        return status;
    }
    UGRIDSGRID_API long get_xy_face_coordinate_api(SUGRID* grid, double** x, double** y)
    {
        long status = -1;
        struct _mesh2d* m2d = grid->get_mesh_2d();
        *x = m2d->face[0]->x.data();
        *y = m2d->face[0]->y.data();
        return status;
    }
    UGRIDSGRID_API long get_edge_node_index_api(SUGRID* grid, int** edge_nodes)
    {
        long status = -1;
        struct _mesh2d* m2d = grid->get_mesh_2d();
        *edge_nodes = m2d->edge[0]->edge_nodes[0];
        return status;
    }
    UGRIDSGRID_API long get_face_nodes_index_api(SUGRID* grid, int** face_nodes)
    {
        long status = -1;
        struct _mesh2d* m2d = grid->get_mesh_2d();
        int n_faces = m2d->face_nodes.size();
        int nodes_per_face = m2d->face_nodes[0].size();
        int * f_nodes = (int*)malloc(sizeof(int) * n_faces * nodes_per_face);
        int k = 0;
        for (int j = 0; j < n_faces; ++j)
        {
            for (int i = 0; i < nodes_per_face; ++i)
            {
                k = j * nodes_per_face + i;
                f_nodes[k] = m2d->face_nodes[j][i];
            }
        }
        *face_nodes = f_nodes;
        return status;
    }
        
    UGRIDSGRID_API long get_edge_type_count_api(SUGRID* grid, int* edge_type_count)
    {
        long ret_value = -1;
        struct _mesh_variable* vars = grid->get_variables();
        std::vector<char*> flag_m;
        for (int i = 0; i < vars->nr_vars; i++)
        {
            _variable* var = vars->variable[i];
            if (!var->time_series && var->coordinates != "")
            {
                std::string str = var->long_name;
                std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                if (str.find("edge type") != std::string::npos)  // && var->location == "edge" && var->topology_dimension == 2 && var->nc_type == NC_INT)
                {
                    *edge_type_count = (int)var->flag_values.size();
                    ret_value = 0;
                    break;
                }
            }
        }
        return ret_value;
    }
    UGRIDSGRID_API long get_edge_type_api(SUGRID* grid, int** flag_values, double** z_value)
    {
        long status = -1;
        struct _mesh_variable* vars = grid->get_variables();
        std::vector<char*> flag_m;
        std::string meaning_str;
        for (int i = 0; i < vars->nr_vars; i++)
        {
            _variable* var = vars->variable[i];
            if (!var->time_series && var->coordinates != "")
            {
                std::string str = var->long_name;
                std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                if (str.find("edge type") != std::string::npos)  // && var->location == "edge" && var->topology_dimension == 2 && var->nc_type == NC_INT)
                {
                    DataValuesProvider2D<double>std_data_at_edge = grid->get_variable_values(var->var_name);
                    *z_value = std_data_at_edge.GetValueAtIndex(0, 0);
                    *flag_values = var->flag_values.data();
                    for (int i = 0; i < var->flag_meanings.size(); ++i)
                    {
                        meaning_str += var->flag_meanings[i].data();
                        if (i < var->flag_meanings.size() - 1) { str += " "; }
                        int a = 1;
                    }
                    break;
                }
            }
        }
        return status;
    }
    UGRIDSGRID_API char * get_face_value_stationary_api(SUGRID* grid, int index, double** z_value)
    {
        long status = -1;
        struct _mesh_variable* vars = grid->get_variables();
        std::vector<char*> flag_m;
        std::string meaning_str;
        char * bname = nullptr;
        int indx = -1;
        for (int i = 0; i < vars->nr_vars; i++)
        {
            _variable* var = vars->variable[i];
            if (!var->time_series && var->coordinates != "")
            {
                if (var->location == "face" && var->topology_dimension == 2 && var->nc_type == NC_DOUBLE)
                {
                    indx += 1;
                    if (indx == index)  // choose requested stationary variable
                    {
                        bname = var->long_name.data();
                        if (var->long_name.size() == 0)
                        {
                            bname = var->standard_name.data();
                            if (var->standard_name.size() == 0)
                            {
                                bname = var->standard_name.data();
                            }
                        }
                        // var->sediment_index == -1
                        DataValuesProvider2D<double>std_data_at_face = grid->get_variable_values(var->var_name, -1);
                        *z_value = std_data_at_face.GetValueAtIndex(0, 0);
                        status = 0;
                        break;
                    }
                }
            }
        }
        return bname;
    }
    UGRIDSGRID_API char* get_face_value_api(SUGRID* grid, int index, int current_step, double** z_value)
    {
        long status = -1;
        struct _mesh_variable* vars = grid->get_variables();
        std::vector<char*> flag_m;
        std::string meaning_str;
        char* bname = nullptr;
        int indx = -1;
        for (int i = 0; i < vars->nr_vars; i++)
        {
            _variable* var = vars->variable[i];
            if (var->time_series && var->coordinates != "")
            {
                if (var->location == "face" && var->topology_dimension == 2 && var->nc_type == NC_DOUBLE)
                {
                    indx += 1;
                    if (i == index)  // choose requested stationary variable
                    {
                        bname = var->long_name.data();
                        if (var->long_name.size() == 0)
                        {
                            bname = var->standard_name.data();
                            if (var->standard_name.size() == 0)
                            {
                                bname = var->standard_name.data();
                            }
                        }
                        // var->sediment_index == -1
                        DataValuesProvider2D<double>std_data_at_face = grid->get_variable_values(var->var_name, -1);
                        *z_value = std_data_at_face.GetValueAtIndex(current_step, 0);
                        status = 0;
                        break;
                    }
                }
            }
        }
        return bname;
    }
    UGRIDSGRID_API char* get_node_value_stationary_api(SUGRID* grid, int index, double** z_value)
    {
        long status = -1;
        struct _mesh_variable* vars = grid->get_variables();
        char* bname = nullptr;
        int indx = -1;
        for (int i = 0; i < vars->nr_vars; i++)
        {
            _variable* var = vars->variable[i];
            if (!var->time_series && var->coordinates != "")
            {
                if (var->location == "node" && var->topology_dimension == 2 && var->nc_type == NC_DOUBLE)
                {
                    indx += 1;
                    if (indx == index)  // choose requested stationary variable
                    {
                        bname = var->long_name.data();
                        if (var->long_name.size() == 0)
                        {
                            bname = var->standard_name.data();
                            if (var->standard_name.size() == 0)
                            {
                                bname = var->standard_name.data();
                            }
                        }
                        // var->sediment_index == -1
                        DataValuesProvider2D<double>std_data_at_face = grid->get_variable_values(var->var_name, -1);
                        *z_value = std_data_at_face.GetValueAtIndex(0, 0);
                        status = 0;
                        break;
                    }
                }
            }
        }
        return bname;
    }
    UGRIDSGRID_API long get_times_count_api(SUGRID* grid, int* count)
    {
        long status = -1;
        *count = grid->get_times_count();
        status = 0;
        return status;
    }
    UGRIDSGRID_API char * get_times_unit_api(SUGRID* grid)
    {
        char * str = grid->get_times_unit();
        return str;
    }
    UGRIDSGRID_API long get_times_api(SUGRID* grid, double ** times)
    {
        long status = -1;
        *times = grid->get_times()->data();
        status = 0;
        return status;
    }
    UGRIDSGRID_API long get_var_count_api(SUGRID* grid, int* count)
    {
        long status = 0;
        struct _mesh_variable* vars = grid->get_variables();
        *count = vars->nr_vars;
        return status;
    }
    UGRIDSGRID_API char * get_var_name_2d_scalar_api(SUGRID* grid, int index)
    {
        char * str = nullptr;
        struct _mesh_variable* vars = grid->get_variables();
        _variable* var = vars->variable[index];
        str = _strdup("");
        if (var->time_series && var->coordinates != "")  // it is a time-series
        {
            if (var->dims.size() == 2 ||
                var->dims.size() == 3 && grid->get_file_type() == FILE_TYPE::SGRID )
                str = _strdup(var->long_name.data());
        }
        return str;
    }
    UGRIDSGRID_API char* get_location_2d_scalar_api(SUGRID* grid, int index)
    {
        char* str = nullptr;
        struct _mesh_variable* vars = grid->get_variables();
        _variable* var = vars->variable[index];
        str = _strdup("");
        if (var->time_series && var->coordinates != "")  // it is a time-series
        {
            if (var->dims.size() == 2 ||
                var->dims.size() == 3 && grid->get_file_type() == FILE_TYPE::SGRID)
                str = _strdup(var->location.data());
        }
        return str;
    }
    UGRIDSGRID_API char* get_unit_2d_scalar_api(SUGRID* grid, int index)
    {
        char* str = nullptr;
        struct _mesh_variable* vars = grid->get_variables();
        _variable* var = vars->variable[index];
        str = _strdup("");
        if (var->time_series && var->coordinates != "")  // it is a time-series
        {
            if (var->dims.size() == 2 ||
                var->dims.size() == 3 && grid->get_file_type() == FILE_TYPE::SGRID)
                str = _strdup(var->units.data());
        }
        return str;
    }
    UGRIDSGRID_API char* get_var_name_3d_scalar_api(SUGRID* grid, int index)
    {
        char* str = nullptr;
        struct _mesh_variable* vars = grid->get_variables();
        _variable* var = vars->variable[index];
        str = _strdup("");
        if (var->time_series && var->coordinates != "")  // it is a time-series
        {
            if (var->dims.size() == 3)
                str = _strdup(var->long_name.data());
        }
        return str;
    }
    UGRIDSGRID_API char* get_location_3d_scalar_api(SUGRID* grid, int index)
    {
        char* str = nullptr;
        struct _mesh_variable* vars = grid->get_variables();
        _variable* var = vars->variable[index];
        str = _strdup("");
        if (var->time_series && var->coordinates != "")  // it is a time-series
        {
            if (var->dims.size() == 3)
                str = _strdup(var->location.data());
        }
        return str;
    }

    UGRIDSGRID_API char* get_unit_3d_scalar_api(SUGRID* grid, int index)
    {
        char* str = nullptr;
        struct _mesh_variable* vars = grid->get_variables();
        _variable* var = vars->variable[index];
        str = _strdup("");
        if (var->time_series && var->coordinates != "")  // it is a time-series
        {
            if (var->dims.size() == 3)
                str = _strdup(var->units.data());
        }
        return str;
    }

}
