#ifndef _INC_MAP_PROPERTY
#define _INC_MAP_PROPERTY

#include <QDateTimeEdit>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QCheckBox>

class MapProperty
{
public:
    MapProperty();  // Constructor
    ~MapProperty();  // Destructor
    static MapProperty * getInstance()
    {
        if (obj == nullptr)
            obj = new MapProperty();
        return obj;
    }

    void set_dynamic_legend(bool);
    void set_minimum(double);
    void set_maximum(double);
    void set_opacity(double);
    void set_refresh_time(double);
    void set_vector_scaling(double);
    bool get_dynamic_legend();
    double get_minimum();
    double get_maximum();
    double get_opacity();
    double get_refresh_time();
    double get_vector_scaling();

private:
    static MapProperty * obj;

    double prop_opacity;
    double prop_refresh_time;
    bool prop_dynamic_min_max;
    double prop_max;
    double prop_min;
    double prop_vector_scaling;
    // TODO: QVector<QPair<qreal, QColor> > prop_initramp;
    // TODO: Drawing on node/edge/face; dot, line or fill 
};
#endif _INC_MAP_PROPERTY
