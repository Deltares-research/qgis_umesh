#include "legend_overlay.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QLinearGradient>

#include <qgsmapcanvas.h>

NumericLegendOverlay* NumericLegendOverlay::obj = nullptr;
int NumericLegendOverlay::count = 0;

// Constructor
NumericLegendOverlay::NumericLegendOverlay(QgsMapCanvas* canvas)
    : QgsMapCanvasItem(canvas)
    , mCanvas(canvas)   // store pointer safely
{
    ++count;
    mShowLegend = false;

    setZValue(1000);   // keep on top
    setVisible(true);
}
NumericLegendOverlay* NumericLegendOverlay::getInstance(QgsMapCanvas* canvas)
{
    if (obj == nullptr)
    {
        obj = new NumericLegendOverlay(canvas);
    }
    return obj;
}
int NumericLegendOverlay::get_count()
{
    return count;
}

//
void NumericLegendOverlay::setRamp(QgsColorRamp* ramp)
{
    mRamp = ramp;   // store pointer only
    update();
}

// --- Set the title ---
void NumericLegendOverlay::setTitle(const QString& title)
{
    mTitle = title;
    update();
}
void NumericLegendOverlay::setShow(bool show)
{
    mShowLegend = show;
    update();
}

// --- Set min/max range ---
void NumericLegendOverlay::setRange(double min, double max)
{
    mMin = min;
    mMax = max;
    if (mMax < mMin) 
    {
        // mMax always greater then mMin
        double t = mMax;
        mMax = mMin;
        mMin = t;
    }
    if (std::abs(mMax - mMin) < 0.001)
    {
        mMax = mMax + 0.001;
        mMin = mMin - 0.001;
    }
    update();
}

QColor NumericLegendOverlay::colorForValue(double value) const
{
    if (!mRamp || mMax <= mMin)
        return Qt::transparent;

    double t = (value - mMin) / (mMax - mMin);

    // Clamp ALWAYS
    t = std::clamp(t, 0.0, 1.0);

    // Handle inverted ramp once
    //if (mRamp->isInverted())
    t = 1.0 - t;

    return mRamp->color(t);
}




// --- Paint the legend ---
void NumericLegendOverlay::paint(QPainter* painter)
{
    if (!mCanvas || !mRamp)
        return;

    if (!mShowLegend)
    {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // --- Bottom-right legend box ---
    const int boxWidth  = 120;
    const int boxHeight = 210;
    const int margin    = 15;

    QSize canvasSize = mCanvas->size();

    QRect box(
        margin,  // canvasSize.width()  - boxWidth - margin, // right
        margin,  // canvasSize.height() - boxHeight - margin,  // bottom
        boxWidth,
        boxHeight
    );

    // --- Background ---
    painter->setPen(Qt::black);
    painter->setBrush(QColor(255, 255, 255, 255));
    painter->drawRoundedRect(box, 1, 1);

    // --- Title ---
    painter->setPen(Qt::black);
    painter->drawText(box.left() + 12, box.top() + 22, mTitle);

    // --- Gradient color bar ---
    QRect rampRect(box.left() + 25, box.top() + 45, 35, 140);

    QLinearGradient grad(rampRect.topLeft(), rampRect.bottomLeft());

    const int nSamples = 100; // smooth gradient
    for (int i = 0; i <= nSamples; ++i)
    {
        double t = 1.0 - double(i) / nSamples;
        //QColor c = mRamp->color(t);
        double value = mMin + (1.0 - t) * (mMax - mMin);
        QColor c = colorForValue(value);

        grad.setColorAt(double(i)/nSamples, c);
    }

    painter->setBrush(grad);
    painter->setPen(Qt::NoPen);
    painter->drawRect(rampRect);

    // --- Colorbar outline ---
    painter->setPen(QPen(Qt::black, 1));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rampRect);

    // --- Tick marks ---
    const int nTicks = 5;

    painter->setPen(QPen(Qt::black, 1));
    for (int i = 0; i < nTicks; ++i)
    {
        double frac = double(i) / (nTicks - 1);
        int y = rampRect.top() + int((1.0 - frac) * rampRect.height());

        // Tick line
        painter->drawLine(
            rampRect.right(),
            y,
            rampRect.right() + 8,
            y
        );

        // Numeric label
        double value = mMin + frac * (mMax - mMin);
        painter->drawText(
            rampRect.right() + 12,
            y + 4,
            QString::number(value, 'f', 3)
        );
    }

    painter->restore();
}
