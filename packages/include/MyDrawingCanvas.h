// RUN MLSupportCppHParser.exe
// corresponding multi language support files are generated

#ifndef _INC_MyCanvas
#define _INC_MyCanvas

#include <assert.h>
#include <math.h>       /* sqrt */

#include "AbstractCanvas.h"
#include "ugrid.h"
#include "QColorRampEditor.h"
#include "map_property.h"

#define GUI_EXPORT __declspec(dllimport)
#include "qgisinterface.h"
#include "qgsmaptool.h"
#include "qgsmaptoolemitpoint.h"
#include "qgspointxy.h"
#include "qgsmapcanvas.h"
#include "qgsmapcanvasitem.h"
#include "qgsmapmouseevent.h"

#include <qgsapplication.h>
#include <qgsgeometry.h>
#include <qgslayertree.h>
#include <qgslayertreegroup.h>

#include "qgsmaptool.h"
#include "qgsrubberband.h"


#include <QtCore/QVector>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <QtGui/QColor>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QWheelEvent>

#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>


#define NR_CACHES 1
#define IMAGE_WIDTH  3840 // 1420 // 
#define IMAGE_HEIGHT 2140 // 1080 // 

class MyCanvas : public QgsMapTool, public QgsMapCanvasItem
{
    Q_OBJECT

public:  // function containing the emit-statement
    void canvasDoubleClickEvent(QgsMapMouseEvent *);
    void canvasMoveEvent(QgsMapMouseEvent *);
    void canvasPressEvent(QgsMapMouseEvent *);
    void canvasReleaseEvent(QgsMapMouseEvent *) override;
    void wheelEvent(QWheelEvent *);

signals:  // functions send by the emit-statement
    void MouseDoubleClickEvent(QMouseEvent*);
    void MouseMoveEvent(QMouseEvent*);
    void MousePressEvent(QMouseEvent*);
    void MouseReleaseEvent(QgsMapMouseEvent *);
    void WheelEvent(QWheelEvent*);

public slots:
    void draw_all();

private slots:
    void renderPlugin(QPainter *);
    void renderCompletePlugin(QPainter *);
/********** Events a widget must handle or forward to plugin **********/
    void MyMouseDoubleClickEvent(QMouseEvent*);
    void MyMouseMoveEvent(QMouseEvent*);
    void MyMousePressEvent(QMouseEvent*);
    void MyMouseReleaseEvent(QgsMapMouseEvent *);
    void MyWheelEvent(QWheelEvent* e);

    void MyKeyPressEvent(QKeyEvent*);
    void MyKeyReleasedEvent(QKeyEvent*);


public:
    MyCanvas(QgisInterface *);
    ~MyCanvas();

    void empty_caches();

    void startDrawing(int);
    void finishDrawing();

    // Create a new draw cache in the frontend and it gets the given index.
    bool newCache(int index);

    // Get the cache index of default available draw cache of the map frontend.
    int getBackgroundCache();

    // Copy a background cache into the drawcache. Both caches are in the
    // frontend and their indexes are used.
    // The drawcache becomes the currently used cache to draw text and geometry.
    void copyCache(int backgroundCacheIndex, int drawCacheIndex);

    void drawDot(double x, double y);
    void drawMultiDot(vector<double>, vector<double>, vector<int> rgb);
    void drawPoint(double x, double y);  // Draw a single point with currently set colour by routine fillColor()
    void drawMultiPoint(vector<double> xs, vector<double> ys, vector<int> rgb);  // Draw an array of points according the given array of colours
    void drawPolygon(vector<double>, vector<double>);  // Draw a polygon. The polygon gets a color according the routine setFillColor

    void drawPolyline(vector<double>, vector<double>);  // Draw a polyline. This line width and line colour has to be set before this call
    void drawLineGradient(vector<double> xs, vector<double> ys, vector<int> rgb);

    void drawRectangle(double x, double y, int width, int height);

    // Draw the text. Textposition, font size and color has to bet set first by routines setFont.....
    // and setTextAlignment()
    void drawText(double x, double y, int width, int height, const char* text);

    void setPointSize(int size);
    void setLineWidth(int size);
    void setLineColor(int rgb);
    void setFillColor(int rgb);
    void setFontName(const char* name);
    void setFontColor(int rgb);
    void setFontPointSize(int size);
    void setFontItalic(bool value);
    void setFontBold(bool value);
    void setFontUnderline(bool value);
    bool isFontAvailable(const char* name);
    int getTextWidth(const char* name);
    int getTextHeight(const char* name, int maxWidth);

