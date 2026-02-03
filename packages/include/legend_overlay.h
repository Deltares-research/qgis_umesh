#pragma once

#include <qgsmapcanvasitem.h>
#include <qgscolorramp.h>
#include <memory>
#include <QString>

class QPainter;
class QgsMapCanvas;

class NumericLegendOverlay : public QgsMapCanvasItem
{
public:
    explicit NumericLegendOverlay(QgsMapCanvas* canvas);

    static NumericLegendOverlay* getInstance(QgsMapCanvas* canvas);
    static int get_count();

    void setRamp(QgsColorRamp* ramp);
    void setTitle(const QString& title);
    void setRange(double min, double max);
    void setShow(bool show);
    QColor colorForValue(double value) const;
    void paint(QPainter* painter) override;

private:
    static NumericLegendOverlay* obj;
    static int count;

    // Optional but recommended
    NumericLegendOverlay(const NumericLegendOverlay&) = delete;
    NumericLegendOverlay& operator=(const NumericLegendOverlay&) = delete;

    QString mTitle = "Legend";
    double mMin = 0.0;
    double mMax = 1.0;
    bool mShowLegend;

    // Overlay owns the ramp safely
    QgsColorRamp*  mRamp;

    // Store canvas pointer for C++ usage
    QgsMapCanvas* mCanvas = nullptr;
};
