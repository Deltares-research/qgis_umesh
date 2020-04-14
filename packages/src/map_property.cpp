#include "map_property.h"

MapProperty::MapProperty()
{
    prop_opacity = 0.75;  // ondoorzichtig == niet transparant
    prop_dynamic_min_max = true;
    prop_min = INFINITY;
    prop_max = -INFINITY;
    prop_vector_scaling = 1.5;  // 1.5 times the mode of the cell length (meest voorkomende lengte (= sqrt{cell_area})
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
double MapProperty::get_vector_scaling()
{
    return prop_vector_scaling;
}
