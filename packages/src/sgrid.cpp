#include "sgrid.h"
#include "perf_timer.h"

// Class to support structured grids

SGRID::SGRID()
{
}
SGRID::SGRID(QFileInfo filename, int ncid, QProgressBar* pgBar)
{
    m_fname = filename;  // filename without path, just the name
    m_sgrid_file_name = filename.absoluteFilePath();  // filename with complete path
    m_ncid = ncid;
    m_pgBar = pgBar;
}
SGRID::~SGRID()
{
    int status;
    free(m_dimids);
    status = nc_close(m_ncid);
}
//------------------------------------------------------------------------------
long SGRID::read()
{
    long status = -1;

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
QFileInfo SGRID::get_filename()
{
    return m_sgrid_file_name;
}
//------------------------------------------------------------------------------
long SGRID::read_mesh()
{
    int ndims, nvars, natts, nunlimited;
    int unlimid;
    std::string cf_role;
    std::string grid_mapping_name;
    std::string std_name;
    std::vector<std::string> tmp_dim_names;
    long stat = 1;
    int status = 1;
    long nr_smesh2d = 0;
    int var_id = -1;
    std::string coordinates;
    std::string att_value;
    size_t count;

    m_pgBar->setValue(15);
  
    char* var_name_c = (char*)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    m_mapping = new _mapping();
    m_mapping->epsg = 0;
    m_mapping->epsg_code = "EPSG:0";

    status = nc_inq(m_ncid, &ndims, &nvars, &natts, &nunlimited);
    m_dimids = (size_t*)malloc(sizeof(size_t) * ndims);
    for (int i = 0; i < ndims; i++)
    {
        status = nc_inq_dimlen(m_ncid, i, &m_dimids[i]);
        status = nc_inq_dimname(m_ncid, i, var_name_c);
        m_dim_names.push_back(std::string(var_name_c));
        m_map_dim[std::string(var_name_c)] = (long) m_dimids[i];
    }
    status = nc_inq_unlimdim(m_ncid, &unlimid);

    nr_smesh2d += 1;
    if (nr_smesh2d == 1)
    {
        m_smesh2d_strings = (struct _smesh2d_string**)malloc(sizeof(struct _smesh2d_string*) * nr_smesh2d);
    }
    m_smesh2d_strings[nr_smesh2d - 1] = new _smesh2d_string;
    if (nr_smesh2d == 1)
    {
        m_smesh2d = new _smesh2d();
        m_smesh2d->node = (struct _sfeature**)malloc(sizeof(struct _sfeature*));
        m_smesh2d->node[nr_smesh2d - 1] = new _sfeature();
    }

    for (int i_var = 0; i_var < nvars; i_var++)
    {
        status = get_attribute(m_ncid, i_var, "coordinates", &coordinates);
        if (status == 0) 
        {
            // coordinates found, so it is a variable on the structured mesh
        }
        else
        {
            nc_type nc_type;
            nc_inq_varndims(m_ncid, i_var, &ndims);
            int* var_dimids = NULL;
            if (ndims > 0) {
                var_dimids = (int*)malloc(sizeof(int) * ndims);
            }
            status = nc_inq_var(m_ncid, i_var, var_name_c, &nc_type, &ndims, var_dimids, &natts);
            status = nc_inq_varid(m_ncid, var_name_c, &var_id);
            std::string var_name(var_name_c);
#ifdef NATIVE_C
            fprintf(stderr, "SGRID::read_mesh()\n    Variable name: %d - %s\n", i_var + 1, var_name.c_str());
#else
            m_pgBar->setValue(20 + (i_var + 1) / nvars * (500 - 20));
#endif

            {   // coordinate axis (x, y, time)
                status = get_attribute(m_ncid, i_var, "standard_name", &std_name);
                status = get_dimension_var(this->m_ncid, var_name, &count);

                if (std_name == "projection_x_coordinate" || std_name == "longitude")
                {
                    m_smesh2d_strings[nr_smesh2d - 1]->x_node_name = var_name;
                    m_smesh2d->node[nr_smesh2d - 1]->x = std::vector<double>(count);
                    status = nc_get_var_double(this->m_ncid, var_id, m_smesh2d->node[nr_smesh2d - 1]->x.data());
                }
                else if (std_name == "projection_y_coordinate" || std_name == "altitude")
                {
                    m_smesh2d_strings[nr_smesh2d - 1]->y_node_name = var_name;
                    m_smesh2d->node[nr_smesh2d - 1]->y = std::vector<double>(count);
                    status = nc_get_var_double(this->m_ncid, var_id, m_smesh2d->node[nr_smesh2d - 1]->y.data());
                }
            }
        }
    }

    free(var_name_c); var_name_c = nullptr;

    return stat;
}
//------------------------------------------------------------------------------
long SGRID::read_times()
{
    // Look for variable with standard name "time"
    int ndims, nvars, natts, nunlimited;
    int status;
    std::string std_name;
    std::string tmp_unit;

    char* var_name_c;
    size_t length;
    int dimids;
    int nr_time_series = 0;
    double dt;

    QStringList date_time;
    char* time_var_name;
    QDateTime* RefDate;
    struct _stime_series t_series;
    time_series.push_back(t_series);
    time_series[0].nr_times = 0;
    time_series[0].dim_name = nullptr;
    time_series[0].long_name = nullptr;
    time_series[0].unit = nullptr;

    m_pgBar->setValue(700);

    var_name_c = (char*)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    var_name_c[0] = '\0';
    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);
    for (long i_var = 0; i_var < nvars; i_var++)
    {
        status = get_attribute(m_ncid, i_var, "standard_name", &std_name);
        if (std_name == "time") // or check on var_name == var_dimension == "time"
        {
            status = get_attribute(m_ncid, i_var, "units", &tmp_unit);
            if (tmp_unit == "s")
            {
                date_time = QStringList{ "seconds", "since", "0000-00-00", "00:00:00" };
            }
            else
            {
                QString units = QString::fromStdString(tmp_unit).replace("T", " ");  // "seconds since 1970-01-01T00:00:00" changed into "seconds since 1970-01-01 00:00:00"
                date_time = units.split(" ");
            }
            if (date_time.count() >= 2)
            {
                if (!strcmp("since", date_time.at(1).toUtf8()))
                {
                    // now it is the time variable, can only be detected by the "seconds since 1970-01-01T00:00:00" character string
                    // retrieve the long_name, standard_name -> var_name for the xaxis label
                    length = (size_t)-1;
                    status = nc_inq_var(this->m_ncid, i_var, var_name_c, NULL, &ndims, &dimids, &natts);
                    status = nc_inq_dimname(this->m_ncid, dimids, var_name_c);
                    time_series[0].dim_name = new QString(var_name_c);
                    status = nc_inq_attlen(this->m_ncid, i_var, "long_name", &length);
                    if (status == NC_NOERR)
                    {
                        char* c_label = (char*)malloc(sizeof(char) * (length + 1));
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
                                qdt_times.append(RefDate->addMSecs(1000. * time_series[0].times[j]));  // milli seconds as smallest time unit
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
                    }
                    break;
                }
            }
        }
    }
    free(var_name_c);
    var_name_c = nullptr;
    m_pgBar->setValue(800);

    return (long)status;
}

