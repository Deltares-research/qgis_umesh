#include "mapping.h"

MAPPING::MAPPING()
{
    m_name = std::string("---");
    m_epsg = -1;
    m_epsg_string = std::string("---");
}
MAPPING::~MAPPING()
{
}

//------------------------------------------------------------------------------
long MAPPING::set_epsg(int epsg, std::string epsg_code)
{
    long status = 0;
    m_epsg = (long)epsg;
    m_epsg_string = epsg_code;
    return status;
}
//------------------------------------------------------------------------------
long MAPPING::get_epsg()
{
    return m_epsg;
}
//------------------------------------------------------------------------------
std::string MAPPING::get_epsg_string()
{
    return m_epsg_string;
}
