#include <iostream>
#include <fstream>
#include <iomanip>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

#include "his_cf.h"
#include "netcdf.h"

//------------------------------------------------------------------------------
#ifdef NATIVE_C
    HISCF::HISCF(char * filename, int * dummy)
#else
    HISCF::HISCF(QFileInfo filename, QProgressBar * pgBar)
#endif
{
#ifdef NATIVE_C
    m_fname = strdup(filename);  // filename without path, just the name
    m_hiscf_file_name = strdup(filename);  // filename with complete path
#else
    m_fname = filename;  // filename without path, just the name
    m_hiscf_file_name = filename.absoluteFilePath();  // filename with complete path
    m_message_count = 0;
    _pgBar = pgBar;
#endif
}
//------------------------------------------------------------------------------
HISCF::~HISCF()
{
#ifdef NATIVE_C
    free(m_fname);
    free(m_hiscf_file_name);
#endif    
    (void)nc_close(this->m_ncid);

    // free the memory
    for (long i = 0; i < global_attributes->count; i++)
    {
        //delete global_attributes->attribute[i];
    }
    free(global_attributes->attribute);
    delete global_attributes;
}
//------------------------------------------------------------------------------
long HISCF::read()
{
    int status = -1;
#ifdef NATIVE_C
    status = nc_open(m_hiscf_file_name, NC_NOWRITE, &this->ncid);
    if (status != NC_NOERR)
    {
        fprintf(stderr, "HISCF::read()\n\tFailed to open file: %s\n", m_hiscf_file_name);
        return status;
    }
    fprintf(stderr, "HISCF::read()\n\tOpened: %s\n", m_hiscf_file_name);
#else
    char * hiscf_file_name_c = strdup(m_fname.absoluteFilePath().toUtf8());
    status = nc_open(hiscf_file_name_c, NC_NOWRITE, &this->m_ncid);
    if (status != NC_NOERR)
    {
        QMessageBox::critical(0, QString("Error"), QString("HISCF::read()\n\tFailed to open file: %1").arg(m_hiscf_file_name));
        return status;
    }
    free(hiscf_file_name_c);
    hiscf_file_name_c = nullptr;
#endif
    m_mapping = new _mapping();
    set_grid_mapping_epsg(0, "EPSG:0");
    status = this->read_global_attributes();
    status = this->read_locations();
    status = this->read_parameters();

    return status;
}
//------------------------------------------------------------------------------
long HISCF::read_global_attributes()
{
    long status;
    int ndims, nvars, natts, nunlimited;
    nc_type att_type;
    size_t att_length;

#ifdef NATIVE_C
    fprintf(stderr, "HISCF::read_global_attributes()\n");
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
        this->global_attributes->attribute[i]->name = std::string(att_name_c);
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
            fprintf(stderr, "\tAttribute nc_type: %d\n", att_type);
#endif    
        }
    }
    free(att_name_c);
    att_name_c = nullptr;

    return status;
}
//------------------------------------------------------------------------------
struct _global_attributes * HISCF::get_global_attributes()
{
#ifdef NATIVE_C
    fprintf(stderr, "HISCF::get_global_attributes()\n");
#endif    
    return this->global_attributes;
}
//------------------------------------------------------------------------------
std::vector<_location_type *> HISCF::get_observation_location()
{
#ifdef NATIVE_C
    fprintf(stderr, "HISCF::get_observation_location()\n");
#endif    
    return loc_type;
}
#ifdef NATIVE_C
//------------------------------------------------------------------------------
char * HISCF::get_filename()
{
    return m_hiscf_file_name;
}
#else
//------------------------------------------------------------------------------
QFileInfo HISCF::get_filename()
{
    return this->m_fname;
}
#endif
//------------------------------------------------------------------------------
long HISCF::read_locations()
{
    // look for variable with cf_role == timeseries_id, that are the locations
    int ndims, nvars, natts, nunlimited;
    long status;
    long location_name_varid;
    char * cf_role;
    char * dim_name_c;
    char * var_name_c;
    size_t length;
    size_t mem_length;
    long i_par_loc = -1;
    long nr_locations = -1;
    long * sn_dims; // station names dimensions
    bool model_wide_found = false;
    bool model_wide_exist = false;
    std::string grid_mapping_name;


    int nr_par_loc = 0;

    dim_name_c = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    dim_name_c[0] = '\0';
    var_name_c = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    var_name_c[0] = '\0';

    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);
    // cf_role = timeseries_id
    for (long i_var = 0; i_var < nvars; i_var++)
    {
        length = (size_t) -1;
        status = nc_inq_attlen(this->m_ncid, i_var, "cf_role", &length);
        if (status == NC_NOERR)
        {
            cf_role = (char *)malloc(sizeof(char) * (length + 1));
            cf_role[0] = '\0';
            status = nc_get_att(this->m_ncid, i_var, "cf_role", cf_role);
            cf_role[length] = '\0';

            if (!strcmp(cf_role, "timeseries_id"))
            {
                nc_type nc_type;
                location_name_varid = i_var;
                status = nc_inq_var(this->m_ncid, location_name_varid, var_name_c, &nc_type, &ndims, NULL, &natts);
                for (int i = 0; i < nr_par_loc; i++)
                {
                    if (!strcmp(loc_type[i]->location_var_name, var_name_c))
                    {
                        model_wide_found = true;  // voeg geen nieuwe stations toe, want model_wide zit er al in.
                        i_par_loc = i;
                        break;
                    }
                    if (!strcmp(loc_type[i]->location_var_name, "model_wide"))
                    {
                        // current var_name is not model_wide, if there is a model_wide location swap it to the last par_loc index
                        model_wide_exist = true;
                        i_par_loc = i;
                    }
                }

                if (!model_wide_found)
                {
                    loc_type.push_back(new _location_type);
                    nr_par_loc = (int)loc_type.size();  // Todo: HACK typecast
                    if (model_wide_exist)
                    {
                        struct _location_type * tmp = loc_type[i_par_loc];
                        loc_type[i_par_loc] = loc_type[nr_par_loc - 1];
                        loc_type[nr_par_loc - 1] = tmp;
                    }
                    else
                    {
                        i_par_loc = nr_par_loc - 1;
                    }
                }
                loc_type[i_par_loc]->location_var_name = strdup(var_name_c);
                length = (size_t) -1;
                status = nc_inq_attlen(this->m_ncid, location_name_varid, "long_name", &length);
                if (status == NC_NOERR)
                {
                    char * long_name = (char *)malloc(sizeof(char) * (length + 1));
                    long_name[0] = '\0';
                    status = nc_get_att(this->m_ncid, location_name_varid, "long_name", long_name);
                    long_name[length] = '\0';
                    loc_type[i_par_loc]->location_long_name = strdup(long_name);
                    free(long_name);
                }
                else
                {
                    loc_type[i_par_loc]->location_long_name = strdup(var_name_c);
                }

                sn_dims = (long *)malloc(sizeof(long)*ndims);
                status = nc_inq_vardimid(this->m_ncid, location_name_varid, (int*)sn_dims);

                mem_length = 1;
                name_len = (size_t) -1;
                for (long j = 0; j < ndims; j++)
                {
                    length = (size_t) -1;
                    status = nc_inq_dim(this->m_ncid, sn_dims[j], dim_name_c, &length);

                    if (strstr(dim_name_c, "len") || name_len == -1 && j == 1)  // second dimension is the string length if not already set
                    {
                        name_len = length;
                    }
                    else
                    {
                        loc_type[i_par_loc]->location_dim_name = strdup(dim_name_c);
                        nr_locations = (long)length;
                    }
                    mem_length = mem_length * length;
                }
                loc_type[i_par_loc]->location.reserve(nr_locations);
                // reading 64 strings for each location, length of string??
                if (nc_type == NC_STRING)
                {
                    char ** location_strings = (char **)malloc(sizeof(char *) * (mem_length)+1);
                    status = nc_get_var_string(this->m_ncid, location_name_varid, location_strings);
                    QString janm = QString("JanM");
                    for (int k = 0; k < nr_locations; k++)
                    {
                        janm = QString("");
                        for (int k2 = 0; k2 < name_len; k2++)
                        {
                            QTextCodec *codec2 = QTextCodec::codecForName("UTF-8");
                            QString str = codec2->toUnicode(*(location_strings + k * name_len + k2));
                            janm = janm + str;
                        }
                        struct _location loc;
                        loc.name = janm;
                        loc_type[i_par_loc]->location.push_back(loc);
                    }
                    loc_type[i_par_loc]->x_geom_location_string = std::string("---");
                    loc_type[i_par_loc]->y_geom_location_string = std::string("---");
                    free(location_strings);
                }
                else if (nc_type == NC_CHAR)
                {
                    char * location_chars = (char *)malloc(sizeof(char) * (mem_length)+1);
                    location_chars[0] = '\0';
                    status = nc_get_var_text(this->m_ncid, location_name_varid, location_chars);
                    char * janm = (char *)malloc(sizeof(char)*(name_len + 1));
                    janm[name_len] = '\0';
                    for (int k = 0; k < nr_locations; k++)
                    {
                        strncpy(janm, location_chars + k * name_len, name_len);
                        struct _location loc;
                        loc.name = janm;
                        loc_type[i_par_loc]->location.push_back(loc);
                    }
                    loc_type[i_par_loc]->x_geom_location_string = std::string("---");
                    loc_type[i_par_loc]->y_geom_location_string = std::string("---");
                    free(janm);
                    free(location_chars);
                }
                else
                {
                    // trying to read unsupported variable
                }
                free(sn_dims);
            }
            free(cf_role);
        }
        else
        {
            status = get_attribute(this->m_ncid, i_var, "grid_mapping_name", &grid_mapping_name);
            if (status == NC_NOERR)
            {
                status = read_grid_mapping(i_var, var_name_c, grid_mapping_name);
            }
        }
    }
    free(var_name_c);
    return status;
}
//------------------------------------------------------------------------------
long HISCF::read_parameters()
{
    // Read the parameters with dimension nr_times only (that is a time series without location, so model wide) This is a special case!
    // Read the parameters with coordinate as defined by the variables with attribute timeseries_id
    int ndims, nvars, natts, nunlimited;
    int status;
    size_t length;
    char * var_name_c;
    std::string coord;
    std::string geometry;
    std::string att_value;
    std::string coord_geom;

    size_t * par_dim;
    long i_par_loc = -1;
    int i_geom_id = -1;
    int i_geom_node = -1;
    size_t geom_node_count;
    int * geom_count;
    std::vector<bool> timeseries_geom_found;
    for (int i = 0; i < loc_type.size(); i++)
    {
        timeseries_geom_found.push_back(false);
    }

    status = -1;

    var_name_c = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    var_name_c[0] = '\0';

    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);
    for (long i_var = 0; i_var < nvars; i_var++)
    {
        long * par_dim_ids;  // parameter dimensions

        length = (size_t) -1;
        i_par_loc = -1;
        status = nc_inq_var(this->m_ncid, i_var, var_name_c, NULL, &ndims, NULL, &natts);

        par_dim = (size_t *)malloc(sizeof(long)*ndims);
        par_dim_ids = (long *)malloc(sizeof(long)*ndims);
        status = nc_inq_vardimid(this->m_ncid, i_var, (int*)par_dim_ids);

        status = get_attribute(this->m_ncid, i_var, "coordinates", &coord);
        if (status == NC_NOERR)
        {
            status = get_attribute(this->m_ncid, i_var, "geometry", &geometry);
            // if status != NC_NOERR then it is an old format
            if (status == NC_NOERR)
            {
                std::vector<std::string> token = tokenize(coord, ' ');
                //  loop over the variables with the cf_role == "timeseries_id"
                for (int j = 0; j < loc_type.size(); j++)
                {
                    bool found = false;
                    for (int k = 0; k < token.size(); k++)
                    {
                        if (loc_type[j]->location_var_name == token[k])
                        {
                            found = true;
                        }
                    }
                    if (!found) { continue; }
                    if (timeseries_geom_found[j]) { continue; }
                    // only variables with the attribute "coordinates" and "geometry" are treated
                    status = nc_inq_varid(this->m_ncid, geometry.c_str(), &i_geom_id);
                    status = get_attribute(this->m_ncid, i_geom_id, "geometry_type", &att_value);
                    if (att_value == "point")
                    {
                        timeseries_geom_found[j] = true;
                        loc_type[j]->type = OBS_POINT;
                        // point location, observation points
                        status = get_attribute(this->m_ncid, i_geom_id, "node_count", &att_value);
                        status = get_dimension_var(this->m_ncid, att_value, &geom_node_count);
                        geom_count = (int *)malloc(sizeof(int) * geom_node_count);
                        status = nc_inq_varid(this->m_ncid, att_value.c_str(), &i_geom_node);
                        status = nc_get_var_int(this->m_ncid, i_geom_node, geom_count);
                        for (int i = 0; i < geom_node_count; i++)
                        {
                            loc_type[j]->node_count.push_back(geom_count[i]);
                        }
                        //
                        status = get_attribute(this->m_ncid, i_geom_id, "node_coordinates", &coord_geom);
                        std::vector<std::string> token_geom = tokenize(coord_geom, ' ');
                        for (int i = 0; i < token_geom.size(); i++)  // var_x, var_y
                        {
                            int var_id = -1;
                            status = nc_inq_varid(this->m_ncid, token_geom[i].c_str(), &var_id);
                            status = get_attribute(this->m_ncid, var_id, "standard_name", &att_value);
                            if (att_value == "projection_x_coordinate" || att_value == "longitude")
                            {
                                loc_type[j]->x_geom_location_string = token_geom[i];
                            }
                            if (att_value == "projection_y_coordinate" || att_value == "latitude")
                            {
                                loc_type[j]->y_geom_location_string = token_geom[i];
                            }
                        }
                        for (int i = 0; i < token.size(); i++)  // tokens from the coordinates attribute of a quantity (i.e. water level)
                        {
                            int var_id = -1;
                            status = nc_inq_varid(this->m_ncid, token[i].c_str(), &var_id);
                            status = get_attribute(this->m_ncid, var_id, "standard_name", &att_value);
                            if (att_value == "projection_x_coordinate" || att_value == "longitude")
                            {
                                loc_type[j]->x_location_string = token[i];
                            }
                            if (att_value == "projection_y_coordinate" || att_value == "latitude")
                            {
                                loc_type[j]->y_location_string = token[i];
                            }
                        }
                    }
                    else if (att_value == "line")
                    {
                        timeseries_geom_found[j] = true;
                        loc_type[j]->type = OBS_POLYLINE;
                        // line location, cross sections
                        status = get_attribute(this->m_ncid, i_geom_id, "node_count", &att_value);
                        status = get_dimension_var(this->m_ncid, att_value, &geom_node_count);
                        geom_count = (int *)malloc(sizeof(int) * geom_node_count);
                        status = nc_inq_varid(this->m_ncid, att_value.c_str(), &i_geom_node);
                        status = nc_get_var_int(this->m_ncid, i_geom_node, geom_count);
                        for (int i = 0; i < geom_node_count; i++)
                        {
                            loc_type[j]->node_count.push_back(geom_count[i]);
                        }
                        //
                        status = get_attribute(this->m_ncid, i_geom_id, "node_coordinates", &coord_geom);
                        std::vector<std::string> token_geom = tokenize(coord_geom, ' ');
                        for (int i = 0; i < token_geom.size(); i++)  // var_x, var_y
                        {
                            int var_id = -1;
                            status = nc_inq_varid(this->m_ncid, token_geom[i].c_str(), &var_id);
                            status = get_attribute(this->m_ncid, var_id, "standard_name", &att_value);
                            if (att_value == "projection_x_coordinate" || att_value == "longitude")
                            {
                                loc_type[j]->x_location_string = token_geom[i];
                            }
                            if (att_value == "projection_y_coordinate" || att_value == "latitude")
                            {
                                loc_type[j]->y_location_string = token_geom[i];
                            }
                        }

                    }
                    else if (att_value != "")
                    {
#ifdef NATIVE_C
                        int a = 1;
#else
                        QMessageBox::information(0, QString("HISCF::read_parameters()"), QString("Just \'point\' and \'line\' are supported. But type \'%1\' given").arg(att_value.c_str()));
#endif
                    }
                }  // end location type, observation point, area or line
            }
            else
            {
#ifdef NATIVE_C
                int a = 1;
#else
                if (m_message_count < 1)
                {
                    QString msg = QString("Deprecated history file used, probably not all observation points and/or cross section are visualised.");
                    QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Warning, true);
                    m_message_count += 1;
                }

                //QMessageBox::information(0, QString("HISCF::read_parameters()"), QString("Old type not anymore supported. Attribute 'geometry' missing."));
#endif
                //  loop over the variables with the cf_role == "timeseries_id"
                std::vector<std::string> token = tokenize(coord, ' ');
                for (int j = 0; j < loc_type.size(); j++)
                {
                    // only variables with the attribute "coordinates" are treated
                    bool found = false;
                    loc_type[j]->node_count.clear();
                    if (token.size() == 1 && loc_type[j]->type == OBS_NONE && loc_type[j]->location_var_name == token[0])  // assume it is the cross_section
                    {
                        loc_type[j]->type = OBS_POLYLINE;
                        token.resize(3);
                        token[2] = token[0];
                        token[0] = "cross_section_x_coordinate";
                        token[1] = "cross_section_y_coordinate";
                        for (int i = 0; i < loc_type[j]->location.size(); i++)
                        {
                            loc_type[j]->node_count.push_back(3);  // 3 == cross_section_pts
                        }
                    }
                    for (int i_token = 0; i_token < token.size(); i_token++)  // var_x, var_y, var_name
                    {
                        // is one of the tokens equal to one of the loc_types?
                        if (!found && std::string(loc_type[j]->location_var_name) == token[i_token])
                        {
                            found = true;
                            if (loc_type[j]->type == OBS_NONE) { 
                                loc_type[j]->type = OBS_POINT; 
                                loc_type[j]->node_count.push_back(1);
                            }
                            // location_var_name is one of the tokens
                            // loop again over all tokens to find the x- and y- coordinates, ie doubling the token i_token
                            for (int i = 0; i < token.size(); i++)  // var_x, var_y, var_name
                            {
                                int var_id = -1;
                                status = nc_inq_varid(this->m_ncid, token[i].c_str(), &var_id);
                                status = get_attribute(this->m_ncid, var_id, "standard_name", &att_value);
                                if (status == NC_NOERR)
                                {
                                    // just check the first one, other locations should have the x,y-coordinate name
                                    if (att_value == "projection_x_coordinate" || att_value == "longitude")
                                    {
                                        loc_type[j]->x_location_string = token[i];
                                    }
                                    if (att_value == "projection_y_coordinate" || att_value == "latitude")
                                    {
                                        loc_type[j]->y_location_string = token[i];
                                    }
                                }
                            }
                        }  // end test token[i] on loc_type->var_name
                    }
                }  // end location type, observation point, area or line
            }
        }
    }

    free(var_name_c); var_name_c = NULL;

    // fill the location x- and y-coordinate arrays
    int var_id;
    for (int j = 0; j < loc_type.size(); j++)
    {
        status = nc_inq_varid(this->m_ncid, loc_type[j]->x_location_string.c_str(), &var_id);
        if (status == NC_NOERR) { status = nc_inq_varndims(this->m_ncid, var_id, &ndims); }
        if (status == NC_NOERR && loc_type[j]->type == OBS_POINT)
        {
            double * values_c = (double *)malloc(sizeof(double)*loc_type[j]->location.size());
            status = nc_get_var_double(this->m_ncid, var_id, values_c);
            for (int n = 0; n < loc_type[j]->location.size(); n++)
            {
                loc_type[j]->location[n].x.push_back(*(values_c + n));
            }
            status = nc_inq_varid(this->m_ncid, loc_type[j]->y_location_string.c_str(), &var_id);
            status = nc_get_var_double(this->m_ncid, var_id, values_c);
            for (int n = 0; n < loc_type[j]->location.size(); n++)
            {
                loc_type[j]->location[n].y.push_back(*(values_c + n));
            }
            free(values_c);
            values_c = NULL;
        }
        else if (status == NC_NOERR && loc_type[j]->type == OBS_POLYLINE)
        {
            long * dims = (long *)malloc(sizeof(long)*ndims);
            status = nc_inq_vardimid(this->m_ncid, var_id, (int*)dims);
            size_t mem_length = 1;
            for (long n = 0; n < ndims; n++)
            {
                length = (size_t) -1;
                status = nc_inq_dimlen(this->m_ncid, dims[n], &length);
                mem_length = mem_length * length;
            }
            double * values_c = (double *)malloc(sizeof(double)*mem_length);
            status = nc_get_var_double(this->m_ncid, var_id, values_c);
            int k_start = 0;
            for (int i = 0; i < loc_type[j]->location.size(); i++)
            {
                for (int k = k_start; k < k_start + loc_type[j]->node_count[i]; k++)
                {
                    loc_type[j]->location[i].x.push_back(*(values_c + k));
                }
                k_start += loc_type[j]->node_count[i];
            }
            status = nc_inq_varid(this->m_ncid, loc_type[j]->y_location_string.c_str(), &var_id);
            status = nc_get_var_double(this->m_ncid, var_id, values_c);
            k_start = 0;
            for (int i = 0; i < loc_type[j]->location.size(); i++)
            {
                for (int k = k_start; k < k_start+loc_type[j]->node_count[i]; k++)
                {
                    loc_type[j]->location[i].y.push_back(*(values_c + k));
                }
                k_start += loc_type[j]->node_count[i];
            }
            free(values_c);
            values_c = NULL;
        }
        else
        {
            for (int n = 0; n < loc_type[j]->location.size(); n++)
            {
                loc_type[j]->location[n].x.push_back(std::numeric_limits<double>::quiet_NaN());
                loc_type[j]->location[n].y.push_back(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }

    return status;
}
//------------------------------------------------------------------------------
long HISCF::set_grid_mapping_epsg(long epsg, std::string epsg_code)
{
    long status = 0;
    m_mapping->epsg = epsg;
    m_mapping->epsg_code = epsg_code;
    return status;
}
//------------------------------------------------------------------------------
struct _mapping * HISCF::get_grid_mapping()
{
#ifdef NATIVE_C
    fprintf(stderr, "HISCF::get_grid_mapping()\n");
#else
    //QMessageBox::warning(0, QString("Warning"), QString("HISCF::get_grid_mapping()"));
#endif 
    return m_mapping;
}
//==============================================================================
// PRIVATE functions
//==============================================================================
int HISCF::get_attribute(int ncid, int i_var, char * att_name, char ** att_value)
{
    size_t length = 0;
    int status = -1;

    status = nc_inq_attlen(ncid, i_var, att_name, &length);
    *att_value = (char *)malloc(sizeof(char) * (length + 1));
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
int HISCF::get_attribute(int ncid, int i_var, char * att_name, std::string * att_value)
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
        tmp_value[0] = '\0';
        status = nc_get_att(ncid, i_var, att_name, tmp_value);
        tmp_value[length] = '\0';
        *att_value = std::string(tmp_value, length);
        free(tmp_value);
    }
    return status;
}
//------------------------------------------------------------------------------
int HISCF::get_attribute(int ncid, int i_var, std::string att_name, std::string * att_value)
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
        tmp_value[0] = '\0';
        status = nc_get_att(ncid, i_var, att_name.c_str(), tmp_value);
        tmp_value[length] = '\0';
        *att_value = std::string(tmp_value, length);
        free(tmp_value);
    }
    return status;
}
//------------------------------------------------------------------------------
int HISCF::get_attribute(int ncid, int i_var, char * att_name, double * att_value)
{
    int status = -1;

    status = nc_get_att_double(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int HISCF::get_attribute(int ncid, int i_var, char * att_name, int * att_value)
{
    int status = -1;

    status = nc_get_att_int(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int HISCF::get_attribute(int ncid, int i_var, char * att_name, long * att_value)
{
    int status = -1;

    status = nc_get_att_long(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int HISCF::get_dimension(int ncid, char * dim_name, size_t * dim_length)
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
int HISCF::get_dimension(int ncid, std::string dim_name, size_t * dim_length)
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
int HISCF::get_dimension_var(int ncid, std::string var_name, size_t * dim_length)
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
        char * name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));;
        name[0] = '\0';
        status = nc_inq_dimname(ncid, janm, name);
        status = get_dimension(ncid, name, dim_length);
        free(name);
    }
    return status;
}
std::vector<std::string>  HISCF::get_dimension_names(int ncid, std::string var_name)
{
    int dimid;
    int status = -1;
    std::vector<std::string> dim_names;

    if (var_name.size() != 0)
    {
        int janm;
        status = nc_inq_varid(ncid, var_name.c_str(), &dimid);
        status = nc_inq_vardimid(ncid, dimid, &janm);
        char * name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));;
        name[0] = '\0';
        status = nc_inq_dimname(ncid, janm, name);
        dim_names.push_back(name);
        free(name);
    }
    return dim_names;
}
//------------------------------------------------------------------------------
std::vector<std::string> HISCF::get_names(int ncid, std::string names, size_t count)
{
    int var_id;
    int status;
    std::vector<std::string> token;

    status = nc_inq_varid(ncid, names.c_str(), &var_id);
    if (status == NC_NOERR)
    {
        int ndims[2];
        char * length_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));;
        length_name[0] = '\0';
        status = nc_inq_vardimid(ncid, var_id, (int*)ndims);
        status = nc_inq_dimname(ncid, ndims[1], length_name);
        size_t strlen;
        status = get_dimension(ncid, length_name, &strlen);

        char * c = (char *)malloc(sizeof(char) * (count * strlen));
        c[0] = '\0';
        status = nc_get_var_text(ncid, var_id, c);

        token = tokenize(c, count);
        free(c);
        free(length_name);
    }
    return token;
}

