#pragma once

#include <qgsmapcanvasitem.h>
#include <qgscolorramp.h>
#include <memory>
#include <vector>
#include <QString>
#include <QPolygon>

class QPainter;
class QgsMapCanvas;

class UnitVectorOverlay : public QgsMapCanvasItem
{
public:
    explicit UnitVectorOverlay(QgsMapCanvas* canvas);
    ~UnitVectorOverlay();
    static UnitVectorOverlay* getInstance(QgsMapCanvas* canvas);
    static void deleteInstance();
    int get_count();

    void setShow(bool show);
    void setTitle(const QString& title);
    void setPolyline(std::vector<int> pix_x, std::vector<int> pix_y);
    void paint(QPainter* painter) override;

private:
    static UnitVectorOverlay* obj;
    static int count;

    // Optional but recommended
    UnitVectorOverlay(const UnitVectorOverlay&) = delete;
    UnitVectorOverlay& operator=(const UnitVectorOverlay&) = delete;

    bool mShowUnitVectorOverlay = false;
    const int m_boxWidth = 120;
    const int m_boxHeight = 45;
    const int m_margin = 15;

    QString mTitle = "--- x unit vector";
    QPolygon mPolyline;

    // Store canvas pointer for C++ usage
    QgsMapCanvas* mCanvas = nullptr;
};
