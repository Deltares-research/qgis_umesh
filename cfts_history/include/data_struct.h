#ifndef DATA_H_INC
#define DATA_H_INC

// data structures needed in several file
enum class LOCATION
{
    NODE = 0,
    EDGE,
    FACE,
    NR_LOCATIONS
};
enum class OBSERVATION_TYPE
{
    OBS_NONE,  // needed for observation point
    OBS_POINT,  // needed for observation point
    OBS_POLYLINE,  // needed for cross-sectins
    OBS_POLYGON  // needed for monitoring areas
};

struct _mapping {
    std::string name;
    long epsg;
    std::string grid_mapping_name;
    double longitude_of_prime_meridian;
    double semi_major_axis;
    double semi_minor_axis;
    double inverse_flattening;
    std::string  epsg_code;
    std::string  value;
    std::string  projection_name;
    std::string  wkt;
};

struct _global_attribute {
    int type;
    std::size_t length;
    std::string name{};
    std::string cvalue{};
    std::string svalue{};
    std::string ivalue{};
    std::string rvalue{};
    std::string dvalue{};
};
struct _global_attributes {
    std::size_t count;
    struct _global_attribute ** attribute;  // should e defined before use
};
#endif DATA_H_INC
