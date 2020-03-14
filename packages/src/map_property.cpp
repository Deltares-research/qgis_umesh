#include "map_property.h"

MapProperty::MapProperty()
{
    prop_opacity = 0.75;  // ondoorzichtig == niet transparant
    prop_dynamic_min_max = true;
    prop_min = -INFINITY;
    prop_max = INFINITY;
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
void MapProperty::set_opacity(double z_max)
{
    prop_opacity = z_max;
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
