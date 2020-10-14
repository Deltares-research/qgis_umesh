#ifndef DATA_H_INC
#define DATA_H_INC

// data structures needed in several file
using namespace std;

enum LOCATION
{
    NODE = 0,
    EDGE,
    FACE,
    NR_LOCATIONS
};
enum OBSERVATION_TYPE
{
    OBS_NONE,  // needed for observation point
    OBS_POINT,  // needed for observation point
    OBS_POLYLINE,  // needed for cross-sectins
    OBS_POLYGON  // needed for monitoring areas
};

struct _mapping {
    string name;
    long epsg;
    string grid_mapping_name;
    double longitude_of_prime_meridian;
    double semi_major_axis;
    double semi_minor_axis;
    double inverse_flattening;
    string  epsg_code;
    string  value;
    string  projection_name;
    string  wkt;
};

struct _global_attribute {
    int type;
    size_t length;
    string name{};
    string cvalue{};
    string svalue{};
    string ivalue{};
    string rvalue{};
    string dvalue{};
};
struct _global_attributes {
    size_t count;
    struct _global_attribute ** attribute;  // should e defined before use
};
#endif DATA_H_INC
