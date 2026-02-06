#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>

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

#include "ugrid.h"
#include "netcdf.h"
#include "perf_timer.h"

//------------------------------------------------------------------------------
UGRID::UGRID()
{
    m_nr_mesh_contacts = 0;
    _two = 2;
}
UGRID::UGRID(QFileInfo filename, int ncid, QProgressBar* pgBar)
{
    long ret_value = 1;
    m_fname = filename;  // filename without path, just the name
    m_ugrid_file_name = filename.absoluteFilePath();  // filename with complete path
    m_ncid = ncid;
    m_pgBar = pgBar;

    m_nr_mesh_contacts = 0;
    _two = 2;
}


//------------------------------------------------------------------------------
UGRID::~UGRID()
{
#ifdef NATIVE_C
    free(this->fname);
    free(this->ugrid_file_name);
#endif    
    int status;
    status = nc_close(m_ncid);
    if (status == NC_NOERR)
    {
        // free DataValueProvider2D3D
        for (int i = 0; i < m_nr_mesh_var; i++)
        {
            free(m_mesh_vars->variable[m_nr_mesh_var - 1]->data_2d.m_arrayPtr);
            m_mesh_vars->variable[m_nr_mesh_var - 1]->data_2d.m_arrayPtr = nullptr;
            free(m_mesh_vars->variable[m_nr_mesh_var - 1]->data_3d.m_arrayPtr);
            m_mesh_vars->variable[m_nr_mesh_var - 1]->data_3d.m_arrayPtr = nullptr;
        }
    }
}
//------------------------------------------------------------------------------
long UGRID::open(QFileInfo filename, QProgressBar* pgBar)
{
    long ret_value = 1;
    m_fname = filename;  // filename without path, just the name
    m_ugrid_file_name = filename.absoluteFilePath();  // filename with complete path
    m_pgBar = pgBar;

#ifdef NATIVE_C
    int status = nc_open(m_ugrid_file_name, NC_NOWRITE, &this->m_ncid);
    if (status != NC_NOERR)
    {
        fprintf(stderr, "UGRID::read()\n    Failed to open file: %s\n", m_ugrid_file_name);
        return ret_value;
    }
    fprintf(stderr, "UGRID::read()\n    Opened: %s\n", m_ugrid_file_name);
#else
    char* ug_fname = strdup(m_fname.absoluteFilePath().toUtf8());
    int status = nc_open(ug_fname, NC_NOWRITE, &m_ncid);
    if (status != NC_NOERR)
    {
        QMessageBox::critical(0, QString("Error"), QString("UGRID::read()\n    Failed to open file: %1").arg(ug_fname));
        return ret_value;
    }
    free(ug_fname);
    ug_fname = nullptr;
#endif
    status = this->read_global_attributes();

    ret_value = 0;
    return ret_value;
}
//------------------------------------------------------------------------------
long UGRID::read()
{
    int status = -1;

    START_TIMERN(Read mesh);
    status = this->read_mesh();
    STOP_TIMER(Read mesh);

    START_TIMER(Read times);
    status = this->read_times();
    STOP_TIMER(Read times);

    START_TIMER(Read variables);
    status = this->read_variables();
    STOP_TIMER(Read variables);

    return status;
}
//------------------------------------------------------------------------------
long UGRID::read_global_attributes()
{
    long status;
    int ndims, nvars, natts, nunlimited;
    nc_type att_type;
    size_t att_length;

#ifdef NATIVE_C
    fprintf(stderr, "UGRID::read_global_attributes()\n");
#endif    

    char * att_name_c = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    att_name_c[0] = '\0';

    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);

    this->global_attributes = (struct _global_attributes *)malloc(sizeof(struct _global_attributes));
    this->global_attributes->count = natts;
    this->global_attributes->attribute = (struct _global_attribute **)malloc(sizeof(struct _global_attribute *) * natts);
    for (long i = 0; i < natts; i++)
    {
        this->global_attributes->attribute[i] = new _global_attribute;
    }

    for (long i = 0; i < natts; i++)
    {
        status = nc_inq_attname(this->m_ncid, NC_GLOBAL, i, att_name_c);
        status = nc_inq_att(this->m_ncid, NC_GLOBAL, att_name_c, &att_type, &att_length);
        this->global_attributes->attribute[i]->name = std::string(strdup(att_name_c));
        this->global_attributes->attribute[i]->type = att_type;
        this->global_attributes->attribute[i]->length = att_length;
        if (att_type == NC_CHAR)
        {
            char * att_value_c = (char *)malloc(sizeof(char) * (att_length + 1));
            att_value_c[0] = '\0';
            status = nc_get_att_text(this->m_ncid, NC_GLOBAL, att_name_c, att_value_c);
            att_value_c[att_length] = '\0';
            this->global_attributes->attribute[i]->cvalue = std::string(strdup(att_value_c));
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
long UGRID::get_global_attribute_value(std::string att_name, std::string* att_value)
{
    long status = 1;  // error by default
    bool att_name_found = false;
    for (int i = 0; i < global_attributes->count; i++)
    {
        if (att_name_found) { exit; }
        if (global_attributes->attribute[i]->name == att_name)
        {
            att_name_found = true;
            *att_value = global_attributes->attribute[i]->cvalue;
            *att_value = std::string(global_attributes->attribute[i]->cvalue);
            status = 0;
        }
    }
    return status;
}
#ifdef NATIVE_C
//------------------------------------------------------------------------------
char * UGRID::get_filename()
{
    return this->m_ugrid_file_name;
}
#else
//------------------------------------------------------------------------------
QFileInfo UGRID::get_filename()
{
    return m_ugrid_file_name;
}
#endif
//------------------------------------------------------------------------------
long UGRID::read_mesh()
{
    int ndims, nvars, natts, nunlimited;
    int unlimid;
    std::string cf_role;
    std::string grid_mapping_name;
    std::string std_name;
    std::vector<std::string> tmp_dim_names;
    int status = 1;
    size_t length;

#ifdef NATIVE_C
    fprintf(stderr, "UGRID::read_mesh()\n");
#else
    m_pgBar->setValue(15);
#endif    

    char * var_name_c = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    m_mapping = new _mapping();
    set_grid_mapping_epsg(0, "EPSG:0");

    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);
    m_dimids = (size_t *)malloc(sizeof(size_t) * ndims);
    for (int i = 0; i < ndims; i++)
    {
        status = nc_inq_dimlen(this->m_ncid, i, &m_dimids[i]);
        status = nc_inq_dimname(this->m_ncid, i, var_name_c);
        m_dim_names.push_back(std::string(var_name_c));
        m_map_dim[std::string(var_name_c)] = m_dimids[i];
    }
    status = nc_inq_unlimdim(this->m_ncid, &unlimid);
    for (int i_var = 0; i_var < nvars; i_var++)
    {
        nc_type nc_type;
        nc_inq_varndims(this->m_ncid, i_var, &ndims);
        int * var_dimids = NULL;
        if (ndims > 0) {
            var_dimids = (int *)malloc(sizeof(int) * ndims);
        }
        status = nc_inq_var(this->m_ncid, i_var, var_name_c, &nc_type, &ndims, var_dimids, &natts);
        std::string var_name(var_name_c);
#ifdef NATIVE_C
        fprintf(stderr, "UGRID::read_mesh()\n    Variable name: %d - %s\n", i_var + 1, var_name.c_str());
#else
        m_pgBar->setValue(20 + (i_var + 1) / nvars * (500 - 20));
        //QMessageBox::warning(0, QString("Warning"), QString("UGRID::get_grid_mapping()\nProgress: (%1, %2) %3").arg(i_var + 1).arg(nvars).arg(var_name));
#endif
        length = 0;
        status = get_attribute(this->m_ncid, i_var, "cf_role", &cf_role);
        if (status == NC_NOERR)
        {
            status = read_variables_with_cf_role(i_var, var_name, cf_role, ndims, var_dimids);
        }

        status = get_attribute(this->m_ncid, i_var, "standard_name", &std_name);
        if (std_name == "ocean_sigma_coordinate")
        {
            // this grid contains sigma layers, find sigma dimension name
            tmp_dim_names = get_dimension_names(this->m_ncid, var_name);
            for (int i = 0; i < tmp_dim_names.size(); i++)
            {
                if (tmp_dim_names[i].find("nterface") != std::string::npos)  // Todo: HACK: dimension name should have the sub-string 'interface' or 'Interface'
                {
                    m_map_dim_name["zs_dim_interface"] = tmp_dim_names[i];
                    m_map_dim_name["zs_name_interface"] = var_name;
                }
                else
                {
                    m_map_dim_name["zs_dim_layer"] = tmp_dim_names[i];
                    m_map_dim_name["zs_name_layer"] = var_name;
                }
            }
        }
        else if (std_name == "altitude")
        {
            // this grid contains z- layers, find z-dimension name
            tmp_dim_names = get_dimension_names(this->m_ncid, var_name);
            for (int i = 0; i < tmp_dim_names.size(); i++)
            {
                if (tmp_dim_names[i].find("nterface") != std::string::npos)
                {
                    m_map_dim_name["zs_dim_interface"] = tmp_dim_names[i];
                    m_map_dim_name["zs_name_interface"] = var_name;
                }
                else if (tmp_dim_names[i].find("Layer") != std::string::npos)
                {
                    m_map_dim_name["zs_dim_layer"] = tmp_dim_names[i];
                    m_map_dim_name["zs_name_layer"] = var_name;
                }
            }
        }
        else
        {
            tmp_dim_names = get_dimension_names(this->m_ncid, var_name);
            for (int i = 0; i < tmp_dim_names.size(); i++)
            {
                if (tmp_dim_names[i].find("nBedLayers") != std::string::npos)
                {
                    m_map_dim_name["zs_dim_bed_layer"] = tmp_dim_names[i];
                    m_map_dim_name["zs_name_bed_layer"] = var_name;
                }
            }
        }

        status = get_attribute(this->m_ncid, i_var, "grid_mapping_name", &grid_mapping_name);
        if (status == NC_NOERR)
        {
            status = read_grid_mapping(i_var, var_name, grid_mapping_name);
        }
        free(var_dimids);
        var_dimids = nullptr;
    }

#ifndef NATIVE_C
    m_pgBar->setValue(600);
#endif
    status = determine_mesh1d_edge_length(m_mesh1d, m_ntw_edges);
    status = create_mesh1d_nodes(m_mesh1d, m_ntw_edges, m_ntw_geom);

    if (m_mesh_contact != nullptr)
    {
        // create the mesh contact edges
        // llog first for the meshes (ie the var_names) and contact points
        std::string mesh_a;
        std::string mesh_b;
        std::string location_a;
        std::string location_b;
        int topology_a;
        int topology_b;

        int i = 0;
        for (int i_var = 0; i_var < nvars; i_var++)
        {
            status = nc_inq_varname(this->m_ncid, i_var, var_name_c);
            std::string var_name(var_name_c);
            if (var_name == m_mesh_contact->mesh_a)
            {
                mesh_a = m_mesh_contact->mesh_a;
                location_a = m_mesh_contact->location_a;
                status = get_attribute(this->m_ncid, i_var, const_cast<char*>("topology_dimension"), &topology_a);
                i++;
            }
            if (var_name == m_mesh_contact->mesh_b)
            {
                mesh_b = m_mesh_contact->mesh_b;
                location_b = m_mesh_contact->location_b;
                status = get_attribute(this->m_ncid, i_var, const_cast<char*>("topology_dimension"), &topology_b);
                i++;
            }
            if (i == 2)
            {
                break;
            }
        }
        // looking for mesh_a

        m_mesh_contact->node[0]->x = std::vector<double>(m_mesh_contact->node[m_nr_mesh_contacts - 1]->count);
        m_mesh_contact->node[0]->y = std::vector<double>(m_mesh_contact->node[m_nr_mesh_contacts - 1]->count);

        for (int j = 0; j < m_mesh_contact->edge[0]->count; j++)
        {
            if (topology_a == 1)
            {
                int p1 = m_mesh_contact->edge[0]->edge_nodes[j][0];
                if (location_a == "node")
                {
                    m_mesh_contact->node[0]->x[2 * j] = m_mesh1d->node[0]->x[p1];
                    m_mesh_contact->node[0]->y[2 * j] = m_mesh1d->node[0]->y[p1];
                }
            }
            else if (topology_a == 2)
            {
                int p1 = m_mesh_contact->edge[0]->edge_nodes[j][0];
                if (location_a == "node")
                {
                    m_mesh_contact->node[0]->x[2 * j] = m_mesh2d->node[0]->x[p1];
                    m_mesh_contact->node[0]->y[2 * j] = m_mesh2d->node[0]->y[p1];
                }
                if (location_a == "face")
                {
                    m_mesh_contact->node[0]->x[2 * j] = m_mesh2d->face[0]->x[p1];
                    m_mesh_contact->node[0]->y[2 * j] = m_mesh2d->face[0]->y[p1];
                }
            }
            m_mesh_contact->edge[0]->edge_nodes[j][0] = 2 * j;
        }
        // looking for mesh_b
        for (int j = 0; j < m_mesh_contact->edge[0]->count; j++)
        {
            if (topology_b == 1)
            {
                int p1 = m_mesh_contact->edge[0]->edge_nodes[j][1];
                if (location_b == "node")
                {
                    m_mesh_contact->node[0]->x[2 * j + 1] = m_mesh1d->node[0]->x[p1];
                    m_mesh_contact->node[0]->y[2 * j + 1] = m_mesh1d->node[0]->y[p1];
                }
            }
            if (topology_b == 2)
            {
                int p1 = m_mesh_contact->edge[0]->edge_nodes[j][1];
                if (location_b == "node")
                {
                    m_mesh_contact->node[0]->x[2 * j + 1] = m_mesh2d->node[0]->x[p1];
                    m_mesh_contact->node[0]->y[2 * j + 1] = m_mesh2d->node[0]->y[p1];
                }
                else if (location_b == "face")
                {
                    m_mesh_contact->node[0]->x[2 * j + 1] = m_mesh2d->face[0]->x[p1];
                    m_mesh_contact->node[0]->y[2 * j + 1] = m_mesh2d->face[0]->y[p1];
                }
            }
            m_mesh_contact->edge[0]->edge_nodes[j][1] = 2 * j + 1;
        }
        length = m_mesh_contact->node[0]->name.size();
        length += m_mesh_contact->node[0]->long_name.size();
        length += m_mesh_contact->edge[0]->name.size();
        length += m_mesh_contact->edge[0]->long_name.size();
    }
    //status = create_mesh_contacts(mesh_a, location_a, mesh_b, location_b);

#ifndef NATIVE_C
    m_pgBar->setValue(700);
#endif
    free(var_name_c);
    var_name_c = nullptr;

    return status;
}
//------------------------------------------------------------------------------
long UGRID::set_grid_mapping_epsg(long epsg, std::string epsg_code)
{
    long status = 0;
    m_mapping->epsg = epsg;
    m_mapping->epsg_code = epsg_code;
    return status;
}
//------------------------------------------------------------------------------
long UGRID::read_times()
{
    // Look for variable with unit like: "seconds since 2017-02-25 15:26:00"
    int ndims, nvars, natts, nunlimited;
    int status;
    char * var_name_c;
    char * units_c;
    size_t length;
    int dimids;
    int nr_time_series = 0;
    double dt;

    QStringList date_time;
    char * time_var_name;
    QDateTime * RefDate;
    struct _time_series t_series;
    time_series.push_back(t_series);
    time_series[0].nr_times = 0;
    time_series[0].dim_name = nullptr;
    time_series[0].long_name = nullptr;
    time_series[0].unit = nullptr;

#ifndef NATIVE_C
    m_pgBar->setValue(700);
#endif
    var_name_c = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    var_name_c[0] = '\0';
    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);
    for (long i_var = 0; i_var < nvars; i_var++)
    {
        length = (size_t) -1;
        status = nc_inq_attlen(this->m_ncid, i_var, "units", &length);
        if (status == NC_NOERR)
        {
            units_c = (char *)malloc(sizeof(char) * (length + 1));
            units_c[length] = '\0';
            status = nc_get_att(this->m_ncid, i_var, "units", units_c);
            QString units = QString(units_c).replace("T", " ");  // "seconds since 1970-01-01T00:00:00" changed into "seconds since 1970-01-01 00:00:00"
            date_time = units.split(" ");
            if (date_time.count() >= 2)
            {
                if (!strcmp("since", date_time.at(1).toUtf8()))
                {
                    // now it is the time variable, can only be detected by the "seconds since 1970-01-01T00:00:00" character string
                    // retrieve the long_name, standard_name -> var_name for the xaxis label
                    length = (size_t) -1;
                    status = nc_inq_var(this->m_ncid, i_var, var_name_c, NULL, &ndims, &dimids, &natts);
                    status = nc_inq_dimname(this->m_ncid, dimids, var_name_c);
                    time_series[0].dim_name = new QString(var_name_c);
                    status = nc_inq_attlen(this->m_ncid, i_var, "long_name", &length);
                    if (status == NC_NOERR)
                    {
                        char * c_label = (char *)malloc(sizeof(char) * (length + 1));
                        c_label[length] = '\0';
                        status = nc_get_att(this->m_ncid, i_var, "long_name", c_label);
                        time_series[0].long_name = new QString(c_label);
                        free(c_label); 
                        c_label = nullptr;
                    }
                    else
                    {
                        time_series[0].long_name = new QString(var_name_c);
                    }
                    //status = get_dimension_names(this->m_ncid, var_name_c, &time_series[0].dim_name);

                    // retrieve the time series
                    nr_time_series += 1;
                    if (nr_time_series > 1)
                    {
#if defined NATIVE_C
#else
                        QMessageBox::warning(NULL, QObject::tr("Warning"), QObject::tr("Support of just one time series, only the first one will be supported.\nPlease mail to jan.mooiman@deltares.nl"));
#endif
                        continue;
                    }

                    status = nc_inq_dimlen(this->m_ncid, dimids, &time_series[0].nr_times);
                    time_var_name = strdup(var_name_c);
                    m_map_dim[time_var_name] = time_series[0].nr_times;
                    m_map_dim_name["time"] = time_var_name;

                    qdt_times.reserve((int)time_series[0].nr_times);  // Todo: HACK typecast
                    // ex. date_time = "seconds since 2017-02-25 15:26:00"   year, yr, day, d, hour, hr, h, minute, min, second, sec, s and all plural forms
                    time_series[0].unit = new QString(date_time.at(0));

                    QDate date = QDate::fromString(date_time.at(2), "yyyy-MM-dd");
                    QTime time = QTime::fromString(date_time.at(3), "hh:mm:ss");
                    RefDate = new QDateTime(date, time, Qt::UTC);

#if defined(DEBUG)
                    QString janm1 = date.toString("yyyy-MM-dd");
                    QString janm2 = time.toString();
                    QString janm3 = RefDate->toString("yyyy-MM-dd hh:mm:ss.zzz");
#endif
                    time_series[0].times.resize(time_series[0].nr_times);
                    status = nc_get_var_double(this->m_ncid, i_var, time_series[0].times.data());
                    dt = 0.0;
                    if (time_series[0].nr_times >= 2)
                    {
                        dt = time_series[0].times[1] - time_series[0].times[0];
                    }

                    if (time_series[0].unit->contains("sec") ||
                        time_series[0].unit->trimmed() == "s")  // seconds, second, sec, s
                    {
                        time_series[0].unit = new QString("sec");
                    }
                    else if (time_series[0].unit->contains("min"))  // minutes, minute, min
                    {
                        time_series[0].unit = new QString("min");
                    }
                    else if (time_series[0].unit->contains("h"))  // hours, hour, hrs, hr, h
                    {
                        time_series[0].unit = new QString("hour");
                    }
                    else if (time_series[0].unit->contains("d"))  // days, day, d
                    {
                        time_series[0].unit = new QString("day");
                    }
                    for (int j = 0; j < time_series[0].nr_times; j++)
                    {
                        if (time_series[0].unit->contains("sec") ||
                            time_series[0].unit->trimmed() == "s")  // seconds, second, sec, s
                        {
                            //QMessageBox::warning(NULL, "Warning", QString("RefDate: %1").arg(RefDate->addSecs(time_series[0].times[j]).toString()));
                            if (dt < 1.0)
                            {
                                qdt_times.append(RefDate->addMSecs(1000.*time_series[0].times[j]));  // milli seconds as smallest time unit
                            }
                            else
                            {
                                qdt_times.append(RefDate->addSecs(time_series[0].times[j]));  // seconds as smallest time unit
                            }
                        }
                        else if (time_series[0].unit->contains("min"))  // minutes, minute, min
                        {
                            qdt_times.append(RefDate->addSecs(time_series[0].times[j] * 60.0));
                        }
                        else if (time_series[0].unit->contains("h"))  // hours, hour, hrs, hr, h
                        {
                            qdt_times.append(RefDate->addSecs(time_series[0].times[j] * 3600.0));
                        }
                        else if (time_series[0].unit->contains("d"))  // days, day, d
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
            free(units_c);
            units_c = nullptr;
        }
    }
    free(var_name_c); 
    var_name_c = nullptr;
#ifndef NATIVE_C
    m_pgBar->setValue(800);
#endif

    return (long)status;
}
//------------------------------------------------------------------------------
long UGRID::read_variables()
{
    // read the attributes of the variable  and the dimensions
#ifdef NATIVE_C
    fprintf(stderr, "UGRID::read_variables()\n");
#endif    
    int ndims, nvars, natts, nunlimited;
    int status;
    struct _variable * var;
    var = NULL;
#ifndef NATIVE_C
    m_pgBar->setValue(800);
#endif
    m_nr_mesh_var = 0;

    std::string tmp_string;
    char * var_name_c = (char *) malloc(sizeof(char) * (NC_MAX_NAME + 1));
    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);

    for (int i_var = 0; i_var < nvars; i_var++)
    {
        nc_type nc_type;
        nc_inq_varndims(this->m_ncid, i_var, &ndims);
        int * var_dimids = NULL;
        if (ndims > 0) {
            var_dimids = (int *)malloc(sizeof(int) * ndims);
        }
        status = nc_inq_var(this->m_ncid, i_var, var_name_c, &nc_type, &ndims, var_dimids, &natts);
        std::string var_name(var_name_c);
        status = get_attribute(this->m_ncid, i_var, "mesh", &tmp_string);  // statement, to detect if this is a variable on a mesh
        if (status == NC_NOERR) { status = get_attribute(this->m_ncid, i_var, "location", &tmp_string); } // each variable does have a location
        if (status == NC_NOERR)
        {
            // This variable is defined on a mesh and has a dimension
            m_nr_mesh_var += 1;
            if (m_nr_mesh_var == 1)
            {
                m_mesh_vars = new _mesh_variable();
                m_mesh_vars->variable = (struct _variable **)malloc(sizeof(struct _variable *));
            }
            else
            {
                m_mesh_vars->variable = (struct _variable **)realloc(m_mesh_vars->variable, sizeof(struct _variable *) * m_nr_mesh_var);
            }
            m_mesh_vars->variable[m_nr_mesh_var - 1] = new _variable();
            m_mesh_vars->nr_vars = m_nr_mesh_var;

            m_mesh_vars->variable[m_nr_mesh_var - 1]->var_name = var_name;
            m_mesh_vars->variable[m_nr_mesh_var - 1]->nc_type = nc_type;
            m_mesh_vars->variable[m_nr_mesh_var - 1]->read = false;
            status = get_attribute(this->m_ncid, i_var, "location", &m_mesh_vars->variable[m_nr_mesh_var - 1]->location);
            status = get_attribute(this->m_ncid, i_var, "mesh", &m_mesh_vars->variable[m_nr_mesh_var - 1]->mesh);
            status = get_attribute(this->m_ncid, i_var, "coordinates", &m_mesh_vars->variable[m_nr_mesh_var - 1]->coordinates);
            status = get_attribute(this->m_ncid, i_var, "cell_methods", &m_mesh_vars->variable[m_nr_mesh_var - 1]->cell_methods);
            status = get_attribute(this->m_ncid, i_var, "standard_name", &m_mesh_vars->variable[m_nr_mesh_var - 1]->standard_name);
            status = get_attribute(this->m_ncid, i_var, "long_name", &m_mesh_vars->variable[m_nr_mesh_var - 1]->long_name);
            if (status != NC_NOERR || m_mesh_vars->variable[m_nr_mesh_var - 1]->long_name.size() <=1 )
            {
                m_mesh_vars->variable[m_nr_mesh_var - 1]->long_name = m_mesh_vars->variable[m_nr_mesh_var - 1]->standard_name;
                if ( m_mesh_vars->variable[m_nr_mesh_var - 1]->long_name.size() <= 1)
                {
                    m_mesh_vars->variable[m_nr_mesh_var - 1]->long_name = m_mesh_vars->variable[m_nr_mesh_var - 1]->var_name;
                }
            }
            status = get_attribute(this->m_ncid, i_var, "units", &m_mesh_vars->variable[m_nr_mesh_var - 1]->units);
            status = get_attribute(this->m_ncid, i_var, "grid_mapping", &m_mesh_vars->variable[m_nr_mesh_var - 1]->grid_mapping);
            status = get_attribute(this->m_ncid, i_var, const_cast<char*>("_FillValue"), &m_mesh_vars->variable[m_nr_mesh_var - 1]->fill_value);
            if (status != NC_NOERR)
            {
                m_mesh_vars->variable[m_nr_mesh_var - 1]->fill_value = std::numeric_limits<double>::quiet_NaN();
            }
            status = get_attribute(this->m_ncid, i_var, "comment", &m_mesh_vars->variable[m_nr_mesh_var - 1]->comment);

            // variable with flag_values and flag_meanings
            std::string flag_meanings;
            status = get_attribute(this->m_ncid, i_var, "flag_meanings", &flag_meanings);
            if (status == NC_NOERR)
            {
                size_t length;
                status = nc_inq_attlen(this->m_ncid, i_var, "flag_values", &length);
                int* flag_values_c = (int*)malloc(sizeof(int) * length);
                status = get_attribute(this->m_ncid, i_var, const_cast<char*>("flag_values"), flag_values_c);
                for (int j = 0; j < length; ++j)
                {
                    m_mesh_vars->variable[m_nr_mesh_var - 1]->flag_values.push_back(flag_values_c[j]);
                }
                m_mesh_vars->variable[m_nr_mesh_var - 1]->flag_meanings = tokenize(flag_meanings, ' ');
            }

            m_mesh_vars->variable[m_nr_mesh_var - 1]->draw = false;
            m_mesh_vars->variable[m_nr_mesh_var - 1]->time_series = false;
            for (int j = 0; j < ndims; j++)
            {
                status = nc_inq_dimname(this->m_ncid, var_dimids[j], var_name_c);
                
                m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.push_back((long)m_dimids[var_dimids[j]]);  // Todo: HACK typecast: size_t -> long
                if (time_series[0].nr_times != 0 && QString::fromUtf8(m_dim_names[var_dimids[j]].c_str()) == time_series[0].dim_name)
                {
                    m_mesh_vars->variable[m_nr_mesh_var - 1]->time_series = true;
                }
                m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names.push_back(m_dim_names[var_dimids[j]]);
            }

            int topo_dim;
            int mesh_id;
            status = nc_inq_varid(this->m_ncid, m_mesh_vars->variable[m_nr_mesh_var - 1]->mesh.c_str(), &mesh_id);
            status = get_attribute(this->m_ncid, mesh_id, const_cast<char*>("topology_dimension"), &topo_dim);
            m_mesh_vars->variable[m_nr_mesh_var - 1]->topology_dimension = topo_dim;
            if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.size() == 1)
            {
                // nothing to do
            }
            if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.size() == 2)
            {
                bool contains_time_dimension = false;
                for (int i = 0; i < m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.size(); i++)
                {
                    // check if one of the dimension is the time dimension
                    if (QString::fromUtf8(m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i].c_str()) == time_series[0].dim_name)
                    {
                        contains_time_dimension = true;
                        break;
                    }
                }
                if (contains_time_dimension)
                {
                    // nothing special: 2D (time, xy-space)
                }
                else
                {
                    // special: 2D (nSedTot, xy-space)
                    for (int i = 0; i < m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.size(); i++)
                    {
                        if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i] == "nSedTot")  // todo: hard coded sediment dimension
                        {
                            std::vector<std::string> sed_name = get_string_var(this->m_ncid, "sedfrac_name");
                            int nr_sedtot = m_mesh_vars->variable[m_nr_mesh_var - 1]->dims[i];
                            for (int j = 0; j < nr_sedtot; j++)
                            {
                                if (j > 0)
                                {
                                    m_nr_mesh_var += 1;
                                    m_mesh_vars->variable = (struct _variable **)realloc(m_mesh_vars->variable, sizeof(struct _variable *) * m_nr_mesh_var);
                                    m_mesh_vars->variable[m_nr_mesh_var - 1] = new _variable();
                                    m_mesh_vars->nr_vars = m_nr_mesh_var;
                                }
                                std::stringstream ss;
                                std::streamsize nsig = int(log10(nr_sedtot)) + 1;
                                ss << std::setfill('0') << std::setw(nsig) << j + 1;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->draw = false;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->time_series = false;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->mesh = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->mesh;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->location = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->location;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->var_name = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->var_name;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->coordinates = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->coordinates;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->topology_dimension = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->topology_dimension;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->nc_type = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->nc_type;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->sediment_index = j;
                                std::string janm = var_name + "- " + sed_name[j] + " - " + "nSedTot";
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->long_name = strdup(janm.c_str());
                            }
                        }
                    }
                }
            }
            if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.size() == 3)
            {
                bool contains_time_dimension = false;
                for (int i = 0; i < m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.size(); i++)
                {
                    // check if one of the dimension is the time dimension
                    if (QString::fromUtf8(m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i].c_str()) == time_series[0].dim_name)
                    {
                        contains_time_dimension = true;
                        break;
                    }
                }
                if (contains_time_dimension)
                {
                    for (int i = 0; i < m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.size(); i++)
                    {
                        if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i] == m_map_dim_name["zs_dim_layer"])
                        {
                            int nr_layers = m_map_dim[m_map_dim_name["zs_dim_layer"]];
                            m_mesh_vars->variable[m_nr_mesh_var - 1]->nr_hydro_layers = nr_layers;

                            std::string name = m_map_dim_name["zs_name_layer"];
                            int ii;
                            status = nc_inq_varid(m_ncid, name.c_str(), &ii);
                            if (status == NC_NOERR)
                            {
                                double * values_c = (double *)malloc(sizeof(double) * nr_layers);
                                status = nc_get_var_double(this->m_ncid, ii, values_c);
                                std::vector<double> values;
                                values.reserve(nr_layers);
                                for (int j = 0; j < nr_layers; j++)
                                {
                                    values.push_back(*(values_c + j));
                                }
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->layer_center = values;
                            }
                            else
                            {
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->layer_center.resize(nr_layers, std::numeric_limits<double>::quiet_NaN());
                            }
                        }
                        else if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i] == m_map_dim_name["zs_dim_interface"])
                        {
                            int nr_layers = m_map_dim[m_map_dim_name["zs_dim_interface"]];
                            m_mesh_vars->variable[m_nr_mesh_var - 1]->nr_hydro_layers = nr_layers;

                            std::string name = m_map_dim_name["zs_name_interface"];
                            int ii;
                            status = nc_inq_varid(m_ncid, name.c_str(), &ii);
                            if (status == NC_NOERR)
                            {
                                double* values_c = (double*)malloc(sizeof(double) * nr_layers);
                                status = nc_get_var_double(this->m_ncid, ii, values_c);
                                std::vector<double> values;
                                values.reserve(nr_layers);
                                for (int j = 0; j < nr_layers; j++)
                                {
                                    values.push_back(*(values_c + j));
                                }
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->layer_center = values;
                            }
                            else
                            {
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->layer_center.resize(nr_layers, std::numeric_limits<double>::quiet_NaN());
                            }
                        } 
                        else if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i] == m_map_dim_name["zs_dim_bed_layer"])
                        {
                            int nr_layers = m_map_dim[m_map_dim_name["zs_dim_bed_layer"]];
                            m_mesh_vars->variable[m_nr_mesh_var - 1]->nr_bed_layers = nr_layers;

                            std::string name = m_map_dim_name["zs_name_bed_layer"];
                            std::vector<double> values;
                            values.reserve(nr_layers);
                            for (int j = 0; j < nr_layers; j++)
                            {
                                values.push_back(j+1);
                            }
                            m_mesh_vars->variable[m_nr_mesh_var - 1]->layer_center = values;
                        }
                        else if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i] == "nSedTot")  // todo: hard coded sediment dimension
                        {
                            // because it is not a 3D dimesional data set no layer information, just time, space and constituent:
                            // generate new variables for each dimesnion which s not the time and space dimension
                            // in this example it nSedTot extra variables
                            // - time
                            // - sediment dimension
                            // - space dimension

                            std::vector<std::string> sed_name = get_string_var(this->m_ncid, "sedfrac_name");
                            int nr_sedtot = m_mesh_vars->variable[m_nr_mesh_var - 1]->dims[i];
                            for (int j = 0; j < nr_sedtot; j++)
                            {
                                if (j > 0)
                                {
                                    m_nr_mesh_var += 1;
                                    m_mesh_vars->variable = (struct _variable **)realloc(m_mesh_vars->variable, sizeof(struct _variable *) * m_nr_mesh_var);
                                    m_mesh_vars->variable[m_nr_mesh_var - 1] = new _variable();
                                    m_mesh_vars->nr_vars = m_nr_mesh_var;
                                }
                                std::stringstream ss;
                                std::streamsize nsig = int(log10(nr_sedtot)) + 1;
                                ss << std::setfill('0') << std::setw(nsig) << j+1;
                                std::string name_sed = "Sediment " + ss.str();
                                // reduce dimension (from 3 to 2)
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->draw = false;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->time_series = true;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->mesh = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->mesh;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->location = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->location;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->var_name = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->var_name;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->coordinates = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->coordinates;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->topology_dimension = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->topology_dimension;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->nc_type = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->nc_type;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->dims = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->dims;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->dim_names;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->sediment_index = j;
                                std::string janm = var_name + "- " + sed_name[j] + " - " + "nSedTot";
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->long_name = strdup(janm.c_str());

                                if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i] == m_map_dim_name["zs_dim_bed_layer"])
                                {
                                    int nr_layers = m_map_dim[m_map_dim_name["zs_dim_bed_layer"]];
                                    m_mesh_vars->variable[m_nr_mesh_var - 1]->nr_bed_layers = nr_layers;

                                    std::string name = m_map_dim_name["zs_name_bed_layer"];
                                    std::vector<double> values;
                                    values.reserve(nr_layers);
                                    for (int jj = 0; jj < nr_layers; jj++)
                                    {
                                        values.push_back(jj + 1);
                                    }
                                    m_mesh_vars->variable[m_nr_mesh_var - 1]->layer_center = values;
                                }
                            }
                        }
                        else if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i] == "nSedSus")  // todo: hard coded sediment dimension
                        {
                            // because it is not a 3D dimesional data set no layer information, just time, space and constituent:
                            // generate new variables for each dimesnion which s not the time and space dimension
                            // in this example it nSedTot extra variables
                            // - time
                            // - sediment dimension
                            // - space dimension

                            std::vector<std::string> sed_name;
                            int nr_sedtot = m_mesh_vars->variable[m_nr_mesh_var - 1]->dims[i];
                            for (int j = 0; j < nr_sedtot; j++)
                            {
                                std::stringstream ss;
                                std::streamsize nsig = int(log10(nr_sedtot)) + 1;
                                ss << std::setfill('0') << std::setw(nsig) << j + 1;
                                std::string janm = "Sediment " + ss.str();
                                sed_name.push_back(janm);
                            }
                            for (int j = 0; j < nr_sedtot; j++)
                            {
                                if (j > 0)
                                {
                                    m_nr_mesh_var += 1;
                                    m_mesh_vars->variable = (struct _variable **)realloc(m_mesh_vars->variable, sizeof(struct _variable *) * m_nr_mesh_var);
                                    m_mesh_vars->variable[m_nr_mesh_var - 1] = new _variable();
                                    m_mesh_vars->nr_vars = m_nr_mesh_var;
                                }
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->draw = false;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->time_series = true;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->mesh = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->mesh;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->location = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->location;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->var_name = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->var_name;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->coordinates = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->coordinates;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->topology_dimension = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->topology_dimension;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->nc_type = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->nc_type;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->dims = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->dims;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->dim_names;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->sediment_index = j;
                                std::string janm = var_name + "- " + sed_name[j] + " - " + "nSedSus";
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->long_name = strdup(janm.c_str());

                                if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i] == m_map_dim_name["zs_dim_bed_layer"])
                                {
                                    int nr_layers = m_map_dim[m_map_dim_name["zs_dim_bed_layer"]];
                                    m_mesh_vars->variable[m_nr_mesh_var - 1]->nr_bed_layers = nr_layers;

                                    std::string name = m_map_dim_name["zs_name_bed_layer"];
                                    std::vector<double> values;
                                    values.reserve(nr_layers);
                                    for (int jj = 0; jj < nr_layers; jj++)
                                    {
                                        values.push_back(jj + 1);
                                    }
                                    m_mesh_vars->variable[m_nr_mesh_var - 1]->layer_center = values;
                                }
                            }
                        }
                    }
                }
            }
            else if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.size() == 4)
            {
                // 4D variable:
                // - time
                // - sediment dimension
                // - xy-space dimension
                // - z-space (nbedlayers)
                QString qname = QString::fromUtf8(m_mesh_vars->variable[m_nr_mesh_var - 1]->var_name.c_str());
                QString msg = QString("Variable \'%1\' does have 4 dimensions, still under construction.").arg(qname) ;
#ifdef NATIVE_C
#else
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Warning, true);
#endif
                bool contains_time_dimension = false;
                for (int i = 0; i < m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.size(); i++)
                {
                    // check if one of the dimension is the time dimension
                    if (QString::fromUtf8(m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i].c_str()) == time_series[0].dim_name)
                    {
                        contains_time_dimension = true;
                        break;
                    }
                }
                if (contains_time_dimension)
                {
                    int nr_sedtot = -1;
                    int nr_layers = -1;
                    for (int i = 0; i < m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.size(); i++)
                    {
                        if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i] == "nSedTot")
                        {
                            nr_sedtot = m_mesh_vars->variable[m_nr_mesh_var - 1]->dims[i];
                        }
                        if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i] == "nBedLayers")
                        {
                            nr_layers = m_mesh_vars->variable[m_nr_mesh_var - 1]->dims[i];
                        }
                    }
                    for (int i = 0; i < m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.size(); i++)
                    {
                        if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[i] == "nSedTot")  // todo: hard coded sediment dimension
                        {
                            std::vector<std::string> sed_name = get_string_var(this->m_ncid, "sedfrac_name");
                            for (int j = 0; j < nr_sedtot; j++)
                            {
                                if (j > 0)
                                {
                                    m_nr_mesh_var += 1;
                                    m_mesh_vars->variable = (struct _variable **)realloc(m_mesh_vars->variable, sizeof(struct _variable *) * m_nr_mesh_var);
                                    m_mesh_vars->variable[m_nr_mesh_var - 1] = new _variable();
                                    m_mesh_vars->nr_vars = m_nr_mesh_var;
                                }
                                std::stringstream ss;
                                std::streamsize nsig = int(log10(nr_sedtot)) + 1;
                                ss << std::setfill('0') << std::setw(nsig) << j + 1;
                                std::string name_sed = "Sediment " + ss.str();
                                // reduce dimension (from 3 to 2)
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->draw = false;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->time_series = true;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->mesh = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->mesh;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->location = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->location;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->var_name = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->var_name;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->coordinates = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->coordinates;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->topology_dimension = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->topology_dimension;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->nc_type = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->nc_type;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->dims = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->dims;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names = m_mesh_vars->variable[m_nr_mesh_var - 1 - j]->dim_names;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->sediment_index = j;
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->nr_bed_layers = nr_layers;
                                std::string janm = var_name + "- " + sed_name[j] + " - " + "nSedTot";
                                m_mesh_vars->variable[m_nr_mesh_var - 1]->long_name = strdup(janm.c_str());

                                for (int ii = 0; ii < m_mesh_vars->variable[m_nr_mesh_var - 1]->dims.size(); ii++)
                                {
                                    if (m_mesh_vars->variable[m_nr_mesh_var - 1]->dim_names[ii] == m_map_dim_name["zs_dim_bed_layer"])
                                    {
                                        std::string name = m_map_dim_name["zs_name_bed_layer"];
                                        std::vector<double> values;
                                        values.reserve(nr_layers);
                                        for (int jj = 0; jj < nr_layers; jj++)
                                        {
                                            values.push_back(jj + 1);
                                        }
                                        m_mesh_vars->variable[m_nr_mesh_var - 1]->layer_center = values;
                                    }
                                }
                            }
                        }
                    }
                }
            }

             

            /*
            int nr_mesh2d = 1;
            if (var_name == mesh2d_strings[nr_mesh2d - 1]->x_bound_edge_name)
            {
                m_mesh_vars->variable[m_nr_mesh_var - 1]->location = "edge_boundary";
            }
            if (var_name == mesh2d_strings[nr_mesh2d - 1]->y_bound_edge_name)
            {
                m_mesh_vars->variable[m_nr_mesh_var - 1]->location = "edge_boundary";
            }
            if (var_name == mesh2d_strings[nr_mesh2d - 1]->x_bound_face_name)
            {
                m_mesh_vars->variable[m_nr_mesh_var - 1]->location = "face_boundary";
            }
            if (var_name == mesh2d_strings[nr_mesh2d - 1]->y_bound_face_name)
            {
                m_mesh_vars->variable[m_nr_mesh_var - 1]->location = "face_boundary";
            }
            */
        }
    }

    free(var_name_c);
    var_name_c = nullptr;
