#include "map_property.h"

MapProperty::MapProperty()
{
    prop_opacity = 0.75;  // tranparancy = 1.0 - opacity 
    prop_dynamic_min_max = true;
    prop_refresh_rate = 0.1;
    prop_min = INFINITY;
    prop_max = -INFINITY;
    prop_vector_scaling = 1.0;  // 1.0 times the average cell length (= sqrt{cell_area})
}
MapProperty::~MapProperty()
{
}
void MapProperty::set_dynamic_legend(bool dynamic)
{
    prop_dynamic_min_max = dynamic;
}
void MapProperty::set_minimum(double z_min)
{
    prop_min = z_min;
}
void MapProperty::set_maximum(double z_max)
{
    prop_max = z_max;
}
void MapProperty::set_opacity(double opacity)
{
    prop_opacity = opacity;
}
void MapProperty::set_refresh_rate(double refresh_rate)
{
    prop_refresh_rate = refresh_rate;
}
void MapProperty::set_vector_scaling(double v_fac)
{
    prop_vector_scaling = v_fac;
}
bool MapProperty::get_dynamic_legend()
{
    return prop_dynamic_min_max;
}
double MapProperty::get_minimum()
{
    return prop_min;
}
double MapProperty::get_maximum()
{
    return prop_max;
}
double MapProperty::get_opacity()
{
    return prop_opacity;
}
double MapProperty::get_refresh_rate()
{
    return prop_refresh_rate;
}
double MapProperty::get_vector_scaling()
{
    return prop_vector_scaling;
}