    double getPixelWidth(double x, double y);
    double getPixelHeight(double x, double y);

    double getMinX();
    double getMaxX();
    double getMinY();
    double getMaxY();
    double getMinVisibleX();
    double getMaxVisibleX();
    double getMinVisibleY();
    double getMaxVisibleY();
    int getWidth();
    int getHeight();
    int getPixelXFromXY(double x, double y);
    int getPixelYFromXY(double x, double y);
    double getXFromPixel(int pixelX, int pixelY);
    double getYFromPixel(int pixelX, int pixelY);
    int getPixelColor(double X, double Y);

    void setFullExtend(double minX, double maxX, double minY, double maxY);
    void zoomToExtend(double minX, double maxX, double minY, double maxY);

    void setUgridFile(UGRID *);
    void reset_min_max();
    void set_variable(struct _variable *);
    void set_variables(struct _mesh_variable * variables);
    void set_coordinate_type(QStringList);

    void set_layer(int);
    void set_current_step(int);
    void setColorRamp(QColorRampEditor *);

    void draw_dot_at_edge();
    void draw_dot_at_face();
    void draw_dot_at_node();
    void draw_data_along_edge();
    void draw_data_at_face();
    void draw_line_at_edge();
    void draw_vector_at_face();

    vector<double *> z_value;
    vector<double *> u_value;
    vector<double *> v_value;
    double m_z_min;
    double m_z_max;
    QColorRampEditor * m_ramph;

protected:
    // From world (document) coordinates to Qt
    int qx( double wx ) { return (int)( (     ((wx) - min_X)/window_width ) * (double)frame_width ); }
    int qy( double wy ) { return (int)( (1.0 -((wy) - min_Y)/window_height) * (double)frame_height); }
    // From Qt coordinates to world (document) coordinates
    double wx ( int qx ) { return ( min_X + ((qx)-1)*dx ); }
    double wy ( int qy ) { return ( min_Y + (frame_height - ((qy)+1))*dy ); }

private:
    // functions
    void paint( QPainter * );
    void determine_min_max(vector<double *>, double *, double *, vector<int> &, double);
    void determine_min_max(vector<double *>, double *, double *, double);
    double statistics_mode_length_of_cell(struct _variable *);

    // variables
    QgisInterface* mQGisIface;
    QPointer<QgsMapCanvas> mMapCanvas;
    QPointer<QgsMapCanvas> mMapCanvasItem;
    QPixmap * mMapCanvasMap;
    AbstractCanvas * mCanvas;

    AbstractCanvasListener* listener;
    bool drawing;
    QPainter* qgis_painter; // The painter object during a Plugin drawing sequence.
    QPainter* mCache_painter; // The painter object during a cache drawing sequence.

    QPaintDevice * pd;
    QWidget * w;
    QVBoxLayout * vb;
    QLabel * label[NR_CACHES];

    // The coordinate system transformation. Y is flipped.
    double scale; //
    double dx;    // distance of one pixel in world coordinates
    double dy;    // distance of one pixel in world coordinates
    int frame_width; // in pixels
    int frame_height; // in pixels
    double window_width;    // in world coordinates
    double window_height;   // in world coordinates

    double min_X, min_Y, max_X, max_Y; // extent
    double pixelWidth, pixelHeight;

    double radius; // For SetPointSize/DrawPoint. Measured in (fractional) pixels.
    QImage* buffer0;   // Draw cache for double buffering
    QImage* cacheArray[NR_CACHES];   // caches for the plugin
    QColor bgColour;
    QColor fillColor;
    QColor textColour;
    int drawCache;
    bool printing;

    void InitDrawEachCaches(void);
    void DrawEachCaches(void);

    UGRID * _ugrid_file;
    struct _variable * _variable;
    struct _mesh_variable * m_variables;
    QStringList m_coordinate_type;
    int m_layer;
    int _current_step;
    vector<long> dims;
    vector<double> mesh1d_x;
    vector<double> mesh1d_y;
    vector<int> rgb_color;
    vector<vector <double *>> std_dot_at_edge;
    vector<vector <double *>> std_data_at_node;
    vector<vector <double *>> std_data_at_face;
    MapProperty * m_property;
    bool m_vscale_determined;
    double m_mode_length;

};

#endif  /* _INC_MyCanvas */