#ifndef NATIVE_C
    m_pgBar->setValue(900);
#endif

    return (long) status;
}
//------------------------------------------------------------------------------
struct _mesh_variable * UGRID::get_variables()
{
   return this->m_mesh_vars;
}
//------------------------------------------------------------------------------
struct _mesh_contact * UGRID::get_mesh_contact()
{
    return this->m_mesh_contact;
}
//------------------------------------------------------------------------------
struct _ntw_nodes * UGRID::get_connection_nodes()
{
    return this->m_ntw_nodes;
}
//------------------------------------------------------------------------------
struct _ntw_edges * UGRID::get_network_edges()
{
    return this->m_ntw_edges;
}
//------------------------------------------------------------------------------
struct _ntw_geom * UGRID::get_network_geometry()
{
    return this->m_ntw_geom;
}
//------------------------------------------------------------------------------
struct _mesh1d * UGRID::get_mesh_1d()
{
#ifdef NATIVE_C
    fprintf(stderr, "UGRID::get_mesh1d()\n");
#endif    
    return m_mesh1d;
}
//------------------------------------------------------------------------------
struct _mesh2d * UGRID::get_mesh_2d()
{
#ifdef NATIVE_C
    fprintf(stderr, "UGRID::get_mesh2d()\n");
#endif    
    return m_mesh2d;
}
//------------------------------------------------------------------------------
struct _mesh1d_string ** UGRID::get_mesh1d_string()
{
#ifdef NATIVE_C
    fprintf(stderr, "UGRID::get_mesh2d()\n");
#endif    
    return m_mesh1d_strings;
}
//------------------------------------------------------------------------------
struct _mesh2d_string ** UGRID::get_mesh2d_string()
{
#ifdef NATIVE_C
    fprintf(stderr, "UGRID::get_mesh2d()\n");
#endif    
    return m_mesh2d_strings;
}
//------------------------------------------------------------------------------
struct _mesh_contact_string ** UGRID::get_mesh_contact_string()
{
#ifdef NATIVE_C
    fprintf(stderr, "UGRID::get_mesh2d()\n");
#endif    
    return m_mesh_contact_strings;
}
//------------------------------------------------------------------------------
struct _mapping * UGRID::get_grid_mapping()
{
#ifdef NATIVE_C
    fprintf(stderr, "UGRID::get_grid_mapping()\n");
#else
    //QMessageBox::warning(0, QString("Warning"), QString("UGRID::get_grid_mapping()"));
#endif 
    return m_mapping;
}
//------------------------------------------------------------------------------
struct _variable * UGRID::get_var_by_std_name(struct _mesh_variable * vars, std::string mesh, std::string standard_name)
{
    for (int i = 0; i < vars->nr_vars; i++)
    {
        if (standard_name == vars->variable[i]->standard_name && mesh == vars->variable[i]->mesh)
        {
            return vars->variable[i];
        }
    }
    return nullptr;
}
//------------------------------------------------------------------------------
DataValuesProvider2D<double> UGRID::get_variable_values(const std::string var_name)
{
    // return: 1d dimensional value(x), ie time independent
    // return: 2d dimensional value(time, x)
    return get_variable_values(var_name, -1);
}
//------------------------------------------------------------------------------
DataValuesProvider2D<double> UGRID::get_variable_values(const std::string var_name, int array_index)
// return: 2d dimensional value(array, x)
{
    int var_id;
    int status;
    int i_var;
    nc_type var_type;
    DataValuesProvider2D<double> data_pointer;

#ifdef NATIVE_C
    fprintf(stderr, "UGRID::get_variable_values()\n");
#endif    

    for (int i = 0; i < m_mesh_vars->nr_vars; i++)
    {
        if (var_name == m_mesh_vars->variable[i]->var_name)
        {
            i_var = i;
            if (!m_mesh_vars->variable[i]->read)  // are the z_values already read
            {
                START_TIMERN(read_variable);
#ifdef NATIVE_C
#else
                m_pgBar->show();
                m_pgBar->setValue(0);
#endif
                status = nc_inq_varid(this->m_ncid, var_name.c_str(), &var_id);
                status = nc_inq_vartype(this->m_ncid, var_id, &var_type);
                size_t length = 1;
                for (int j = 0; j < m_mesh_vars->variable[i]->dims.size(); j++)
                {
                    length *= m_mesh_vars->variable[i]->dims[j];
                }
#ifdef NATIVE_C
#else
                m_pgBar->setValue(400);
#endif
                double * values_c = (double *)malloc(sizeof(double) * length);
                if (var_type == NC_DOUBLE)
                {
                    status = nc_get_var_double(this->m_ncid, var_id, values_c);
                }
                else if (var_type == NC_INT)
                {
                    // Todo: HACK, convert integer to double
                    int * values_i = (int *)malloc(sizeof(int) * length);
                    status = nc_get_var_int(this->m_ncid, var_id, values_i);
                    for (int ii = 0; ii < length; ++ii)
                    {
                        values_c[ii] = (double)values_i[ii];
                    }
                    free(values_i);

                }
#ifdef NATIVE_C
#else
                m_pgBar->setValue(800);
#endif
                if (m_mesh_vars->variable[i]->dims.size() == 1)
                {
                    long time_dim = 1;
                    long xy_dim = m_mesh_vars->variable[i]->dims[0];
                    DataValuesProvider2D<double> DataValuesProvider2D(values_c, time_dim, xy_dim);
                    m_mesh_vars->variable[i_var]->data_2d = DataValuesProvider2D;
                }
                else if (m_mesh_vars->variable[i]->dims.size() == 2)
                {
                    if (m_mesh_vars->variable[i]->time_series)
                    {
                        long time_dim = m_mesh_vars->variable[i]->dims[0];  // TODO: Assumed to be the time dimension
                        long xy_dim = m_mesh_vars->variable[i]->dims[1];  // TODO: Assumed to be the 2DH space dimension
                        DataValuesProvider2D<double> DataValuesProvider2D(values_c, time_dim, xy_dim);
                        m_mesh_vars->variable[i_var]->data_2d = DataValuesProvider2D;
                    }
                    else
                    {
                        // not a time series, one dimesion should be the xy-space
                        // but the other dimension is a list of array quantities (ex. type of sediments)
                        if (m_mesh_vars->variable[i]->location == "face")
                        {
                            for (int j = 0; j < m_mesh_vars->variable[i]->dims.size(); j++)
                            {
                                if (m_mesh2d->face[0]->x.size() == m_mesh_vars->variable[i]->dims[j])
                                {
                                    long time_dim = 1;
                                    if (array_index != -1)
                                    {
                                        time_dim = array_index + 1;
                                    }
                                    long xy_dim = m_mesh_vars->variable[i]->dims[j];
                                    DataValuesProvider2D<double> DataValuesProvider2D(values_c, time_dim, xy_dim);
                                    m_mesh_vars->variable[i_var]->data_2d = DataValuesProvider2D;
                                }
                            }
                        }
                    }
                }
                else if (m_mesh_vars->variable[i]->dims.size() == 3)
                {
                    continue;
                }
                else if (m_mesh_vars->variable[i]->dims.size() == 4)
                {
                    continue;
                }
                m_mesh_vars->variable[i]->read = true;
#ifdef NATIVE_C
#else
                m_pgBar->setValue(1000);
                m_pgBar->hide();
#endif
                STOP_TIMER(read_variable);

                return m_mesh_vars->variable[i_var]->data_2d; // variable value is found
            }
            return m_mesh_vars->variable[i_var]->data_2d;
        }
    }
    return data_pointer;
}
//==============================================================================
// PRIVATE functions
//==============================================================================
int UGRID::get_attribute_by_var_name(int ncid, std::string var_name, std::string att_name, std::string * att_value)
{
    int status = -1;
    int i_var;
    status = nc_inq_varid(ncid, var_name.c_str(), &i_var);
    status = get_attribute(ncid, i_var, att_name, att_value);
    return status;
}
//------------------------------------------------------------------------------
int UGRID::get_attribute(int ncid, int i_var, char * att_name, char ** att_value)
{
    size_t length = 0;
    int status = -1;

    status = nc_inq_attlen(ncid, i_var, att_name, &length);
    *att_value = (char *)malloc(sizeof(char) * (length + 1));
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
int UGRID::get_attribute(int ncid, int i_var, char * att_name, std::string * att_value)
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
        char * tmp_value = (char *)malloc(sizeof(char) * (length + 1));
        status = nc_get_att(ncid, i_var, att_name, tmp_value);
        tmp_value[length] = '\0';
        *att_value = std::string(tmp_value, length);
        free(tmp_value);
        tmp_value = nullptr;
    }
    return status;
}
//------------------------------------------------------------------------------
int UGRID::get_attribute(int ncid, int i_var, std::string att_name, std::string * att_value)
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
        char * tmp_value = (char *)malloc(sizeof(char) * (length + 1));
        status = nc_get_att(ncid, i_var, att_name.c_str(), tmp_value);
        tmp_value[length] = '\0';
        *att_value = std::string(tmp_value, length);
        free(tmp_value);
        tmp_value = nullptr;
    }
    return status;
}
//------------------------------------------------------------------------------
int UGRID::get_attribute(int ncid, int i_var, char * att_name, double * att_value)
{
    int status = -1;

    status = nc_get_att_double(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int UGRID::get_attribute(int ncid, int i_var, char * att_name, int * att_value)
{
    int status = -1;

    status = nc_get_att_int(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int UGRID::get_attribute(int ncid, int i_var, char * att_name, long * att_value)
{
    int status = -1;

    status = nc_get_att_long(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int UGRID::get_dimension(int ncid, char * dim_name, size_t * dim_length)
{
    int dimid;
    int status = -1;

    *dim_length = 0;
    if (dim_name != NULL && strlen(dim_name) != 0)
    {
        status = nc_inq_dimid(ncid, dim_name, &dimid);
        status = nc_inq_dimlen(ncid, dimid, dim_length);
    }
    return status;
}
//------------------------------------------------------------------------------
int UGRID::get_dimension(int ncid, std::string dim_name, size_t * dim_length)
{
    int dimid;
    int status = -1;

    *dim_length = 0;
    if (dim_name.size() != 0)
    {
        status = nc_inq_dimid(ncid, dim_name.c_str(), &dimid);
        status = nc_inq_dimlen(ncid, dimid, dim_length);
    }
    return status;
}
//------------------------------------------------------------------------------
int UGRID::get_dimension_var(int ncid, std::string var_name, size_t * dim_length)
{
    // get the total dimension length in bytes of the var_name variable
    int dimid;
    int status = -1;
    *dim_length = 0;

    if (var_name.size() != 0)
    {
        int janm;
        status = nc_inq_varid(ncid, var_name.c_str(), &dimid);
        status = nc_inq_vardimid(ncid, dimid, &janm);
        char * tmp_value = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));;
        status = nc_inq_dimname(ncid, janm, tmp_value);
        status = get_dimension(ncid, tmp_value, dim_length);
        free(tmp_value);
        tmp_value = nullptr;
    }
    return status;
}
std::vector<std::string>  UGRID::get_dimension_names(int ncid, std::string var_name)
{
    int varid;
    int status = -1;
    std::vector<std::string> dim_names;

    if (var_name.size() != 0)
    {
        int nr_dims;
        int * var_dimids = NULL;
        status = nc_inq_varid(ncid, var_name.c_str(), &varid);
        status = nc_inq_varndims(ncid, varid, &nr_dims);
        if (nr_dims > 0) {
            var_dimids = (int *)malloc(sizeof(int) * nr_dims);
        }
        status = nc_inq_var(ncid, varid, NULL, NULL, &nr_dims, var_dimids, NULL);
        for (int i = 0; i < nr_dims; i++)
        {
            //status = nc_inq_vardimid(this->m_ncid, dimid, janm);
            char * tmp_value = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));;
            status = nc_inq_dimname(ncid, var_dimids[i], tmp_value);
            dim_names.push_back(std::string(tmp_value));
            free(tmp_value);
            tmp_value = nullptr;
        }
    }
    return dim_names;
}


char * UGRID::strndup(const char *s, size_t n)
{
    char *result;
    size_t len = strlen(s);

    if (n < len)
        len = n;

    result = (char *)malloc(len + 1);
    if (!result)
        return 0;

    result[len] = '\0';
    return (char *)memcpy(result, s, len);
}




