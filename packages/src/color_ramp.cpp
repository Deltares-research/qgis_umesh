#include "color_ramp.h"
#include "QtWidgets/QMessageBox"

ColorRamp::ColorRamp()
{
    m_z_min =  std::numeric_limits<double>::infinity();
    m_z_max = -std::numeric_limits<double>::infinity();
    initialisation();
}
ColorRamp::~ColorRamp()
{
}
void ColorRamp::initialisation()
{
    m_default_ramp.push_back(QPair<qreal, QColor>(0.00, QColor(255,  0,  0)));
    m_default_ramp.push_back(QPair<qreal, QColor>(0.25, QColor(255,255,  0)));
    m_default_ramp.push_back(QPair<qreal, QColor>(0.50, QColor(  0,255,  0)));
    m_default_ramp.push_back(QPair<qreal, QColor>(0.75, QColor(  0,255,255)));
    m_default_ramp.push_back(QPair<qreal, QColor>(1.00, QColor(  0,  0,255)));
}
void ColorRamp::setMinMax(double z_min, double z_max)
{
    m_z_min = z_min;
    m_z_max = z_max;

    if (m_z_min == m_z_max)
    {
        int delta = log10(m_z_min) - 1;
        m_z_min = m_z_min - std::pow(10., delta);
        m_z_max = m_z_max + std::pow(10., delta);
    }
    
    for (int i=0; i<m_default_ramp.size(); i++)
    {
        double value = m_z_min + m_default_ramp[i].first * (m_z_max - m_z_min);
        QColor color = m_default_ramp[i].second;
        m_ramp.push_back(QPair<qreal, QColor>(value, color));
    }
}
int ColorRamp::get_rgb_from_value(double z_value)
{
    int color = qRgb(128, 128, 128);  // default grey
    for (int i = 1; i < m_ramp.size(); i++)
    {
        if (m_ramp[i].first >= z_value)
        {
            double alpha = (z_value - m_ramp[i - 1].first) / (m_ramp[i].first - m_ramp[i - 1].first);
            int r = m_ramp[i-1].second.red() + alpha * (m_ramp[i].second.red() - m_ramp[i-1].second.red());
            int g = m_ramp[i-1].second.green() + alpha * (m_ramp[i].second.green() - m_ramp[i-1].second.green());
            int b = m_ramp[i-1].second.blue() + alpha * (m_ramp[i].second.blue() - m_ramp[i-1].second.blue());
            color = qRgb(r, g, b);
            break;
        }
    }
    return color;
}
