#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

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
    (void)nc_close(this->ncid);

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
    char * m_hiscf_file_name = strdup(m_fname.absoluteFilePath().toUtf8());
    status = nc_open(m_hiscf_file_name, NC_NOWRITE, &this->ncid);
    if (status != NC_NOERR)
    {
        QMessageBox::critical(0, QString("Error"), QString("HISCF::read()\n\tFailed to open file: %1").arg(m_hiscf_file_name));
        return status;
    }
    delete m_hiscf_file_name;
    m_hiscf_file_name = NULL;
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
    char * att_value_c = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    att_value_c[0] = '\0';

    status = nc_inq(this->ncid, &ndims, &nvars, &natts, &nunlimited);

    this->global_attributes = (struct _global_attributes *)malloc(sizeof(struct _global_attributes));
    this->global_attributes->count = natts;
    this->global_attributes->attribute = (struct _global_attribute **)malloc(sizeof(struct _global_attribute *) * natts);
    for (long i = 0; i < natts; i++)
    {
        this->global_attributes->attribute[i] = new _global_attribute;
    }

    for (long i = 0; i < natts; i++)
    {
        status = nc_inq_attname(this->ncid, NC_GLOBAL, i, att_name_c);
        status = nc_inq_att(this->ncid, NC_GLOBAL, att_name_c, &att_type, &att_length);
        this->global_attributes->attribute[i]->name = string(att_name_c);
        this->global_attributes->attribute[i]->type = att_type;
        this->global_attributes->attribute[i]->length = att_length;
        if (att_type == NC_CHAR)
        {
            status = nc_get_att_text(this->ncid, NC_GLOBAL, att_name_c, att_value_c);
            att_value_c[att_length] = '\0';
            this->global_attributes->attribute[i]->cvalue = string(att_value_c);
        }
        else
        {
#ifdef NATIVE_C
            fprintf(stderr, "\tAttribute nc_type: %d\n", att_type);
#endif    
        }
    }
    free(att_name_c);
    free(att_value_c);

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
vector<_location_type *> HISCF::get_observation_location()
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
    return m_hiscf_file_name;
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
    long i_par_loc;
    long nr_locations;
    long * sn_dims; // station names dimensions
    bool model_wide_found = false;
    bool model_wide_exist = false;
    string grid_mapping_name;


    int nr_par_loc = 0;

    dim_name_c = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    var_name_c = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));

    status = nc_inq(this->ncid, &ndims, &nvars, &natts, &nunlimited);
    // cf_role = timeseries_id
    for (long i = 0; i < nvars; i++)
    {
        length = -1;
        status = nc_inq_attlen(this->ncid, i, "cf_role", &length);
        if (status == NC_NOERR)
        {
            cf_role = (char *)malloc(sizeof(char) * (length + 1));
            status = nc_get_att(this->ncid, i, "cf_role", cf_role);
            cf_role[length] = '\0';

            if (!strcmp(cf_role, "timeseries_id"))
            {
                nc_type nc_type;
                location_name_varid = i;
                status = nc_inq_var(this->ncid, location_name_varid, var_name_c, &nc_type, &ndims, NULL, &natts);
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
                    nr_par_loc = (int)loc_type.size();  // HACK typecast
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
                length = -1;
                status = nc_inq_attlen(this->ncid, location_name_varid, "long_name", &length);
                if (status == NC_NOERR)
                {
                    char * long_name = (char *)malloc(sizeof(char) * (length + 1));
                    status = nc_get_att(this->ncid, location_name_varid, "long_name", long_name);
                    long_name[length] = '\0';
                    loc_type[i_par_loc]->location_long_name = strdup(long_name);
                    free(long_name);
                }
                else
                {
                    loc_type[i_par_loc]->location_long_name = strdup(var_name_c);
                }

                sn_dims = (long *)malloc(sizeof(long *)*ndims);
                status = nc_inq_vardimid(this->ncid, location_name_varid, (int*)sn_dims);

                mem_length = 1;
                name_len = -1;
                for (long j = 0; j < ndims; j++)
                {
                    length = -1;
                    status = nc_inq_dim(this->ncid, sn_dims[j], dim_name_c, &length);

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
                    status = nc_get_var_string(this->ncid, location_name_varid, location_strings);
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
                        loc_type[i_par_loc]->x_location_name = string("---");
                        loc_type[i_par_loc]->y_location_name = string("---");
                    }
                    free(location_strings);
                }
                else if (nc_type == NC_CHAR)
                {
                    char * location_chars = (char *)malloc(sizeof(char *) * (mem_length)+1);
                    status = nc_get_var_text(this->ncid, location_name_varid, location_chars);
                    char * janm = (char *)malloc(sizeof(char)*(name_len + 1));
                    janm[name_len] = '\0';
                    for (int k = 0; k < nr_locations; k++)
                    {
                        strncpy(janm, location_chars + k * name_len, name_len);
                        struct _location loc;
                        loc.name = janm;
                        loc_type[i_par_loc]->location.push_back(loc);
                        loc_type[i_par_loc]->x_location_name = string("---");
                        loc_type[i_par_loc]->y_location_name = string("---");
                    }
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
            status = get_attribute(this->ncid, i, "grid_mapping_name", &grid_mapping_name);
            if (status == NC_NOERR)
            {
                status = read_grid_mapping(i, var_name_c, grid_mapping_name);
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
    long status;
    size_t length;
    char * var_name_c;
    char * coord_c = NULL;
    string coord;

    size_t * par_dim;
    long i_par_loc = -1;
    long i_param = -1;

    status = -1;

    var_name_c = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));

    status = nc_inq(this->ncid, &ndims, &nvars, &natts, &nunlimited);
    for (long i_var = 0; i_var < nvars; i_var++)
    {
        long * par_dim_ids;  // parameter dimensions

        length = -1;
        i_par_loc = -1;
        status = nc_inq_var(this->ncid, i_var, var_name_c, NULL, &ndims, NULL, &natts);

        par_dim = (size_t *)malloc(sizeof(long *)*ndims);
        par_dim_ids = (long *)malloc(sizeof(long *)*ndims);
        status = nc_inq_vardimid(this->ncid, i_var, (int*)par_dim_ids);

        status = get_attribute(this->ncid, i_var, "coordinates", &coord);
        if (status == NC_NOERR)
        {
            for (int j = 0; j < loc_type.size(); j++)
            {
                // only variables with the attribute "coordinates" are treathed
                bool found = false;
                vector<string> token = tokenize(coord, ' ');
                if (token.size() == 1 && string(loc_type[j]->location_var_name) == "cross_section_name")  // assume it is the cross_section
                {
                    token.resize(3);
                    token[2] = token[0];
                    token[0] = "cross_section_x_coordinate";
                    token[1] = "cross_section_y_coordinate";
                }
                for (int i_token = 0; i_token < token.size(); i_token++)  // var_x, var_y, var_name
                {
                    if (!found && string(loc_type[j]->location_var_name) == token[i_token])
                    {
                        found = true;
                        // location_var_name is found
                        // loop agian over all tokens to find the x- and y- coordinates, ie doubling the token i_token
                        for (int i = 0; i < token.size(); i++)  // var_x, var_y, var_name
                        {
                            int var_id = -1;
                            status = nc_inq_varid(this->ncid, token[i].c_str(), &var_id);
                            string att_value;
                            status = get_attribute(this->ncid, var_id, "standard_name", &att_value);
                            if (status == NC_NOERR)
                            {
                                // just ceck the first one, other locations should hev the x,y-coordinate name
                                if (att_value == "projection_x_coordinate" || att_value == "longitude")
                                {
                                    loc_type[j]->x_location_name = token[i];
                                }
                                if (att_value == "projection_y_coordinate" || att_value == "latitude")
                                {
                                    loc_type[j]->y_location_name = token[i];
                                }
                            }
                        }
                    }  // end coordinate var_name
                }
            }  // end location type, observation point, area or line
        }
    }

    free(var_name_c); var_name_c = NULL;

    // fill the location x- and y-coordinate arrays
    int var_id;
    for (int j = 0; j < loc_type.size(); j++)
    {
        int ndims;
        status = nc_inq_varid(this->ncid, loc_type[j]->x_location_name.c_str(), &var_id);
        if (status == NC_NOERR) { status = nc_inq_varndims(this->ncid, var_id, &ndims); }
        if (status == NC_NOERR && ndims == 1)
        {
            loc_type[j]->type = OBS_POINT;
            double * values_c = (double *)malloc(sizeof(double)*loc_type[j]->location.size());
            status = nc_get_var_double(this->ncid, var_id, values_c);
            for (int n = 0; n < loc_type[j]->location.size(); n++)
            {
                loc_type[j]->location[n].x.push_back(*(values_c + n));
            }
            status = nc_inq_varid(this->ncid, loc_type[j]->y_location_name.c_str(), &var_id);
            status = nc_get_var_double(this->ncid, var_id, values_c);
            for (int n = 0; n < loc_type[j]->location.size(); n++)
            {
                loc_type[j]->location[n].y.push_back(*(values_c + n));
            }
            free(values_c);
            values_c = NULL;
        }
        else if (ndims == 2)
        {
            //  ndims == 2
            loc_type[j]->type = OBS_POLYLINE;
            long * dims = (long *)malloc(sizeof(long *)*ndims);
            status = nc_inq_vardimid(this->ncid, var_id, (int*)dims);
            size_t mem_length = 1;
            for (long j = 0; j < ndims; j++)
            {
                length = -1;
                status = nc_inq_dimlen(this->ncid, dims[j], &length);
                mem_length = mem_length * length;
            }
            int points_per_line = mem_length/loc_type[j]->location.size();
            double * values_c = (double *)malloc(sizeof(double)*mem_length);
            status = nc_get_var_double(this->ncid, var_id, values_c);
            for (int n = 0; n < loc_type[j]->location.size(); n++)
            {
                for (int k = 0; k < points_per_line; k++)
                {
                    loc_type[j]->location[n].x.push_back(*(values_c + points_per_line * n + k));
                }
            }
            status = nc_inq_varid(this->ncid, loc_type[j]->y_location_name.c_str(), &var_id);
            status = nc_get_var_double(this->ncid, var_id, values_c);
            for (int n = 0; n < loc_type[j]->location.size(); n++)
            {
                for (int k = 0; k < points_per_line; k++)
                {
                    loc_type[j]->location[n].y.push_back(*(values_c + points_per_line * n + k));
                }
            }
            free(values_c);
            values_c = NULL;
        }
        else
        {
            for (int n = 0; n < loc_type[j]->location.size(); n++)
            {
                loc_type[j]->location[n].x.push_back(numeric_limits<double>::quiet_NaN());
                loc_type[j]->location[n].y.push_back(numeric_limits<double>::quiet_NaN());
            }
        }
    }

    return status;
}
//------------------------------------------------------------------------------
long HISCF::set_grid_mapping_epsg(long epsg, string epsg_code)
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

    status = nc_inq_attlen(this->ncid, i_var, att_name, &length);
    *att_value = (char *)malloc(sizeof(char) * (length + 1));
    if (status != NC_NOERR)
    {
        *att_value = '\0';
    }
    else
    {
        status = nc_get_att(this->ncid, i_var, att_name, *att_value);
        att_value[0][length] = '\0';
    }
    return status;
}
//------------------------------------------------------------------------------
int HISCF::get_attribute(int ncid, int i_var, char * att_name, string * att_value)
{
    size_t length = 0;
    int status = -1;

    status = nc_inq_attlen(this->ncid, i_var, att_name, &length);
    if (status != NC_NOERR)
    {
        *att_value = "";
    }
    else
    {
        char * tmp_value = (char *)malloc(sizeof(char) * (length + 1));
        status = nc_get_att(this->ncid, i_var, att_name, tmp_value);
        tmp_value[length] = '\0';
        *att_value = string(tmp_value, length);
        free(tmp_value);
    }
    return status;
}
//------------------------------------------------------------------------------
int HISCF::get_attribute(int ncid, int i_var, string att_name, string * att_value)
{
    size_t length = 0;
    int status = -1;

    status = nc_inq_attlen(this->ncid, i_var, att_name.c_str(), &length);
    if (status != NC_NOERR)
    {
        *att_value = "";
    }
    else
    {
        char * tmp_value = (char *)malloc(sizeof(char) * (length + 1));
        status = nc_get_att(this->ncid, i_var, att_name.c_str(), tmp_value);
        tmp_value[length] = '\0';
        *att_value = string(tmp_value, length);
        free(tmp_value);
    }
    return status;
}
//------------------------------------------------------------------------------
int HISCF::get_attribute(int ncid, int i_var, char * att_name, double * att_value)
{
    int status = -1;

    status = nc_get_att_double(this->ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int HISCF::get_attribute(int ncid, int i_var, char * att_name, int * att_value)
{
    int status = -1;

    status = nc_get_att_int(this->ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int HISCF::get_attribute(int ncid, int i_var, char * att_name, long * att_value)
{
    int status = -1;

    status = nc_get_att_long(this->ncid, i_var, att_name, att_value);

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
        status = nc_inq_dimid(this->ncid, dim_name, &dimid);
        status = nc_inq_dimlen(this->ncid, dimid, dim_length);
    }
    return status;
}
//------------------------------------------------------------------------------
int HISCF::get_dimension(int ncid, string dim_name, size_t * dim_length)
{
    int dimid;
    int status = -1;

    *dim_length = 0;
    if (dim_name.size() != 0)
    {
        status = nc_inq_dimid(this->ncid, dim_name.c_str(), &dimid);
        status = nc_inq_dimlen(this->ncid, dimid, dim_length);
    }
    return status;
}
//------------------------------------------------------------------------------
int HISCF::get_dimension_var(int ncid, string var_name, size_t * dim_length)
{
    // get the total dimension length in bytes of the var_name variable
    int dimid;
    int status = -1;
    *dim_length = 0;

    if (var_name.size() != 0)
    {
        int janm;
        status = nc_inq_varid(this->ncid, var_name.c_str(), &dimid);
        status = nc_inq_vardimid(this->ncid, dimid, &janm);
        char * name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));;
        status = nc_inq_dimname(this->ncid, janm, name);
        status = get_dimension(this->ncid, name, dim_length);
        free(name);
    }
    return status;
}
vector<string>  HISCF::get_dimension_names(int ncid, string var_name)
{
    int dimid;
    int status = -1;
    vector<string> dim_names;

    if (var_name.size() != 0)
    {
        int janm;
        status = nc_inq_varid(this->ncid, var_name.c_str(), &dimid);
        status = nc_inq_vardimid(this->ncid, dimid, &janm);
        char * name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));;
        status = nc_inq_dimname(this->ncid, janm, name);
        dim_names.push_back(name);
        free(name);
    }
    return dim_names;
}
//------------------------------------------------------------------------------
vector<string> HISCF::get_names(int ncid, string names, size_t count)
{
    int var_id;
    int status;
    vector<string> token;

    status = nc_inq_varid(ncid, names.c_str(), &var_id);
    if (status == NC_NOERR)
    {
        int ndims[2];
        char * length_name = (char *)malloc(sizeof(char) * (NC_MAX_NAME + 1));;
        status = nc_inq_vardimid(ncid, var_id, (int*)ndims);
        status = nc_inq_dimname(ncid, ndims[1], length_name);
        size_t strlen;
        status = get_dimension(ncid, length_name, &strlen);

        char * c = (char *)malloc(sizeof(char) * (count * strlen));
        status = nc_get_var_text(ncid, var_id, c);

        token = tokenize(c, count);
        delete c;
        delete length_name;
    }
    return token;
}