//------------------------------------------------------------------------------
int HISCF::read_grid_mapping(int i_var, std::string var_name, std::string grid_mapping_name)
{
    int status = -1;
    m_mapping->epsg = -1;

    status = get_attribute(this->m_ncid, i_var, "name", &m_mapping->name);
    status = get_attribute(this->m_ncid, i_var, const_cast<char*>("epsg"), &m_mapping->epsg);
    m_mapping->grid_mapping_name = grid_mapping_name; //  == status = get_attribute(this->ncid, i_var, "grid_mapping_name", &map->grid_mapping_name);
    status = get_attribute(this->m_ncid, i_var, const_cast<char*>("longitude_of_prime_meridian"), &m_mapping->longitude_of_prime_meridian);
    status = get_attribute(this->m_ncid, i_var, const_cast<char*>("semi_major_axis"), &m_mapping->semi_major_axis);
    status = get_attribute(this->m_ncid, i_var, const_cast<char*>("semi_minor_axis"), &m_mapping->semi_minor_axis);
    status = get_attribute(this->m_ncid, i_var, const_cast<char*>("inverse_flattening"), &m_mapping->inverse_flattening);
    status = get_attribute(this->m_ncid, i_var, "epsg_code", &m_mapping->epsg_code);
    status = get_attribute(this->m_ncid, i_var, "value", &m_mapping->value);
    status = get_attribute(this->m_ncid, i_var, "projection_name", &m_mapping->projection_name);
    status = get_attribute(this->m_ncid, i_var, "wkt", &m_mapping->wkt);

    if (m_mapping->epsg == -1 && m_mapping->epsg_code.size() != 0)
    {
        std::vector<std::string> token = tokenize(m_mapping->epsg_code, ':');
        m_mapping->epsg = atoi(token[1].c_str());  // second token contains the plain EPSG code
    }

    return status;
}

std::vector<std::string> HISCF::tokenize(const std::string& s, char c) {
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

std::vector<std::string> HISCF::tokenize(const std::string & str, size_t count)
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
