#ifndef __MAPPING_H__
#define __MAPPING_H__

#include <QtCore/QFileInfo>

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>

#include <qgsmessagelog.h>

#include "netcdf.h"
#include "data_struct.h"
#include "perf_timer.h"


class MAPPING
{
public:
    MAPPING();
    ~MAPPING();
    long set_epsg(int, std::string);
    long get_epsg();
    std::string get_epsg_string();

private:
    std::string m_name;
    long m_epsg;
    std::string m_grid_mapping_name;
    double m_longitude_of_prime_meridian;
    double m_semi_major_axis;
    double m_semi_minor_axis;
    double m_inverse_flattening;
    std::string m_epsg_string;
    std::string m_value;
    std::string m_projection_name;
    std::string m_wkt;
};
#endif __MAPPING_H__