//------------------------------------------------------------------------------
int HISCF::read_grid_mapping(int i_var, string var_name, string grid_mapping_name)
{
    int status = -1;
    m_mapping->epsg = -1;

    status = get_attribute(this->ncid, i_var, "name", &m_mapping->name);
    status = get_attribute(this->ncid, i_var, "epsg", &m_mapping->epsg);
    m_mapping->grid_mapping_name = grid_mapping_name; //  == status = get_attribute(this->ncid, i_var, "grid_mapping_name", &map->grid_mapping_name);
    status = get_attribute(this->ncid, i_var, "longitude_of_prime_meridian", &m_mapping->longitude_of_prime_meridian);
    status = get_attribute(this->ncid, i_var, "semi_major_axis", &m_mapping->semi_major_axis);
    status = get_attribute(this->ncid, i_var, "semi_minor_axis", &m_mapping->semi_minor_axis);
    status = get_attribute(this->ncid, i_var, "inverse_flattening", &m_mapping->inverse_flattening);
    status = get_attribute(this->ncid, i_var, "epsg_code", &m_mapping->epsg_code);
    status = get_attribute(this->ncid, i_var, "value", &m_mapping->value);
    status = get_attribute(this->ncid, i_var, "projection_name", &m_mapping->projection_name);
    status = get_attribute(this->ncid, i_var, "wkt", &m_mapping->wkt);

    if (m_mapping->epsg == -1 && m_mapping->epsg_code.size() != 0)
    {
        vector<string> token = tokenize(m_mapping->epsg_code, ':');
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

std::vector<std::string> HISCF::tokenize(const std::string& s, std::size_t count)
{
    size_t minsize = s.size() / count;
    std::vector<std::string> tokens;
    for (size_t i = 0, offset = 0; i < count; ++i)
    {
        size_t size = minsize;
        if ((offset + size) < s.size())
            tokens.push_back(s.substr(offset, size));
        else
            tokens.push_back(s.substr(offset, s.size() - offset));
        offset += size;
    }
    return tokens;
}
