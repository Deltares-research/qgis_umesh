#include <QtWidgets/QApplication>
#include <QtWidgets/QDateTimeEdit>

class ColorRamp
{
public:
    ColorRamp();
    ~ColorRamp();

    void initialisation();
    void set_min_max(double, double);
    int get_rgb_from_value(double);

private:
    QVector<QPair<qreal, QColor> > m_default_ramp;
    QVector<QPair<qreal, QColor> > m_ramp;
    double m_z_min;
    double m_z_max;
};
