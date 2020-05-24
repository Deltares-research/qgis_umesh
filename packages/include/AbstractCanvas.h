// RUN MLSupportCppHParser.exe
// corresponding multi language support files are generated

#ifndef _INC_ABSTRACTCANVAS
#define _INC_ABSTRACTCANVAS

#include "AbstractCanvasListener.h"

class AbstractCanvas {
    public:

    enum Alignment {
        ALIGN_LEFT       = 0x0001,
        ALIGN_RIGHT      = 0x0002,
        ALIGN_HCENTER    = 0x0004,
        ALIGN_TOP        = 0x0008,
        ALIGN_BOTTOM     = 0x0010,
        ALIGN_VCENTER    = 0x0020,
        ALIGN_CENTER     = ALIGN_VCENTER | ALIGN_HCENTER
    };

    enum MarkerStyle {
        MS_Circle   = 0,
        MS_Square   = 2,
        MS_Cross    = 3,
        MS_X        = 4,
        MS_Diamond  = 5
    };

    enum LineStyle {
        LS_NoLine    = 0,     // Qt::NoPen	no line at all. For example, QPainter::drawRect() fills but does not draw any boundary line.
        LS_SolidLine = 1,     // Qt::SolidLine	1	A plain line.
        LS_DashLine  = 2,     // Qt::DashLine	2	Dashes separated by a few pixels.
        LS_DotLine   = 3,     // Qt::DotLine	3	Dots separated by a few pixels.
        LS_DashDotLine = 4,   // Qt::DashDotLine	4	Alternate dots and dashes.
        LS_DashDotDotLine = 5 // Qt::DashDotDotLine	5	One dash, two dots, one dash, two dots.
    };


    virtual ~AbstractCanvas() {};

    virtual void setListener(AbstractCanvasListener* p) = 0;

    virtual void startDrawing(int index) = 0;
    virtual void finishDrawing() = 0;

    // Set the background colour (in Qt) with predefined colour names
    // or with a colour value based on rgb values.
    virtual void setCanvasBackgroundColor(const char* colourName) = 0;
    virtual void setCanvasBackgroundColorRgb(int colourIn) = 0;


    // Create a new draw cache in the frontend and it gets the given index.
    virtual bool newCache(int index) = 0;

    // Get the cache index of default available draw cache of the map frontend.
    virtual int getBackgroundCache() = 0;

    // Copy a background cache into the drawcache. Both caches are in the
    // frontend and their indexes are used.
    // The drawcache becomes the currently used cache to draw text and geometry.
    virtual void copyCache(int backgroundCacheIndex, int drawCacheIndex) = 0;

    // Copy a bitmap (first parameter) into another bitmap (second parameter).
    // It can be a cache bitmap or the bitmap of the window. Both are in the frontend.
    // The bitmap is indicated by the index of the cache. If the first parameter
    // is -1, then the bitmap of the frontend window is used.
    virtual void copyBitmap(int sourceIndex, int destinationIndex) = 0;

    // A set routines to draw marker symbols and to set some properties of
    // a marker symbol.
    // A marker symbol can be a circle, cross etc, see enumeration MarkerStyle
    // at the top of this file.
    // A marker symbol is drawn on a x,y position, where x,y are in world coordinates.
    // The current settings of the marker symbol are used when drawing the symbol.
    //
    virtual void drawMarkerSymbol(double x, double y) = 0;

    virtual void setMarkerSymbolStyle(MarkerStyle style) = 0;

    virtual void setMarkerSymbolColor(int rgb) = 0;

    virtual void setMarkerSymbolSize(int size) = 0;

    // Draw a single point with currently set colour by routine fillColor()
    virtual void drawDot(double x, double y) = 0;

    
    virtual void drawPoint(double x, double y) = 0;  // Draw a single point with currently set colour by routine fillColor()
//    virtual void drawMultiPoint(QVector<double> xs, QVector<double> ys, QVector<int> rgb) = 0;  // Draw an array of points according the given array of colours

    // Draw a polygon. The polygon gets a color according the routine setFillColor
    virtual void drawPolygon(int nrPoints,
        double* xs/*[nrPoints]*/ , double* ys/*[nrPoints]*/) = 0;

    virtual void drawRectangle(double x, double y, int width, int height) = 0;

    // Draw the text. Textposition, font size and color has to bet set first by routines setFont.....
    // and setTextAlignment()
    virtual void drawText(double x, double y, int width, int height, const char* text) = 0;

    virtual void setPointSize(int size) = 0;
    virtual void setLineWidth(int size) = 0;
    virtual void setLineColor(int rgb) = 0;
    virtual void setLineStyle(LineStyle style) = 0;
    virtual void setFillColor(int rgb) = 0;
    virtual void setFontName(const char* name) = 0;
    virtual void setFontColor(int rgb) = 0;
    virtual void setFontPointSize(int size) = 0;
    virtual void setFontItalic(bool value) = 0;
    virtual void setFontBold(bool value) = 0;
    virtual void setFontUnderline(bool value) = 0;
    virtual void setTextAlignment(Alignment value) = 0;
    virtual bool isFontAvailable(const char* name) = 0;
    virtual int getTextWidth(const char* name) = 0;
    virtual int getTextHeight(const char* name) = 0;

    virtual double getPixelWidth(double x, double y) = 0;
    virtual double getPixelHeight(double x, double y) = 0;

    virtual double getMinX() = 0;
    virtual double getMaxX() = 0;
    virtual double getMinY() = 0;
    virtual double getMaxY() = 0;
    virtual double getMinVisibleX() = 0;
    virtual double getMaxVisibleX() = 0;
    virtual double getMinVisibleY() = 0;
    virtual double getMaxVisibleY() = 0;
    virtual int getWidth() = 0;
    virtual int getHeight() = 0;
    virtual int getPixelXFromXY(double x, double y) = 0;
    virtual int getPixelYFromXY(double x, double y) = 0;
    virtual double getXFromPixel(int pixelX, int pixelY) = 0;
    virtual double getYFromPixel(int pixelX, int pixelY) = 0;
    virtual int getPixelColor(double X, double Y) = 0;



    virtual void setFullExtend(double minX, double maxX, double minY, double maxY) = 0;
    virtual void zoomToExtend(double minX, double maxX, double minY, double maxY) = 0;


};

#endif  /* _INC_ABSTRACTCANVAS */