long SGRID::read_variables()
{
    // read the attributes of the variable  and the dimensions
    int ndims, nvars, natts, nunlimited;
    int status;
    struct _variable* var;
    std::string coordinates;

    var = NULL;
#ifndef NATIVE_C
    m_pgBar->setValue(800);
#endif
    m_nr_smesh_var = 0;

    std::string tmp_string;
    char* var_name_c = (char*)malloc(sizeof(char) * (NC_MAX_NAME + 1));
    status = nc_inq(this->m_ncid, &ndims, &nvars, &natts, &nunlimited);

    for (int i_var = 0; i_var < nvars; i_var++)
    {
        status = get_attribute(m_ncid, i_var, "coordinates", &coordinates);
        if (status == 0)
        {
            nc_type nc_type;
            nc_inq_varndims(this->m_ncid, i_var, &ndims);
            int* var_dimids = NULL;
            if (ndims > 0) {
                var_dimids = (int*)malloc(sizeof(int) * ndims);
            }
            status = nc_inq_var(this->m_ncid, i_var, var_name_c, &nc_type, &ndims, var_dimids, &natts);
            std::string var_name(var_name_c);
            
            m_nr_smesh_var += 1;
            if (m_nr_smesh_var == 1)
            {
                m_smesh_vars = new _smesh_variable();
                m_smesh_vars->svariable = (struct _svariable**)malloc(sizeof(struct _svariable*));
            }
            else
            {
                m_smesh_vars->svariable = (struct _svariable**)realloc(m_smesh_vars->svariable, sizeof(struct _svariable*) * m_nr_smesh_var);
            }
            m_smesh_vars->svariable[m_nr_smesh_var - 1] = new _svariable();
            m_smesh_vars->nr_vars = m_nr_smesh_var;

            m_smesh_vars->svariable[m_nr_smesh_var - 1]->var_name = var_name;
            m_smesh_vars->svariable[m_nr_smesh_var - 1]->nc_type = nc_type;
            m_smesh_vars->svariable[m_nr_smesh_var - 1]->read = false;
            m_smesh_vars->svariable[m_nr_smesh_var - 1]->topology_dimension = 2;
            m_smesh_vars->svariable[m_nr_smesh_var - 1]->location = "node";
            m_smesh_vars->svariable[m_nr_smesh_var - 1]->map_dim_name = m_map_dim_name;

            status = get_attribute(m_ncid, i_var, "coordinates", &m_smesh_vars->svariable[m_nr_smesh_var - 1]->coordinates);
            status = get_attribute(m_ncid, i_var, "standard_name", &m_smesh_vars->svariable[m_nr_smesh_var - 1]->standard_name);
            status = get_attribute(m_ncid, i_var, "long_name", &m_smesh_vars->svariable[m_nr_smesh_var - 1]->long_name);
            if (status != NC_NOERR || m_smesh_vars->svariable[m_nr_smesh_var - 1]->long_name.size() <= 1)
            {
                m_smesh_vars->svariable[m_nr_smesh_var - 1]->long_name = m_smesh_vars->svariable[m_nr_smesh_var - 1]->standard_name;
                if (m_smesh_vars->svariable[m_nr_smesh_var - 1]->long_name.size() <= 1)
                {
                    m_smesh_vars->svariable[m_nr_smesh_var - 1]->long_name = m_smesh_vars->svariable[m_nr_smesh_var - 1]->var_name;
                }
            }
            status = get_attribute(m_ncid, i_var, "units", &m_smesh_vars->svariable[m_nr_smesh_var - 1]->units);
            status = get_attribute(m_ncid, i_var, const_cast<char*>("_FillValue"), &m_smesh_vars->svariable[m_nr_smesh_var - 1]->fill_value);
            if (status != NC_NOERR)
            {
                m_smesh_vars->svariable[m_nr_smesh_var - 1]->fill_value = std::numeric_limits<double>::quiet_NaN();
            }
            status = get_attribute(m_ncid, i_var, "comment", &m_smesh_vars->svariable[m_nr_smesh_var - 1]->comment);
            // variable with flag_values and flag_meanings
            std::string flag_meanings;
            status = get_attribute(this->m_ncid, i_var, "flag_meanings", &flag_meanings);
            if (status == NC_NOERR)
            {
                QString msg = QString("Attribute 'Flag_meanings' not yet support for variable: \'%1\'").arg(var_name.c_str());
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
            m_smesh_vars->svariable[m_nr_smesh_var - 1]->draw = false;
            m_smesh_vars->svariable[m_nr_smesh_var - 1]->time_series = false;
            for (int j = 0; j < ndims; j++)
            {
                status = nc_inq_dimname(this->m_ncid, var_dimids[j], var_name_c);
                m_smesh_vars->svariable[m_nr_smesh_var - 1]->dims.push_back((long)m_dimids[var_dimids[j]]);  // Todo: HACK typecast: size_t -> long
                if (time_series[0].nr_times != 0 && QString::fromStdString(m_dim_names[var_dimids[j]]) == time_series[0].dim_name)
                {
                    m_smesh_vars->svariable[m_nr_smesh_var - 1]->time_series = true;
                }
                m_smesh_vars->svariable[m_nr_smesh_var - 1]->dim_names.push_back(m_dim_names[var_dimids[j]]);
            }
            if (m_smesh_vars->svariable[m_nr_smesh_var - 1]->dims.size() == 3)
            {
                // Dimensions: time, x an y
            }
            bool contains_time_dimension = false;
            for (int i = 0; i < m_smesh_vars->svariable[m_nr_smesh_var - 1]->dims.size(); i++)
            {
                // check if one of the dimension is the time dimension
                if (QString::fromStdString(m_smesh_vars->svariable[m_nr_smesh_var - 1]->dim_names[i]) == time_series[0].dim_name)
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
                QString msg = QString("Variable other than with time dimension is not supported: \'%1\'").arg(var_name.c_str());
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
        }
    }
    return (long)status;
}
//------------------------------------------------------------------------------
struct _smesh_variable* SGRID::get_variables()
{
    return m_smesh_vars;
}//------------------------------------------------------------------------------
struct _mapping* SGRID::get_grid_mapping()
{
    return m_mapping;
}
//------------------------------------------------------------------------------
long SGRID::set_grid_mapping_epsg(long epsg, std::string epsg_code)
{
    long status = 0;
    m_mapping->epsg = epsg;
    m_mapping->epsg_code = epsg_code;
    return status;
}
//------------------------------------------------------------------------------
struct _smesh2d* SGRID::get_smesh_2d()
{
    return m_smesh2d;
}
//==============================================================================
// PRIVATE functions
//==============================================================================
int SGRID::get_attribute_by_var_name(int ncid, std::string var_name, std::string att_name, std::string* att_value)
{
    int status = -1;
    int i_var;
    status = nc_inq_varid(ncid, var_name.c_str(), &i_var);
    status = get_attribute(ncid, i_var, att_name, att_value);
    return status;
}
//------------------------------------------------------------------------------
int SGRID::get_attribute(int ncid, int i_var, char* att_name, char** att_value)
{
    size_t length = 0;
    int status = -1;

    status = nc_inq_attlen(ncid, i_var, att_name, &length);
    *att_value = (char*)malloc(sizeof(char) * (length + 1));
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
int SGRID::get_attribute(int ncid, int i_var, char* att_name, std::string* att_value)
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
        status = nc_get_att(ncid, i_var, att_name, tmp_value);
        tmp_value[length] = '\0';
        *att_value = std::string(tmp_value, length);
        free(tmp_value);
        tmp_value = nullptr;
    }
    return status;
}
//------------------------------------------------------------------------------
int SGRID::get_attribute(int ncid, int i_var, std::string att_name, std::string* att_value)
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
        status = nc_get_att(ncid, i_var, att_name.c_str(), tmp_value);
        tmp_value[length] = '\0';
        *att_value = std::string(tmp_value, length);
        free(tmp_value);
        tmp_value = nullptr;
    }
    return status;
}
//------------------------------------------------------------------------------
int SGRID::get_attribute(int ncid, int i_var, char* att_name, double* att_value)
{
    int status = -1;

    status = nc_get_att_double(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int SGRID::get_attribute(int ncid, int i_var, char* att_name, int* att_value)
{
    int status = -1;

    status = nc_get_att_int(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int SGRID::get_attribute(int ncid, int i_var, char* att_name, long* att_value)
{
    int status = -1;

    status = nc_get_att_long(ncid, i_var, att_name, att_value);

    return status;
}
//------------------------------------------------------------------------------
int SGRID::get_dimension(int ncid, char* dim_name, size_t* dim_length)
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
int SGRID::get_dimension(int ncid, std::string dim_name, size_t* dim_length)
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
int SGRID::get_dimension_var(int ncid, std::string var_name, size_t* dim_length)
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
        char* tmp_value = (char*)malloc(sizeof(char) * (NC_MAX_NAME + 1));;
        status = nc_inq_dimname(ncid, janm, tmp_value);
        status = get_dimension(ncid, tmp_value, dim_length);
        free(tmp_value);
        tmp_value = nullptr;
    }
    return status;
}
//------------------------------------------------------------------------------
std::vector<std::string> SGRID::tokenize(const std::string& s, char c) {
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
std::vector<std::string> SGRID::tokenize(const std::string& s, std::size_t count)
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


