#include "unit_vector_overlay.h"
#include <QPainter>
#include <QPen>
#include <QBrush>

#include <qgsmapcanvas.h>

UnitVectorOverlay* UnitVectorOverlay::obj = nullptr;
int UnitVectorOverlay::count = 0;

// Constructor
UnitVectorOverlay::UnitVectorOverlay(QgsMapCanvas* canvas)
    : QgsMapCanvasItem(canvas)
    , mCanvas(canvas)   // store pointer safely
{
    ++count;
    setZValue(1001);   // keep on top
    setVisible(true);
}
UnitVectorOverlay::~UnitVectorOverlay()
{
    return;
}
UnitVectorOverlay* UnitVectorOverlay::getInstance(QgsMapCanvas* canvas)
{
    if (obj == nullptr)
    {
        obj = new UnitVectorOverlay(canvas);
    }
    return obj;
}
void UnitVectorOverlay::deleteInstance()
{
    --count;
    delete obj;
    obj = nullptr;
}
int UnitVectorOverlay::get_count()
{
    return count;
}
void UnitVectorOverlay::setShow(bool show)
{
    mShowUnitVectorOverlay = show;
    update();
}
void UnitVectorOverlay::setTitle(const QString& title)
{
    mTitle = title;
    update();
}
void UnitVectorOverlay::setPolyline(std::vector<int> pix_x, std::vector<int> pix_y)
{

    QSize canvasSize = mCanvas->size();
    QPolygon polyline;
    for (int i = 0; i < pix_x.size(); ++i)
    {
        polyline << QPoint(pix_x[i] + m_margin + 12,
                           pix_y[i] + canvasSize.height() - 0.33 * m_boxHeight - m_margin);
    }
    mPolyline = polyline;
    update();
}


// --- Paint the unit vector ---
void UnitVectorOverlay::paint(QPainter* painter)
{
    if (!mCanvas) { return; }
    if (!mShowUnitVectorOverlay) { return; }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // --- Bottom-right legend box ---
    QSize canvasSize = mCanvas->size();
    QRect box(
        m_margin,  // canvasSize.width()  - boxWidth - margin, // right
        canvasSize.height() - m_boxHeight - m_margin,  // bottom
        m_boxWidth,
        m_boxHeight
    );

    // --- Background ---
    painter->setPen(Qt::black);
    painter->setBrush(QColor(255, 255, 255, 255));
    painter->drawRoundedRect(box, 1, 1);

    // --- scaling unit vector ---    
    painter->setPen(Qt::black);
    painter->drawText(box.left() + 12, box.top() + 0.33 * m_boxHeight, mTitle);
    painter->drawPolyline(mPolyline);

    painter->restore();
}
