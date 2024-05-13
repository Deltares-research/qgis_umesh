#include <stdlib.h>
#include <string.h>

#define DLL_EXPORT
#include <qmath.h>
#include "MyDrawingCanvas.h"
#include "map_time_manager_window.h"
#include "color_ramp.h"
#include "perf_timer.h"

#include "qgsmapcanvas.h"
#include "qgsmapcanvasmap.h"
#include "qgsmaptopixel.h"
#include "qgsrubberband.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsmaptool.h"
#include "qgspoint.h"
#include <qgsdistancearea.h>

#if defined(WIN32) || defined(WIN64)
#  include <windows.h>
#  define strdup _strdup
#endif

using namespace std;

#define DRAW_CACHES false

//
// caches
// 0: drawing cache
// 5, 4, 3, 2, 1: stapled caches
// 6 contains the background picture
//
//-----------------------------------------------------------------------------
//
//MapProperty * MapProperty::obj;  // Initialize static member of class MapProperty (Singleton)
//
//
MyCanvas::MyCanvas(QgisInterface * QGisIface) :
    QgsMapTool(QGisIface->mapCanvas()),
    QgsMapCanvasItem(QGisIface->mapCanvas()),
    printing(false)
    {
    QgsMapTool::setCursor(Qt::CrossCursor);
    m_property = MapProperty::getInstance();

    mQGisIface = QGisIface;
    mMapCanvas = QGisIface->mapCanvas();
    mMapCanvasItem = QGisIface->mapCanvas();
    drawing = true;
    m_grid_file = nullptr;
    m_variable = nullptr;
    m_bed_layer = 0;
    m_hydro_layer = 0;
    m_current_step = 0;
    m_ramph = new QColorRampEditor();
    m_ramph_vec_dir = new QColorRampEditor();
    m_vscale_determined = false;
    m_vec_length = 0.0;
    m_vector_draw = VECTOR_NONE;

    qgis_painter = NULL;
    mCache_painter = new QPainter();

    //buffer0 = new QImage(IMAGE_WIDTH, IMAGE_HEIGHT, QImage::Format_ARGB32_Premultiplied);
    //buffer0->fill(Qt::transparent);

    for (int i=0; i<NR_CACHES; i++)
    {
        cacheArray[i] = NULL;
        newCache(i);
    }
    //
    // Render events
    // This calls the renderer everytime the canvas has drawn itself
    //
    QObject::connect(mMapCanvas, SIGNAL(renderComplete(QPainter *)), this, SLOT(renderCompletePlugin(QPainter *)));
    //
    // Key events
    //
    QObject::connect(mMapCanvas, SIGNAL(keyPressed(QKeyEvent *)), this, SLOT(MyKeyPressEvent(QKeyEvent *)));
    QObject::connect(mMapCanvas, SIGNAL(keyReleased(QKeyEvent *)), this, SLOT(MyKeyReleasedEvent(QKeyEvent *)));
    //
    // Mouse events
    //
    //connect(this, SIGNAL(MouseDoubleClickEvent(QMouseEvent *)), this, SLOT(MyMouseDoubleClickEvent(QMouseEvent *)));
    //connect(this, SIGNAL(MouseMoveEvent(QMouseEvent *)), this, SLOT(MyMouseMoveEvent(QMouseEvent *)));
    //connect(this, SIGNAL(MousePressEvent(QMouseEvent *)), this, SLOT(MyMousePressEvent(QMouseEvent *)));
    QObject::connect(this, SIGNAL(MouseReleaseEvent(QMouseEvent *)), this, SLOT(MyMouseReleaseEvent(QMouseEvent *)));
    QObject::connect(this, SIGNAL(WheelEvent(QWheelEvent *)), this, SLOT(MyWheelEvent(QWheelEvent *)));
    //connect(this, &QgsMapTool::wheelEvent, this, &MyCanvas::MyWheelEvent);


    QObject::connect(m_ramph, SIGNAL(rampChanged()), this, SLOT(draw_all()));
    QObject::connect(m_ramph_vec_dir, SIGNAL(rampChanged()), this, SLOT(draw_all()));

    if (DRAW_CACHES) {
        InitDrawEachCaches(); // debug utility
    }
    listener = NULL;
}
//
//-----------------------------------------------------------------------------
//
MyCanvas::~MyCanvas()
{
    for (int j = 0; j < NR_CACHES; j++)
    {
        if (cacheArray[j] != NULL)
        {
            delete cacheArray[j];
        }
    }
}
//-----------------------------------------------------------------------------
void MyCanvas::draw_all()
{
    //QMessageBox::information(0, "Message", "MyCanvas::draw_all");
    //draw_dot_at_edge();
    draw_data_at_face();  // control volume is the face
    draw_data_at_node();  // isofill of the control volume around node
    draw_dot_at_face();
    draw_dot_at_node();
    draw_data_along_edge();
    draw_line_at_edge();
    draw_vector_arrow();
    draw_vector_direction_at_face();
    draw_vector_direction_at_node();
    this->finishDrawing();
}
//-----------------------------------------------------------------------------
void MyCanvas::draw_dot_at_face()
{
    return;  // Todo:
    if (m_variable != nullptr && m_grid_file != nullptr && m_variable->location == "face")
    {
#if DO_TIMING == 1
        auto start = std::chrono::steady_clock::now();
#endif
        string var_name = m_variable->var_name;
        struct _mesh2d * mesh2d = m_grid_file->get_mesh_2d();
        DataValuesProvider2D<double> std_data_at_face = m_grid_file->get_variable_values(var_name);
        z_value = std_data_at_face.GetValueAtIndex(m_current_step, 0);

        double missing_value = m_variable->fill_value;
        m_rgb_color.resize(mesh2d->face[0]->x.size());
        determine_min_max(z_value, mesh2d->face[0]->x.size(), &m_z_min, &m_z_max, m_rgb_color, missing_value);

#if DO_TIMING == 1
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapse_time = end - start;
        QString msg = QString(tr("Timing reading dot at face: %2 [sec]").arg(elapse_time.count()));
        QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);

        start = std::chrono::steady_clock::now();
#endif

        this->startDrawing(CACHE_2D);
        double opacity = mCache_painter->opacity();
        mCache_painter->setOpacity(m_property->get_opacity());
        this->setPointSize(13);
        this->drawMultiDot(mesh2d->face[0]->x, mesh2d->face[0]->y, m_rgb_color);
        mCache_painter->setOpacity(opacity);
        this->finishDrawing();

#if DO_TIMING == 1
        end = std::chrono::steady_clock::now();
        elapse_time = end - start;
        msg = QString(tr("Timing drawing dot at face: %2 [sec]").arg(elapse_time.count()));
        QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
#endif
    }
}
//-----------------------------------------------------------------------------
void MyCanvas::draw_data_at_face()
{
    if (m_grid_file == nullptr) { return; }

    struct _mesh_variable* vars = m_grid_file->get_variables();
    struct _variable* var;
    for (int ivar = 0; ivar < vars->nr_vars; ++ivar)
    {
        var = vars->variable[ivar];
        if (var->draw && var->location == "face")
        {
#if DO_TIMING == 1
            auto start = std::chrono::steady_clock::now();
#endif
            string var_name = var->var_name;
            struct _mesh2d * mesh2d = m_grid_file->get_mesh_2d();
            if (var->dims.size() == 2) // 2D: time, xy_space
            {
                DataValuesProvider2D<double> std_data_at_face = m_grid_file->get_variable_values(var_name);
                z_value = std_data_at_face.GetValueAtIndex(m_current_step, 0);
            }
            else if (var->dims.size() == 3) // 3D: time, layer, xy_space
            {
                DataValuesProvider3D<double> std_data_at_face_3d = m_grid_file->get_variable_3d_values(var_name);
                if (var->sediment_index != -1)
                {
                    z_value = std_data_at_face_3d.GetValueAtIndex(m_current_step, var->sediment_index, 0);
                }
                else
                {
                    if (m_hydro_layer > 0)
                    {
                        z_value = std_data_at_face_3d.GetValueAtIndex(m_current_step, m_hydro_layer - 1, 0);
                    }
                    if (m_bed_layer > 0)
                    {
                        z_value = std_data_at_face_3d.GetValueAtIndex(m_current_step, m_bed_layer - 1, 0);
                    }
                }
            }
            else if (var->dims.size() == 4) // 4D: time, layer, sediment, xy_space
            {
                DataValuesProvider4D<double> std_data_at_face_4d = m_grid_file->get_variable_4d_values(var_name);
                if (var->sediment_index != -1)
                {
                    z_value = std_data_at_face_4d.GetValueAtIndex(m_current_step, m_bed_layer-1, var->sediment_index, 0);
                }
            }
            else
            {
            QMessageBox::information(0, tr("MyCanvas::draw_data_at_face()"), QString("Program error on variable: \"%1\"\nUnsupported number of dimensions (i.e. > 4).").arg(var_name.c_str()));
            }
            if (z_value == nullptr)
            {
                return;
            }

            double missing_value = var->fill_value;
            m_rgb_color.resize(mesh2d->face_nodes.size());
            determine_min_max(z_value, mesh2d->face_nodes.size(), &m_z_min, &m_z_max, missing_value);

#if DO_TIMING == 1
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapse_time = end - start;
            QString msg = QString(tr("Timing reading data at face: %2 [sec]").arg(elapse_time.count()));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);

            start = std::chrono::steady_clock::now();
#endif        
            this->startDrawing(CACHE_2D);
            mCache_painter->setPen(Qt::NoPen);  // The bounding line of the polygon is not drawn
            double opacity = mCache_painter->opacity();
            mCache_painter->setOpacity(m_property->get_opacity());
            vector<double> vertex_x(mesh2d->face_nodes[0].size());
            vector<double> vertex_y(mesh2d->face_nodes[0].size());
            this->setPointSize(13);
            for (int i = 0; i < mesh2d->face_nodes.size(); i++)
            {
                vertex_x.clear();
                vertex_y.clear();
                bool in_view = false;
                if (z_value[i] != missing_value)
                {
                    for (int j = 0; j < mesh2d->face_nodes[i].size(); j++)
                    {
                        int p1 = mesh2d->face_nodes[i][j];
                        if (p1 > -1)
                        {
                            vertex_x.push_back(mesh2d->node[0]->x[p1]);
                            vertex_y.push_back(mesh2d->node[0]->y[p1]);
                            if (mesh2d->node[0]->x[p1] > getMinVisibleX() && mesh2d->node[0]->x[p1] < getMaxVisibleX() &&
                                mesh2d->node[0]->y[p1] > getMinVisibleY() && mesh2d->node[0]->y[p1] < getMaxVisibleY())
                            {
                                in_view = true; // point in visible area, do not skip thi polygon
                            }
                        }
                    }
                }
                if (in_view)
                {
                    QColor col = m_ramph->getRgbFromValue(z_value[i]);
                    double alpha = min(col.alphaF(), m_property->get_opacity());
                    mCache_painter->setOpacity(alpha);
                    this->setFillColor(col);
                    this->drawPolygon(vertex_x, vertex_y);
                }
            }
            mCache_painter->setOpacity(opacity);

#if DO_TIMING == 1
            end = std::chrono::steady_clock::now();
            elapse_time = end - start;
            msg = QString(tr("Timing drawing data at face: %2 [sec]\n").arg(elapse_time.count()));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
#endif
        }
    }

}
//-----------------------------------------------------------------------------
void MyCanvas::draw_data_at_node()
{
    // isofill of the control volume around node (Element based Finite Volume Method)
    if (m_grid_file == nullptr) { return; }

    struct _mesh_variable* vars = m_grid_file->get_variables();
    struct _variable* var;
    for (int ivar = 0; ivar < vars->nr_vars; ++ivar)
    {
        var = vars->variable[ivar];
        if (var->draw && var->location == "node")
        {
#if DO_TIMING == 1
            auto start = std::chrono::steady_clock::now();
#endif
            string var_name = var->var_name;
            struct _mesh2d* mesh2d = m_grid_file->get_mesh_2d();
            if (mesh2d == nullptr) { return; }
            DataValuesProvider2D<double> std_data_at_node = m_grid_file->get_variable_values(var_name);
            if (std_data_at_node.m_numXY == 0)
            {
                return;
            }
            z_value = std_data_at_node.GetValueAtIndex(m_current_step, 0);

            double missing_value = var->fill_value;
            m_rgb_color.resize(mesh2d->node[0]->x.size());
            determine_min_max(z_value, mesh2d->node[0]->y.size(), &m_z_min, &m_z_max, missing_value);

#if DO_TIMING == 1
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapse_time = end - start;
            QString msg = QString(tr("Timing reading data at node: %2 [sec]").arg(elapse_time.count()));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);

            start = std::chrono::steady_clock::now();
#endif        

            this->startDrawing(CACHE_2D);
            mCache_painter->setPen(Qt::NoPen);  // The bounding line of the polygon is not drawn
            mCache_painter->setOpacity(m_property->get_opacity());
            int nr_nodes_per_max_quad = 4;
            vector<double> vertex_x(nr_nodes_per_max_quad);
            vector<double> vertex_y(nr_nodes_per_max_quad);
            this->setPointSize(13);
            for (int i = 0; i < mesh2d->face_nodes.size(); i++)
            {
                vertex_x.clear();
                vertex_y.clear();
                bool in_view = false;
                int p0, p1, p2, p3;

                p0 = mesh2d->face_nodes[i][0];
                p1 = mesh2d->face_nodes[i][1];
                p2 = mesh2d->face_nodes[i][2];
                p3 = mesh2d->face_nodes[i][3];
                in_view = false;
                if (mesh2d->node[0]->x[p0] > getMinVisibleX() && mesh2d->node[0]->x[p0] < getMaxVisibleX() &&
                    mesh2d->node[0]->y[p0] > getMinVisibleY() && mesh2d->node[0]->y[p0] < getMaxVisibleY() ||
                    mesh2d->node[0]->x[p1] > getMinVisibleX() && mesh2d->node[0]->x[p1] < getMaxVisibleX() &&
                    mesh2d->node[0]->y[p1] > getMinVisibleY() && mesh2d->node[0]->y[p1] < getMaxVisibleY() ||
                    mesh2d->node[0]->x[p2] > getMinVisibleX() && mesh2d->node[0]->x[p1] < getMaxVisibleX() &&
                    mesh2d->node[0]->y[p2] > getMinVisibleY() && mesh2d->node[0]->y[p1] < getMaxVisibleY() ||
                    mesh2d->node[0]->x[p3] > getMinVisibleX() && mesh2d->node[0]->x[p3] < getMaxVisibleX() &&
                    mesh2d->node[0]->y[p3] > getMinVisibleY() && mesh2d->node[0]->y[p3] < getMaxVisibleY() )
                {
                    in_view = true; // whole face is inview, todo: could be tested on subcontrol volumes
                }

                if (in_view)
                {
                    double edge_x_01 = 0.5 * (mesh2d->node[0]->x[p0] + mesh2d->node[0]->x[p1]);
                    double edge_y_01 = 0.5 * (mesh2d->node[0]->y[p0] + mesh2d->node[0]->y[p1]);
                    double edge_x_12 = 0.5 * (mesh2d->node[0]->x[p1] + mesh2d->node[0]->x[p2]);
                    double edge_y_12 = 0.5 * (mesh2d->node[0]->y[p1] + mesh2d->node[0]->y[p2]);
                    double edge_x_23 = 0.5 * (mesh2d->node[0]->x[p2] + mesh2d->node[0]->x[p3]);
                    double edge_y_23 = 0.5 * (mesh2d->node[0]->y[p2] + mesh2d->node[0]->y[p3]);
                    double edge_x_31 = 0.5 * (mesh2d->node[0]->x[p3] + mesh2d->node[0]->x[p0]);
                    double edge_y_31 = 0.5 * (mesh2d->node[0]->y[p3] + mesh2d->node[0]->y[p0]);
                    double centre_x = 0.;
                    double centre_y = 0.;
                    for (int j = 0; j < nr_nodes_per_max_quad; j++)
                    {
                        p0 = mesh2d->face_nodes[i][j];
                        centre_x += mesh2d->node[0]->x[p0];
                        centre_y += mesh2d->node[0]->y[p0];
                    }
                    centre_x /= nr_nodes_per_max_quad;
                    centre_y /= nr_nodes_per_max_quad;

                    // determine the polygons [p, edge, centre, edge]
                    vertex_x.clear();
                    vertex_y.clear();
                    p0 = mesh2d->face_nodes[i][0];
                    vertex_x.push_back(mesh2d->node[0]->x[p0]);
                    vertex_y.push_back(mesh2d->node[0]->y[p0]);
                    vertex_x.push_back(edge_x_01);
                    vertex_y.push_back(edge_y_01);
                    vertex_x.push_back(centre_x);
                    vertex_y.push_back(centre_y);
                    vertex_x.push_back(edge_x_31);
                    vertex_y.push_back(edge_y_31);
                    // draw polygon
                    QColor col = m_ramph->getRgbFromValue(z_value[p0]);
                    double alpha = min(col.alphaF(), m_property->get_opacity());
                    mCache_painter->setOpacity(alpha);
                    this->setFillColor(col);
                    this->drawPolygon(vertex_x, vertex_y);


                    // determine the polygons [p, edge, centre, edge]
                    vertex_x.clear();
                    vertex_y.clear();
                    p0 = mesh2d->face_nodes[i][1];
                    vertex_x.push_back(mesh2d->node[0]->x[p0]);
                    vertex_y.push_back(mesh2d->node[0]->y[p0]);
                    vertex_x.push_back(edge_x_12);
                    vertex_y.push_back(edge_y_12);
                    vertex_x.push_back(centre_x);
                    vertex_y.push_back(centre_y);
                    vertex_x.push_back(edge_x_01);
                    vertex_y.push_back(edge_y_01);
                    // draw polygon
                    col = m_ramph->getRgbFromValue(z_value[p0]);
                    alpha = min(col.alphaF(), m_property->get_opacity());
                    mCache_painter->setOpacity(alpha);
                    this->setFillColor(col);
                    this->drawPolygon(vertex_x, vertex_y);


                    // determine the polygons [p, edge, centre, edge]
                    vertex_x.clear();
                    vertex_y.clear();
                    p0 = mesh2d->face_nodes[i][2];
                    vertex_x.push_back(mesh2d->node[0]->x[p0]);
                    vertex_y.push_back(mesh2d->node[0]->y[p0]);
                    vertex_x.push_back(edge_x_23);
                    vertex_y.push_back(edge_y_23);
                    vertex_x.push_back(centre_x);
                    vertex_y.push_back(centre_y);
                    vertex_x.push_back(edge_x_12);
                    vertex_y.push_back(edge_y_12);
                    // draw polygon
                    col = m_ramph->getRgbFromValue(z_value[p0]);
                    alpha = min(col.alphaF(), m_property->get_opacity());
                    mCache_painter->setOpacity(alpha);
                    this->setFillColor(col);
                    this->drawPolygon(vertex_x, vertex_y);


                    // determine the polygons [p, edge, centre, edge]
                    vertex_x.clear();
                    vertex_y.clear();
                    p0 = mesh2d->face_nodes[i][3];
                    vertex_x.push_back(mesh2d->node[0]->x[p0]);
                    vertex_y.push_back(mesh2d->node[0]->y[p0]);
                    vertex_x.push_back(edge_x_31);
                    vertex_y.push_back(edge_y_31);
                    vertex_x.push_back(centre_x);
                    vertex_y.push_back(centre_y);
                    vertex_x.push_back(edge_x_23);
                    vertex_y.push_back(edge_y_23);
                    // draw polygon
                    col = m_ramph->getRgbFromValue(z_value[p0]);
                    alpha = min(col.alphaF(), m_property->get_opacity());
                    mCache_painter->setOpacity(alpha);
                    this->setFillColor(col);
                    this->drawPolygon(vertex_x, vertex_y);
                }
            }
#if DO_TIMING == 1
            end = std::chrono::steady_clock::now();
            elapse_time = end - start;
            msg = QString(tr("Timing drawing data at node: %2 [sec]\n").arg(elapse_time.count()));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
#endif
        }
    }
}
//-----------------------------------------------------------------------------
void MyCanvas::set_draw_vector(vector_quantity vector_draw)
{
    m_vector_draw = vector_draw;
    draw_vector_arrow();
    draw_vector_direction_at_face();
    draw_vector_direction_at_node();
}
//-----------------------------------------------------------------------------
void MyCanvas::draw_vector_arrow()
{
    if (m_vector_draw != VECTOR_ARROW) { return; }
    if (m_grid_file == nullptr) { return; }
    struct _mesh2d_string ** m2d_string = m_grid_file->get_mesh2d_string();
    struct _mesh2d * mesh2d = m_grid_file->get_mesh_2d();
    if (mesh2d != nullptr)
    {
        vector<double> coord_x(5);
        vector<double> coord_y(5);
        vector<double> coord_ax(2);
        vector<double> coord_ay(2);
        vector<double> coord_bx(2);
        vector<double> coord_by(2);
        vector<double> dx(5);
        vector<double> dy(5);
        size_t dimens = -1;
        double vscale;
        double missing_value = -INFINITY;
        double b_len;

        if (m_coordinate_type.size() != 4) { return; }

        mCache_painter->setOpacity(m_property->get_opacity());
        // get average cell size (ie sqrt(area))    
        struct _mesh_variable * vars = m_grid_file->get_variables();
        if (!m_vscale_determined)
        {
            struct _variable * cell_area = m_grid_file->get_var_by_std_name(vars, m2d_string[0]->var_name, "cell_area");
            if (cell_area == nullptr) 
            { 
                m_vec_length = 1.;
                QString msg = QString("Variable \'cell_area\' not found. Vectors are scaled with a factor %1.").arg(m_vec_length);
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Warning, true);
            }
            else
            {
                m_vec_length = statistics_averaged_length_of_cell(cell_area);
            }

            if (m_coordinate_type[0] == "Spherical")
            {
                // TODO scale to degrees
                double radius_earth = 6371008.771;
                m_vec_length = m_vec_length / (2.0 * M_PI * radius_earth / 360.0);  // 1 degree on the great circle
            }
            m_vscale_determined = true;
        }
        double prop_fac = m_property->get_vector_scaling();
        vscale = prop_fac * m_vec_length;

        double vlen;
        double beta;
        dx[0] = 0.0;  // not used
        dx[1] = 1.0;
        dx[2] = 0.8;
        dx[3] = 0.8;
        dx[4] = 0.0;  // not used

        dy[0] = 0.0;  // not used
        dy[1] = 0.0;
        dy[2] = -0.1;
        dy[3] = 0.1;
        dy[4] = 0.0;  // not used
 
        this->setLineColor(1);
        this->setFillColor(1);
        this->setPointSize(7);
        this->setLineWidth(1);

        if (m_coordinate_type[0] == "Cartesian" || m_coordinate_type[0] == "Spherical")
        {
            // 
            int numXY = 0;
            int i_var = 0;
            for (int i = 0; i < vars->nr_vars; ++i)
            {
                if (vars->variable[i]->var_name == m_coordinate_type[1].toStdString())
                {
                    dimens = vars->variable[i]->dims.size();
                    missing_value = vars->variable[i]->fill_value;
                    i_var = i;
                }
            }
            if (dimens == 2  ||
                dimens == 3 && m_grid_file->get_file_type() == FILE_TYPE::SGRID ||
                dimens == 3 && m_grid_file->get_file_type() == FILE_TYPE::KISS) // 2D: time, nodes (= imax*jamx)
            {
                DataValuesProvider2D<double>std_u_vec_at_node = m_grid_file->get_variable_values(m_coordinate_type[1].toStdString());
                u_value = std_u_vec_at_node.GetValueAtIndex(m_current_step, 0);
                numXY = std_u_vec_at_node.m_numXY;
                DataValuesProvider2D<double>std_v_vec_at_node = m_grid_file->get_variable_values(m_coordinate_type[2].toStdString());
                v_value = std_v_vec_at_node.GetValueAtIndex(m_current_step, 0);
            }
            else if (dimens == 3) // 3D: time, layer, nodes
            {
                if (m_hydro_layer > 0)
                {
                    DataValuesProvider3D<double> std_u_vec_at_face_3d = m_grid_file->get_variable_3d_values(m_coordinate_type[1].toStdString());
                    numXY = std_u_vec_at_face_3d.m_numXY;
                    u_value = std_u_vec_at_face_3d.GetValueAtIndex(m_current_step, m_hydro_layer - 1, 0);
                    DataValuesProvider3D<double> std_v_vec_at_face_3d = m_grid_file->get_variable_3d_values(m_coordinate_type[2].toStdString());
                    v_value = std_v_vec_at_face_3d.GetValueAtIndex(m_current_step, m_hydro_layer - 1, 0);
                }
            }
            else
            {
                QMessageBox::information(0, "MapTimeManagerWindow::draw_time_dependent_vector", QString("Layer velocities not yet implemented"));
            }
            if (u_value == nullptr || v_value == nullptr)
            {
                return;
            }
            this->startDrawing(CACHE_VEC_2D);
            //rgb_color.resize(u_value.size());
            //for (int i = 0; i < u_value.size(); i++)
            //{
            //    rgb_color[i] = qRgba(1, 0, 0, 255);
            //}
            //this->setPointSize(3); 
            //this->setFillColor(qRgba(0, 0, 255, 255));  
            
            for (int i = 0; i < numXY; i++)
            {
                if (u_value[i] == missing_value) { continue; }  // *u_value[i] = 0.0;
                if (v_value[i] == missing_value) { continue; }  // *v_value[i] = 0.0;

                coord_x.clear();
                coord_y.clear();
                coord_ax.assign(coord_ax.size(), 0.0);
                coord_ay.assign(coord_ay.size(), 0.0);
                coord_bx.assign(coord_bx.size(), 0.0);
                coord_by.assign(coord_by.size(), 0.0);

                if (vars->variable[i_var]->location == "node")
                {
                    coord_x.push_back(mesh2d->node[0]->x[i]);
                    coord_y.push_back(mesh2d->node[0]->y[i]);
                }
                if (vars->variable[i_var]->location == "face")
                {
                    coord_x.push_back(mesh2d->face[0]->x[i]);
                    coord_y.push_back(mesh2d->face[0]->y[i]);
                }

                vlen = sqrt(u_value[i] * u_value[i] + v_value[i] * v_value[i]);  // The "length" of the vector
                beta = atan2(v_value[i], u_value[i]);
                if (vscale * vlen < 0.00001)
                {
                    vlen = 0.0;
                }
                else
                {
                    vlen = max(vscale * vlen, 0.0001);
                }
                for (int k = 1; k < 4; k++)
                {
                    coord_x.push_back(coord_x[0] + vlen * (cos(beta) * dx[k] - sin(beta) * dy[k]));
                    coord_y.push_back(coord_y[0] + vlen * (sin(beta) * dx[k] + cos(beta) * dy[k]));
                }

                coord_x.push_back(coord_x[1]);
                coord_y.push_back(coord_y[1]);

                //TODO: Incase geographic mesh and cartesian projection, arrow need to be transformed to cartesian projection

                QgsCoordinateReferenceSystem s_crs = QgsProject::instance()->crs();  // CRS of the screen
                if (m_coordinate_type[0] == "Spherical" && s_crs.isGeographic())  // if screen is cartesian, then cartesian vectors are drwan
                {
                    // Screen CRS must be WGS84 4326 CRS layer should be the same as the presentation CRS
                    if (s_crs != QgsCoordinateReferenceSystem("EPSG:4326"))
                    {
                        QMessageBox::information(0, "Message", QString("The data CRS (EPSG:4326) and the screen CRS (%1) are not both equal to EPSG:4326 (WGS84).\nPlease ensure that both CRS's are equal to EPSG:4326 (WGS84) before vectors will be drawn.").arg(s_crs.authid()));
                        return;
                    }

                    double x_tmp;
                    double y_tmp;
                    double fac = std::cos(M_PI / 180.0 * coord_y[0]);  // Typical sphere coordinate transformation, only suitable for WGS84 (EPSG:4326)

                    coord_x[1] = coord_x[1];
                    coord_y[1] = coord_y[0] + fac * (coord_y[1] - coord_y[0]);

                    coord_bx[0] = coord_x[0] + 0.8 * (coord_x[1] - coord_x[0]);
                    coord_by[0] = coord_y[0] + 0.8 * (coord_y[1] - coord_y[0]);

                    double eps = 1e-8;
                    if (abs(coord_y[1] - coord_y[0]) > eps)
                    {
                        b_len = 0.1 * vlen;  // vlen is length of original vector on the nc-file
                        coord_bx[1] = b_len + coord_bx[0];
                        coord_by[1] = -(coord_x[1] - coord_x[0]) * b_len / (coord_y[1] - coord_y[0]) + coord_by[0];

                        //double vlen_b = sqrt((coord_bx[1] - coord_bx[0]) * (coord_bx[1] - coord_bx[0]) + (coord_by[1] - coord_by[0]) * (coord_by[1] - coord_by[0]));
                        double vlen_b = sqrt(b_len * b_len + (coord_by[1] - coord_by[0]) * (coord_by[1] - coord_by[0]));
                        
                        //double b_len = 0.1 * vlen / vlen_b;
                        x_tmp = 0.1 * vlen * (coord_bx[1] - coord_bx[0]) / (vlen_b);
                        y_tmp = 0.1 * vlen * (coord_by[1] - coord_by[0]) / (vlen_b);

                        coord_x[2] = coord_bx[0] - x_tmp;
                        coord_y[2] = coord_by[0] - y_tmp;
                        coord_x[3] = coord_bx[0] + x_tmp;
                        coord_y[3] = coord_by[0] + y_tmp;
                    }
                    else
                    {
                        // y_1 - y_0 === 0
                        b_len = 0.1 * vlen;
                        coord_bx[1] = b_len + coord_bx[0];
                        coord_by[1] = b_len + coord_by[0];
                        //vlen = sqrt((coord_x[1] - coord_x[0]) * (coord_x[1] - coord_x[0]) + (coord_y[1] - coord_y[0]) * (coord_y[1] - coord_y[0]));  // The "length" of the vector
                        vlen = coord_x[1] - coord_x[0];  // The "length" of the vector
                        coord_x[2] = coord_x[0] + 0.8 * vlen;
                        coord_y[2] = coord_y[0] + 0.1 * vlen;
                        coord_x[3] = coord_x[0] + 0.8 * vlen;
                        coord_y[3] = coord_y[0] - 0.1 * vlen;
                    }
                    coord_x[4] = coord_x[1];
                    coord_y[4] = coord_y[1];
                }
                bool in_view = false;
                for (int j = 0; j < coord_x.size(); j++)
                {
                    if (coord_x[j] > getMinVisibleX() && coord_x[j] < getMaxVisibleX() &&
                        coord_y[j] > getMinVisibleY() && coord_y[j] < getMaxVisibleY())
                    {
                        in_view = true; // root of vector not in visible area, skip this vector
                    }
                }

                if (in_view)
                {
                    this->drawPolyline(coord_x, coord_y);
                }
            }

            // set scaled unit vector in right lower corner 
            this->setLineWidth(1);
            coord_x.clear();
            coord_y.clear();
            double alpha = 0.95;
            double unitv_x_head = (1.0 - alpha) * getMinVisibleX() + alpha * getMaxVisibleX();
            alpha = 0.05;
            double unitv_y_head = (1.0 - alpha) * getMinVisibleY() + alpha * getMaxVisibleY();
 
            pix_dx = getPixelWidth(coord_x[0], coord_y[0]);
            pix_dy = getPixelHeight(coord_x[0], coord_y[0]);
                
            int tw = getTextWidth("Unit vector");
            int th = getTextHeight("Unit vector");
            int vw = 2 * tw;  // vector width (length)

            double v_length = wx(vw) - wx(0);
            coord_x.clear();
            coord_y.clear();
            coord_x.push_back(unitv_x_head - v_length);
            coord_y.push_back(unitv_y_head);
            coord_x.push_back(unitv_x_head);
            coord_y.push_back(unitv_y_head);
            coord_x.push_back(unitv_x_head - 0.2 * v_length);
            coord_y.push_back(unitv_y_head + 0.1 * v_length);
            coord_x.push_back(unitv_x_head - 0.2 * v_length);
            coord_y.push_back(unitv_y_head - 0.1 * v_length);
            coord_x.push_back(unitv_x_head);
            coord_y.push_back(unitv_y_head);

            this->setFillColor(QColor(255, 255, 255));
            this->setLineWidth(1);
            mCache_painter->setOpacity(0.66);
            int vh = qy(coord_y[3]) - qy(coord_y[2]);  // vector height
            int width = 5 + vw + 5;
            int height = -(5 + vh + 3 + th + 5);
            this->drawRectangle(coord_x[0] - 5. * pix_dx, coord_y[3] - 5. * pix_dy, width, height);

            mCache_painter->setOpacity(1.0);
            char scratch[50];
            sprintf(scratch, "%3.3f", v_length / vscale);
            string txt(scratch);
            string text = txt + " x unit vector";
            this->drawText(coord_x[0], coord_y[0] + (5 + th) * pix_dy, 0, 0, text.c_str());

            this->drawPolyline(coord_x, coord_y);

            this->finishDrawing();
        }
        //else if (m_coordinate_type[0] == "Spherical")
        {
            // east_sea_water_velocity, north_sea_water_velocity
        }
    }
}
//-----------------------------------------------------------------------------
void MyCanvas::draw_vector_direction_at_face()
{
    if (m_vector_draw != VECTOR_DIRECTION) { return; }
    // draw the vector direction in the range [0, 360) and with cyclic colorramp
    if (m_grid_file == nullptr) { return; }
    struct _mesh2d * mesh2d = m_grid_file->get_mesh_2d();
    if (mesh2d != nullptr)
    {
        size_t dimens = 0;
        double missing_value = -INFINITY;

        if (m_coordinate_type.size() != 4) { return; }
        struct _mesh_variable* vars = m_grid_file->get_variables();
        struct _variable* var;
        int i_var = 0;
        for (int i = 0; i < vars->nr_vars; i++)
        {
            if (vars->variable[i]->var_name == m_coordinate_type[1].toStdString())
            {
                dimens = vars->variable[i]->dims.size();
                missing_value = vars->variable[i]->fill_value;
                i_var = i;
            }
        }
        var = vars->variable[i_var];
        if (!var->draw && var->location != "face") { return; }

        if (dimens == 2 ||
            dimens == 3 && m_grid_file->get_file_type() == FILE_TYPE::SGRID ||
            dimens == 3 && m_grid_file->get_file_type() == FILE_TYPE::KISS) // 2D: time, nodes (= imax*jmax)
        {
            DataValuesProvider2D<double>std_u_vec_at_face = m_grid_file->get_variable_values(m_coordinate_type[1].toStdString());
            u_value = std_u_vec_at_face.GetValueAtIndex(m_current_step, 0);
            DataValuesProvider2D<double>std_v_vec_at_face = m_grid_file->get_variable_values(m_coordinate_type[2].toStdString());
            v_value = std_v_vec_at_face.GetValueAtIndex(m_current_step, 0);
        }
        else if (dimens == 3) // 3D: time, layer, nodes
        {
            if (m_hydro_layer > 0)
            {
                DataValuesProvider3D<double> std_u_vec_at_face_3d = m_grid_file->get_variable_3d_values(m_coordinate_type[1].toStdString());
                u_value = std_u_vec_at_face_3d.GetValueAtIndex(m_current_step, m_hydro_layer - 1, 0);
                DataValuesProvider3D<double> std_v_vec_at_face_3d = m_grid_file->get_variable_3d_values(m_coordinate_type[2].toStdString());
                v_value = std_v_vec_at_face_3d.GetValueAtIndex(m_current_step, m_hydro_layer - 1, 0);
            }
        }
        if (u_value == nullptr || v_value == nullptr)
        {
            return;
        }

        vector<double> vec_z(mesh2d->face_nodes.size(), 0.0); // will contain the direction
        m_rgb_color.resize(mesh2d->face_nodes.size());
        m_ramph_vec_dir->update();

        this->startDrawing(CACHE_2D);
        mCache_painter->setPen(Qt::NoPen);  // The bounding line of the polygon is not drawn
        double opacity = mCache_painter->opacity();
        mCache_painter->setOpacity(m_property->get_opacity());
        vector<double> vertex_x(mesh2d->face_nodes[0].size());
        vector<double> vertex_y(mesh2d->face_nodes[0].size());
        for (int i = 0; i < mesh2d->face_nodes.size(); i++)
        {
            bool in_view = false;
            if (u_value[i] == missing_value) { continue; }  // *u_value[i] = 0.0;
            if (v_value[i] == missing_value) { continue; }  // *v_value[i] = 0.0;
            vertex_x.clear();
            vertex_y.clear();
            for (int j = 0; j < mesh2d->face_nodes[i].size(); j++)
            {
                int p1 = mesh2d->face_nodes[i][j];
                if (p1 > -1)
                {
                    vertex_x.push_back(mesh2d->node[0]->x[p1]);
                    vertex_y.push_back(mesh2d->node[0]->y[p1]);
                    if (mesh2d->node[0]->x[p1] > getMinVisibleX() && mesh2d->node[0]->x[p1] < getMaxVisibleX() &&
                        mesh2d->node[0]->y[p1] > getMinVisibleY() && mesh2d->node[0]->y[p1] < getMaxVisibleY())
                    {
                        vec_z[i] = atan2(v_value[i], u_value[i]) * 360.0 / (2.0 * M_PI);
                        in_view = true; // one point of polygon in visible area, do not skip this polygon
                    }
                }
            }
            if (in_view && vec_z[i] != missing_value)
            {
                setFillColor(m_ramph_vec_dir->getRgbFromValue(vec_z[i]));
                this->drawPolygon(vertex_x, vertex_y);
            }
        }
        mCache_painter->setOpacity(opacity);
        this->finishDrawing();
    }
}
//-----------------------------------------------------------------------------
void MyCanvas::draw_vector_direction_at_node()
{
    // isofill of the control volume around node (Element based Finite Volume Method)
    if (m_vector_draw != VECTOR_DIRECTION) { return; }
    // draw the vector direction in the range [0, 360) and with cyclic colorramp
    if (m_grid_file == nullptr) { return; }
    struct _mesh2d* mesh2d = m_grid_file->get_mesh_2d();
    if (mesh2d != nullptr)
    {
        size_t dimens = 0;
        double missing_value = -INFINITY;

        if (m_coordinate_type.size() != 4) { return; }
        struct _mesh_variable* vars = m_grid_file->get_variables();
        struct _variable* var;
        int i_var = 0;
        for (int i = 0; i < vars->nr_vars; ++i)
        {
            if (vars->variable[i]->var_name == m_coordinate_type[1].toStdString())
            {
                dimens = vars->variable[i]->dims.size();
                missing_value = vars->variable[i]->fill_value;
                i_var = i;
            }
        }
        var = vars->variable[i_var];
        if (!var->draw && var->location != "node") { return; }

        if (dimens == 2 ||
            dimens == 3 && m_grid_file->get_file_type() == FILE_TYPE::SGRID ||
            dimens == 3 && m_grid_file->get_file_type() == FILE_TYPE::KISS) // 2D: time, nodes (= imax*jmax)
        {
            DataValuesProvider2D<double>std_u_vec_at_face = m_grid_file->get_variable_values(m_coordinate_type[1].toStdString());
            u_value = std_u_vec_at_face.GetValueAtIndex(m_current_step, 0);
            DataValuesProvider2D<double>std_v_vec_at_face = m_grid_file->get_variable_values(m_coordinate_type[2].toStdString());
            v_value = std_v_vec_at_face.GetValueAtIndex(m_current_step, 0);
        }
        else if (dimens == 3) // 3D: time, layer, nodes
        {
            if (m_hydro_layer > 0)
            {
                DataValuesProvider3D<double> std_u_vec_at_face_3d = m_grid_file->get_variable_3d_values(m_coordinate_type[1].toStdString());
                u_value = std_u_vec_at_face_3d.GetValueAtIndex(m_current_step, m_hydro_layer - 1, 0);
                DataValuesProvider3D<double> std_v_vec_at_face_3d = m_grid_file->get_variable_3d_values(m_coordinate_type[2].toStdString());
                v_value = std_v_vec_at_face_3d.GetValueAtIndex(m_current_step, m_hydro_layer - 1, 0);
            }
        }
        if (u_value == nullptr || v_value == nullptr)
        {
            return;
        }

#if DO_TIMING == 1
        auto start = std::chrono::steady_clock::now();
#endif
        m_rgb_color.resize(mesh2d->face_nodes.size());
        m_ramph_vec_dir->update();  

        this->startDrawing(CACHE_2D);
        mCache_painter->setPen(Qt::NoPen);  // The bounding line of the polygon is not drawn
        double opacity = mCache_painter->opacity();
        mCache_painter->setOpacity(m_property->get_opacity());
        int nr_nodes_per_max_quad = 4;
        vector<double> vertex_x(nr_nodes_per_max_quad);
        vector<double> vertex_y(nr_nodes_per_max_quad);
        this->setPointSize(13);
        double direction = 0.0;
        double alpha;
        QColor col;
        for (int i = 0; i < mesh2d->face_nodes.size(); i++)
        {
            bool in_view = false;
            if (u_value[i] == missing_value) { continue; }  // *u_value[i] = 0.0;
            if (v_value[i] == missing_value) { continue; }  // *v_value[i] = 0.0;
            vertex_x.clear();
            vertex_y.clear();

            int p0, p1, p2, p3;
            p0 = mesh2d->face_nodes[i][0];
            p1 = mesh2d->face_nodes[i][1];
            p2 = mesh2d->face_nodes[i][2];
            p3 = mesh2d->face_nodes[i][3];
            in_view = false;
            if (mesh2d->node[0]->x[p0] > getMinVisibleX() && mesh2d->node[0]->x[p0] < getMaxVisibleX() &&
                mesh2d->node[0]->y[p0] > getMinVisibleY() && mesh2d->node[0]->y[p0] < getMaxVisibleY() ||
                mesh2d->node[0]->x[p1] > getMinVisibleX() && mesh2d->node[0]->x[p1] < getMaxVisibleX() &&
                mesh2d->node[0]->y[p1] > getMinVisibleY() && mesh2d->node[0]->y[p1] < getMaxVisibleY() ||
                mesh2d->node[0]->x[p2] > getMinVisibleX() && mesh2d->node[0]->x[p1] < getMaxVisibleX() &&
                mesh2d->node[0]->y[p2] > getMinVisibleY() && mesh2d->node[0]->y[p1] < getMaxVisibleY() ||
                mesh2d->node[0]->x[p3] > getMinVisibleX() && mesh2d->node[0]->x[p3] < getMaxVisibleX() &&
                mesh2d->node[0]->y[p3] > getMinVisibleY() && mesh2d->node[0]->y[p3] < getMaxVisibleY())
            {
                in_view = true; // whole face is inview, todo: could be tested on subcontrol volumes
            }

            if (in_view)
            {
                double edge_x_01 = 0.5 * (mesh2d->node[0]->x[p0] + mesh2d->node[0]->x[p1]);
                double edge_y_01 = 0.5 * (mesh2d->node[0]->y[p0] + mesh2d->node[0]->y[p1]);
                double edge_x_12 = 0.5 * (mesh2d->node[0]->x[p1] + mesh2d->node[0]->x[p2]);
                double edge_y_12 = 0.5 * (mesh2d->node[0]->y[p1] + mesh2d->node[0]->y[p2]);
                double edge_x_23 = 0.5 * (mesh2d->node[0]->x[p2] + mesh2d->node[0]->x[p3]);
                double edge_y_23 = 0.5 * (mesh2d->node[0]->y[p2] + mesh2d->node[0]->y[p3]);
                double edge_x_31 = 0.5 * (mesh2d->node[0]->x[p3] + mesh2d->node[0]->x[p0]);
                double edge_y_31 = 0.5 * (mesh2d->node[0]->y[p3] + mesh2d->node[0]->y[p0]);
                double centre_x = 0.;
                double centre_y = 0.;
                for (int j = 0; j < nr_nodes_per_max_quad; j++)
                {
                    p0 = mesh2d->face_nodes[i][j];
                    centre_x += mesh2d->node[0]->x[p0];
                    centre_y += mesh2d->node[0]->y[p0];
                }
                centre_x /= nr_nodes_per_max_quad;
                centre_y /= nr_nodes_per_max_quad;

                // determine the polygons [p, edge, centre, edge]
                vertex_x.clear();
                vertex_y.clear();
                p0 = mesh2d->face_nodes[i][0];
                vertex_x.push_back(mesh2d->node[0]->x[p0]);
                vertex_y.push_back(mesh2d->node[0]->y[p0]);
                vertex_x.push_back(edge_x_01);
                vertex_y.push_back(edge_y_01);
                vertex_x.push_back(centre_x);
                vertex_y.push_back(centre_y);
                vertex_x.push_back(edge_x_31);
                vertex_y.push_back(edge_y_31);
                // draw polygon
                direction = atan2(v_value[p0], u_value[p0]) * 360.0 / (2.0 * M_PI);
                col = m_ramph_vec_dir->getRgbFromValue(direction);
                alpha = min(col.alphaF(), m_property->get_opacity());
                mCache_painter->setOpacity(alpha);
                this->setFillColor(col);
                this->drawPolygon(vertex_x, vertex_y);


                // determine the polygons [p, edge, centre, edge]
                vertex_x.clear();
                vertex_y.clear();
                p0 = mesh2d->face_nodes[i][1];
                vertex_x.push_back(mesh2d->node[0]->x[p0]);
                vertex_y.push_back(mesh2d->node[0]->y[p0]);
                vertex_x.push_back(edge_x_12);
                vertex_y.push_back(edge_y_12);
                vertex_x.push_back(centre_x);
                vertex_y.push_back(centre_y);
                vertex_x.push_back(edge_x_01);
                vertex_y.push_back(edge_y_01);
                // draw polygon
                direction = atan2(v_value[p0], u_value[p0]) * 360.0 / (2.0 * M_PI);
                col = m_ramph_vec_dir->getRgbFromValue(direction);
                alpha = min(col.alphaF(), m_property->get_opacity());
                mCache_painter->setOpacity(alpha);
                this->setFillColor(col);
                this->drawPolygon(vertex_x, vertex_y);


                // determine the polygons [p, edge, centre, edge]
                vertex_x.clear();
                vertex_y.clear();
                p0 = mesh2d->face_nodes[i][2];
                vertex_x.push_back(mesh2d->node[0]->x[p0]);
                vertex_y.push_back(mesh2d->node[0]->y[p0]);
                vertex_x.push_back(edge_x_23);
                vertex_y.push_back(edge_y_23);
                vertex_x.push_back(centre_x);
                vertex_y.push_back(centre_y);
                vertex_x.push_back(edge_x_12);
                vertex_y.push_back(edge_y_12);
                // draw polygon
                direction = atan2(v_value[p0], u_value[p0]) * 360.0 / (2.0 * M_PI);
                col = m_ramph_vec_dir->getRgbFromValue(direction);
                alpha = min(col.alphaF(), m_property->get_opacity());
                mCache_painter->setOpacity(alpha);
                this->setFillColor(col);
                this->drawPolygon(vertex_x, vertex_y);


                // determine the polygons [p, edge, centre, edge]
                vertex_x.clear();
                vertex_y.clear();
                p0 = mesh2d->face_nodes[i][3];
                vertex_x.push_back(mesh2d->node[0]->x[p0]);
                vertex_y.push_back(mesh2d->node[0]->y[p0]);
                vertex_x.push_back(edge_x_31);
                vertex_y.push_back(edge_y_31);
                vertex_x.push_back(centre_x);
                vertex_y.push_back(centre_y);
                vertex_x.push_back(edge_x_23);
                vertex_y.push_back(edge_y_23);
                // draw polygon
                direction = atan2(v_value[p0], u_value[p0]) * 360.0 / (2.0 * M_PI);
                col = m_ramph_vec_dir->getRgbFromValue(direction);
                alpha = min(col.alphaF(), m_property->get_opacity());
                mCache_painter->setOpacity(alpha);
                this->setFillColor(col);
                this->drawPolygon(vertex_x, vertex_y);
            }
        }
        mCache_painter->setOpacity(opacity);
        this->finishDrawing();
    }
}
//-----------------------------------------------------------------------------
void MyCanvas::draw_dot_at_node()
{
    if (m_variable != nullptr && m_grid_file != nullptr && m_variable->location == "node")
    {
        string var_name = m_variable->var_name;

        struct _mesh1d* mesh1d = m_grid_file->get_mesh_1d();
        if (mesh1d != nullptr)
        {
            DataValuesProvider2D<double> std_data_at_node = m_grid_file->get_variable_values(var_name);
            if (std_data_at_node.m_numXY == 0)
            {
                return;
            }
            z_value = std_data_at_node.GetValueAtIndex(m_current_step, 0);

            double missing_value = m_variable->fill_value;
            m_rgb_color.resize(mesh1d->node[0]->x.size());
            determine_min_max(z_value, mesh1d->node[0]->x.size(), &m_z_min, &m_z_max, m_rgb_color, missing_value);

            this->startDrawing(CACHE_1D);
            double opacity = mCache_painter->opacity();
            mCache_painter->setOpacity(m_property->get_opacity());
            this->setPointSize(13);
            this->drawMultiDot(mesh1d->node[0]->x, mesh1d->node[0]->y, m_rgb_color);
            mCache_painter->setOpacity(opacity);
            this->finishDrawing();
        }
    }
}
//-----------------------------------------------------------------------------
void MyCanvas::draw_dot_at_edge()
{
    if (m_variable != nullptr && m_grid_file != nullptr && m_variable->location == "edge")
    {
        double x1, y1, x2, y2;
        vector<double> edge_x;
        vector<double> edge_y;
        string var_name = m_variable->var_name;
        struct _mesh1d * mesh1d = m_grid_file->get_mesh_1d();
        struct _edge * edges = mesh1d->edge[0];
        if (m_variable->dims.size() == 2) // 2D: time, nodes
        {
            DataValuesProvider2D<double>std_dot_at_edge = m_grid_file->get_variable_values(var_name);
            z_value = std_dot_at_edge.GetValueAtIndex(m_current_step, 0);
        }
        else if (m_variable->dims.size() == 3) // 3D: time, layer, nodes
        {
            DataValuesProvider3D<double> std_dot_at_edge_3d = m_grid_file->get_variable_3d_values(var_name);
            if (m_bed_layer > 0)
            {
                z_value = std_dot_at_edge_3d.GetValueAtIndex(m_current_step, m_bed_layer - 1, 0);
            }
            if (m_hydro_layer > 0)
            {
                z_value = std_dot_at_edge_3d.GetValueAtIndex(m_current_step, m_hydro_layer - 1, 0);
            }
        }
        else
        {
            QMessageBox::information(0, tr("MyCanvas::draw_dot_at_edge()"), QString("Program error on variable: \"%1\"\nUnsupported number of dimensions (i.e. > 3).").arg(var_name.c_str()));
        }
        if (z_value == nullptr)
        {
            return;
        }

        double missing_value = m_variable->fill_value;
        m_rgb_color.resize(edges->count);
        determine_min_max(z_value, edges->count, &m_z_min, &m_z_max, m_rgb_color, missing_value);

        for (int j = 0; j < edges->count; j++)
        {
            int p1 = edges->edge_nodes[j][0];
            int p2 = edges->edge_nodes[j][1];
            x1 = mesh1d->node[0]->x[p1];
            y1 = mesh1d->node[0]->y[p1];
            x2 = mesh1d->node[0]->x[p2];
            y2 = mesh1d->node[0]->y[p2];

            edge_x.push_back(0.5*(x1 + x2));
            edge_y.push_back(0.5*(y1 + y2));
        }

        this->startDrawing(CACHE_1D);
        double opacity = mCache_painter->opacity();
        mCache_painter->setOpacity(m_property->get_opacity());
        this->setPointSize(13);
        this->drawMultiDot(edge_x, edge_y, m_rgb_color);
        mCache_painter->setOpacity(opacity);
        this->finishDrawing();
    }
}
//-----------------------------------------------------------------------------
void MyCanvas::draw_line_at_edge()
{
    if (m_grid_file == nullptr) { return; }

    struct _mesh_variable* vars = m_grid_file->get_variables();
    struct _variable* var;
    for (int i = 0; i < vars->nr_vars; ++i)
    {
        var = vars->variable[i];
        // begin HACK edge vs contact
        if (var->draw && (var->location == "edge" || var->location == "contact"))
        {
            vector<double> edge_x(2);
            vector<double> edge_y(2);
            int length = 0;
            string var_name = var->var_name;
            if (var->dims.size() == 2) // 2D: time, nodes
            {
                DataValuesProvider2D<double>std_dot_at_edge = m_grid_file->get_variable_values(var_name);
                z_value = std_dot_at_edge.GetValueAtIndex(m_current_step, 0);
                length = std_dot_at_edge.m_numXY;
            }
            else if (var->dims.size() == 3) // 3D: time, layer, nodes
            {
                DataValuesProvider3D<double> std_dot_at_edge_3d = m_grid_file->get_variable_3d_values(var_name);
                if (var->sediment_index != -1)
                {
                    z_value = std_dot_at_edge_3d.GetValueAtIndex(m_current_step, var->sediment_index, 0);
                }
                else
                {
                    if (m_bed_layer > 0)
                    {
                        z_value = std_dot_at_edge_3d.GetValueAtIndex(m_current_step, m_bed_layer - 1, 0);
                    }
                    if (m_hydro_layer > 0)
                    {
                        z_value = std_dot_at_edge_3d.GetValueAtIndex(m_current_step, m_hydro_layer - 1, 0);
                    }
                }
                length = std_dot_at_edge_3d.m_numXY;
            }
            else
            {
                QMessageBox::information(0, tr("MyCanvas::draw_dot_at_edge()"), QString("Program error on variable: \"%1\"\nUnsupported number of dimensions (i.e. > 3).").arg(var_name.c_str()));
            }
            if (z_value == nullptr)
            {
                return;
            }
            double missing_value = var->fill_value;
            determine_min_max(z_value, length, &m_z_min, &m_z_max, missing_value);

            struct _edge * edges = nullptr;
            struct _mesh1d * mesh1d = nullptr;
            struct _mesh1d_string ** m1d = m_grid_file->get_mesh1d_string();
            if (m1d != nullptr && var->mesh == m1d[0]->var_name)
            {
                mesh1d = m_grid_file->get_mesh_1d();
                edges = mesh1d->edge[0];
            }

            struct _mesh2d * mesh2d = nullptr;
            struct _mesh2d_string ** m2d = m_grid_file->get_mesh2d_string();
            if (m2d != nullptr && var->mesh == m2d[0]->var_name)
            {
                mesh2d = m_grid_file->get_mesh_2d();
                edges = mesh2d->edge[0];
            }

            struct _mesh_contact * mesh1d2d = nullptr;
            struct _mesh_contact_string ** m1d2d = m_grid_file->get_mesh_contact_string();
            if(m1d2d != nullptr && var->mesh == m1d2d[0]->mesh_contact)
            {
                mesh1d2d = m_grid_file->get_mesh_contact();
                edges = mesh1d2d->edge[0];
            }

            m_rgb_color.resize(edges->count);
            this->startDrawing(CACHE_1D);
            double opacity = mCache_painter->opacity();
            mCache_painter->setOpacity(m_property->get_opacity());
            for (int j = 0; j < edges->count; j++)
            {
                int p1 = edges->edge_nodes[j][0];
                int p2 = edges->edge_nodes[j][1];
                if (mesh1d != nullptr)
                {
                    edge_x[0] = mesh1d->node[0]->x[p1];
                    edge_y[0] = mesh1d->node[0]->y[p1];
                    edge_x[1] = mesh1d->node[0]->x[p2];
                    edge_y[1] = mesh1d->node[0]->y[p2];
                }
                else if (mesh2d != nullptr)
                {
                    edge_x[0] = mesh2d->node[0]->x[p1];
                    edge_y[0] = mesh2d->node[0]->y[p1];
                    edge_x[1] = mesh2d->node[0]->x[p2];
                    edge_y[1] = mesh2d->node[0]->y[p2];
                }
                else if (mesh1d2d != nullptr)
                {
                    edge_x[0] = mesh1d2d->node[0]->x[p1];
                    edge_y[0] = mesh1d2d->node[0]->y[p1];
                    edge_x[1] = mesh1d2d->node[0]->x[p2];
                    edge_y[1] = mesh1d2d->node[0]->y[p2];
                }
                this->setLineColor(m_ramph->getRgbFromValue(z_value[j]));
                // 10 klasses: 2, 3, 4, 5, 6, 7, 8, 9 ,10, 11
                //double alpha = (*z_value[j] - m_z_min) / (m_z_max - m_z_min);
                //double width = (1. - alpha) * 1. + alpha * 11.;
                this->setPointSize(13);
                this->setLineWidth(9);
                this->drawPolyline(edge_x, edge_y);
            }
            mCache_painter->setOpacity(opacity);
            this->finishDrawing();
        }
    }
}

//-----------------------------------------------------------------------------
void MyCanvas::draw_data_along_edge()
{
    if (m_grid_file == nullptr) { return; }

    struct _mesh_variable* vars = m_grid_file->get_variables();
    struct _variable* var;
    for (int i = 0; i < vars->nr_vars; ++i)
    {
        var = vars->variable[i];
        if (var->draw && var->location == "node")
        {
            string var_name = var->var_name;
            struct _mesh1d* mesh1d = m_grid_file->get_mesh_1d();

            if (mesh1d != nullptr) 
            {
                DataValuesProvider2D<double>std_data_at_node = m_grid_file->get_variable_values(var_name);
                z_value = std_data_at_node.GetValueAtIndex(m_current_step, 0);
                int length = std_data_at_node.m_numXY;

                dims = var->dims;

                struct _edge* edges = mesh1d->edge[0];
                this->startDrawing(CACHE_1D);
                double opacity = mCache_painter->opacity();
                mCache_painter->setOpacity(m_property->get_opacity());
                this->setPointSize(13);
                vector<double> edge_x(2);
                vector<double> edge_y(2);
                vector<QColor> edge_color(2);

                double missing_value = var->fill_value;
                determine_min_max(z_value, length, &m_z_min, &m_z_max, missing_value);

                if (true)  // boolean to draw gradient along line?
                {
                    for (int j = 0; j < edges->count; j++)
                    {
                        int p1 = edges->edge_nodes[j][0];
                        int p2 = edges->edge_nodes[j][1];
                        edge_x[0] = mesh1d->node[0]->x[p1];
                        edge_y[0] = mesh1d->node[0]->y[p1];
                        edge_x[1] = mesh1d->node[0]->x[p2];
                        edge_y[1] = mesh1d->node[0]->y[p2];

                        edge_color[0] = m_ramph->getRgbFromValue(z_value[p1]);
                        edge_color[1] = m_ramph->getRgbFromValue(z_value[p2]);

                        this->drawLineGradient(edge_x, edge_y, edge_color);
                    }
                }
                if (false)  // boolean to draw multidot?
                {
                    m_rgb_color.resize(length);
                    for (int j = 0; j < length; j++)
                    {
                        m_rgb_color[j] = m_ramph->getRgbFromValue(z_value[j]);
                    }
                    this->drawMultiDot(mesh1d->node[0]->x, mesh1d->node[0]->y, m_rgb_color);
                }
                mCache_painter->setOpacity(opacity);
            }
        }
    }
}
void MyCanvas::setColorRamp(QColorRampEditor * ramph)
{
    m_ramph = ramph;
}
void MyCanvas::setColorRampVector(QColorRampEditor * ramph)
{
    m_ramph_vec_dir = ramph;
}

//-----------------------------------------------------------------------------
void MyCanvas::reset_min_max()
{
    m_z_min = std::numeric_limits<double>::infinity();
    m_z_max = -std::numeric_limits<double>::infinity();
}
//-----------------------------------------------------------------------------
void MyCanvas::set_coordinate_type(QStringList coord_type)
{
    m_coordinate_type = coord_type;
}
//-----------------------------------------------------------------------------
void MyCanvas::set_determine_grid_size(bool value)
{
    m_vscale_determined = !value;
}
//-----------------------------------------------------------------------------
void MyCanvas::set_current_step(int current_step)
{
    m_current_step = current_step;
}
//-----------------------------------------------------------------------------
void MyCanvas::set_variable(struct _variable * variable)
{
    m_variable = variable;
    this->reset_min_max();
}
//-----------------------------------------------------------------------------
void MyCanvas::set_variables(struct _mesh_variable * variables)
{
    m_variables = variables;
}
//-----------------------------------------------------------------------------
void MyCanvas::set_bed_layer(int i_layer)
{
    this->m_bed_layer = i_layer;
}
//-----------------------------------------------------------------------------
void MyCanvas::set_hydro_layer(int i_layer)
{
    this->m_hydro_layer = i_layer;
}
//-----------------------------------------------------------------------------
void MyCanvas::set_grid_file(GRID * grid_file)
{
    m_grid_file = grid_file;
}
//-----------------------------------------------------------------------------
void MyCanvas::determine_min_max(double * z, int length, double * z_min, double * z_max, vector<QColor> &rgb_color, double missing_value)
{
    if (m_property->get_dynamic_legend())
    {
        for (int i = 0; i < length; i++)
        {
            if (z[i] != missing_value)
            {
                *z_min = min(*z_min, z[i]);
                *z_max = max(*z_max, z[i]);
                rgb_color[i] = m_ramph->getRgbFromValue(z[i]);
            }
            else
            {
                rgb_color[i] = qRgba(255, 0, 0, 255);
            }
        }
        m_property->set_minimum(*z_min);
        m_property->set_maximum(*z_max);
    }
    else
    {
        *z_min = m_property->get_minimum();
        *z_max = m_property->get_maximum();
    }
    m_ramph->setMinMax(*z_min, *z_max);
    m_ramph->update();
}
void MyCanvas::determine_min_max(double * z, int length, double * z_min, double * z_max, double missing_value)
{
    if (m_property->get_dynamic_legend())
    {
        *z_min = INFINITY;
        *z_max = -INFINITY;
        for (int i = 0; i < length; i++)
        {
            if (z[i] != missing_value)
            {
                *z_min = min(*z_min, z[i]);
                *z_max = max(*z_max, z[i]);
            }
        }
        m_property->set_minimum(*z_min);
        m_property->set_maximum(*z_max);
    }
    else
    {
        *z_min = m_property->get_minimum();
        *z_max = m_property->get_maximum();
    }
    m_ramph->setMinMax(*z_min, *z_max);
    m_ramph->update();
}

double MyCanvas::statistics_averaged_length_of_cell(struct _variable * var)
{
    double avg_cell_area;
    double * area = var->data_2d.GetValueAtIndex(0, 0);
    int length = var->data_2d.m_numXY;

    avg_cell_area = 0.0;
    for (int i = 0; i < length; i++)
    {
        avg_cell_area += area[i];
    }
    avg_cell_area /= length;
    return std::sqrt(avg_cell_area);
}
//-----------------------------------------------------------------------------
void MyCanvas::empty_cache(drawing_cache cache_i)
{
    cacheArray[cache_i]->fill(Qt::transparent);

    mMapCanvas->update();  // todo, niet uitvoeren na openen van twee map-files
}
//-----------------------------------------------------------------------------
void MyCanvas::empty_caches()
{
    for (int j = 0; j<NR_CACHES; j++)
    {
        if (cacheArray[j] != NULL)
        {
            cacheArray[j]->fill(Qt::transparent);
        }
    }
    mMapCanvas->update();  // todo, niet uitvoeren na openen van twee map-files
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::paint( QPainter * p )
{
    //p->drawImage(0, 0, *cacheArray[NR_CACHES - 1]);
    for (int i = NR_CACHES - 1; i >= 0; i--) {
        p->drawImage(0, 0, *cacheArray[i]);
    }
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::startDrawing(int cache_i)
{
    bool clear_cache;

    if (printing) return;
    clear_cache = true;

    cache_i = max(0, min(cache_i, NR_CACHES - 1));

    if (drawing)
    {
        if (mCache_painter->isActive()) {
            mCache_painter->end(); // end painting on cacheArray[i]
        }
    }
    if (clear_cache) {
        cacheArray[cache_i]->fill(Qt::transparent);
    }
    mCache_painter->begin(cacheArray[cache_i]);
    drawCache = cache_i;
    drawing = true;

    if (DRAW_CACHES) {
        DrawEachCaches(); // debug utility
    }
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::finishDrawing()
{
    // QMessageBox::warning(0, "Message", QString(tr("MyCanvas::finishDrawing.")));
    if (mCache_painter->isActive()) {
        mCache_painter->end();
    }
    mMapCanvas->update();  // needed for initial drawing
    drawing = false;

    if (DRAW_CACHES) {
        DrawEachCaches(); // debug utility
    }
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::renderCompletePlugin(QPainter * Painter)
{
    //QMessageBox::warning(0, "Message", QString(tr("MyCanvas::renderCompletePlugin") ));
    renderPlugin( Painter );
}
void MyCanvas::renderPlugin( QPainter * Painter )
{
    // OK JanM QMessageBox::warning(0, "Message", QString(tr("MyCanvas::renderPlugin") ));
    // need width/height of paint device
    //int myWidth = Painter->device()->width();  //pixels
    //int myHeight = Painter->device()->height(); //pixels
    int width  = 500; //pixels
    int height = 250; //pixels

    this->qgis_painter = Painter;

    mMapCanvas->setMinimumWidth(width);
    mMapCanvas->setMinimumHeight(height);
    scale = mMapCanvas->scale();

    QRect frame = mMapCanvas->frameRect();
    frame_width = frame.width(); //total frame width in pixels
    frame_height = frame.height();  //total height width in pixels

    QgsRectangle extent = mMapCanvas->extent();
    window_width = extent.width();   //total frame width in world coordinates
    window_height = extent.height(); //total height width in world coordinates
    min_X = extent.xMinimum();
    min_Y = extent.yMinimum();

    pix_dx = window_width/frame_width;
    pix_dy = window_height/frame_height;

    this->qgis_painter->setViewport(qx(min_X), qy(min_Y), qx(window_width), qy(window_height));    

    if (listener != NULL)
    {
        listener->onAfterRedraw(false); // TODO: In onafterredraw zit ook een teken event
    }
    //draw_dot_at_edge();
    draw_data_at_face();
    draw_data_at_node();  // isofill of the control volume around node
    draw_dot_at_face();
    draw_dot_at_node();
    draw_data_along_edge();
    draw_line_at_edge();
    draw_vector_arrow();
    draw_vector_direction_at_face();
    draw_vector_direction_at_node();
    this->finishDrawing();
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::canvasDoubleClickEvent(QgsMapMouseEvent * me )
{
    emit MouseDoubleClickEvent( me );
}
void MyCanvas::canvasMoveEvent(QgsMapMouseEvent * me )
{
    //QMessageBox::warning(0, "Message", QString(tr("MyCanvas::canvasMoveEvent: %1").arg(me->button())));
    emit MouseMoveEvent( me );
}
void MyCanvas::canvasPressEvent(QgsMapMouseEvent * me )
{
    emit MousePressEvent( me );
}
void MyCanvas::canvasReleaseEvent(QgsMapMouseEvent * me )
{
    if (me->button() == Qt::RightButton)
    {
        QMessageBox::warning(0, "Message", QString(tr("MyCanvas::canvasReleaseEvent: %1").arg(me->button())));
        // get selected feature from selected layer
        // then view contex menu
        // launch the choose action
        QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
        QList <QgsLayerTreeGroup *> groups = treeRoot->findGroups();
        QgsMapLayer * active_layer = NULL;
            //active_layer = mQGisIface->activeLayer();
            if (active_layer != nullptr)
            {
                QString layer_id = active_layer->id();
                int a = -1;
            }
    }
    emit MyMouseReleaseEvent( me );
}
void MyCanvas::wheelEvent( QWheelEvent * we )
{
    //QMessageBox::warning(0, "Message", QString(tr("MyCanvas::wheelEvent")));
    this->empty_caches();
    mMapCanvas->update();
    emit WheelEvent( we );
}
//
//-----------------------------------------------------------------------------
//
// Create a new draw cache in the frontend and it gets the given index.
bool MyCanvas::newCache(int cacheNr)
{
    if ( cacheNr < 0  ||  cacheNr > NR_CACHES-1 )
    {
        return false;
    }

    if ( cacheArray[cacheNr] == NULL )
    {
        cacheArray[cacheNr] = new QImage(IMAGE_WIDTH, IMAGE_HEIGHT, QImage::Format_ARGB32_Premultiplied);
    }

    return true;
}
//
//-----------------------------------------------------------------------------
//
// Get the cache index of default available draw cache of the map frontend.
int MyCanvas::getBackgroundCache()
{
    return NR_CACHES-1; // Cache in which background should be copyied
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::copyCache(int sourceCacheIndex, int destCacheIndex)
{
    Q_UNUSED(sourceCacheIndex);
    Q_UNUSED(destCacheIndex);

	// No need for this function, still avalaible because of interface with plugins
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::drawDot(double x, double y)
{
    // Don't draw an edge
    QPen current_pen = mCache_painter->pen();
    mCache_painter->setPen( Qt::NoPen );
    QPoint center = QPoint(qx(x), qy(y));
    mCache_painter->drawEllipse( center, int(radius), int(radius) );
    mCache_painter->setPen( current_pen );
}

void MyCanvas::drawMultiDot(vector<double> xs , vector<double> ys , vector<QColor> rgb)
{
    int    i, j;
    size_t k;
    int    index;
    QPen   current_pen;
    double xMin, xMax, yMin, yMax;
    int    iMin, iMax, jMin, jMax;
    int    sizeI, sizeJ;
    double sx, sy;

    size_t nrPoints = xs.size();

    if (radius==0) {return;}
    if (radius < 4.0) { // i.e. 1, 2 and 3
        drawMultiPoint(xs , ys , rgb);
        return;
    }

    // NOTE that in this function is a check if the coordinates are outside the screen/display
    //
    // These are world coordinates
    //
    xMin = getMinVisibleX();
    xMax = getMaxVisibleX();
    yMin = getMinVisibleY();
    yMax = getMaxVisibleY();

    iMin = qx( xMin );
    iMax = qx( xMax );
    // function qy() mirrors the min and max!!!
    jMax = qy( yMin );
    jMin = qy( yMax );

    sizeI = iMax - iMin + 1;
    sizeJ = jMax - jMin + 1;

    int *pixelArray = new int [sizeI*sizeJ];

    // Initial value 0
    //
    memset(pixelArray, 0, (sizeI*sizeJ) * sizeof(int));

    // Don't draw an edge
    current_pen = mCache_painter->pen();
    mCache_painter->setPen( Qt::NoPen );


    // Loop for all point
    //
    sx = radius/scale; // scale x
    sy = radius/scale; // scale y

    for ( k = 0; k != nrPoints; k++ )
    {
        if (xs[k] < getMinVisibleX() || xs[k] > getMaxVisibleX() ||
            ys[k] < getMinVisibleY() || ys[k] > getMaxVisibleY())
        {
            continue; // point not in visible area, skip this vector
        }
        // Convert from world to pixel coordinates.
        // Functions qx() and qy() take into account that (0,0) is upper left corner
        //
        i = qx( xs[k]-sx );
        j = qy( ys[k]+sy );      //Y-flip

        if ( i >= 0 && i < sizeI && j >= 0 && j < sizeJ)
        {
            index = i + j*sizeI;
            if ( pixelArray[index] == 0 )
            {
                // Pixel not drawn yet.
                //
                mCache_painter->setBrush( QColor( rgb[k] ) );
                mCache_painter->drawEllipse( QPoint(i, j), int(radius), int(radius));
                pixelArray[index] = 1;
            }
        }
    }

    mCache_painter->setPen( current_pen );

    delete[] pixelArray;
}
//
//-----------------------------------------------------------------------------
//
// Draw a single point with currently set colour by routine fillColor()
void MyCanvas::drawPoint(double x, double y)
{
    QPen current_pen = mCache_painter->pen();
    mCache_painter->setPen(Qt::NoPen);
    QPoint center = QPoint(qx(x), qy(y));
    mCache_painter->drawEllipse(center, int(0.5*radius), int(0.5*radius));
    mCache_painter->setPen(current_pen);
}
//
//-----------------------------------------------------------------------------
//
// Draw an array of points according the given array of colours
void MyCanvas::drawMultiPoint(vector<double> xs, vector<double> ys, vector<QColor> rgb)
{
    int    i, j;
    size_t k;
    double xMin, xMax, yMin, yMax;
    int    iMin, iMax, jMin, jMax;
    int    sizeI, sizeJ;
    unsigned int * buffer_;
    unsigned int transparent;
    unsigned int colour;

    size_t nrPoints = xs.size();

    // NOTE that in this function is a check if the coordinates are outside the screen/display
    //
    // These are world coordinates
    //
    xMin = getMinVisibleX();
    xMax = getMaxVisibleX();
    yMin = getMinVisibleY();
    yMax = getMaxVisibleY();

    iMin = qx(xMin);
    iMax = qx(xMax);
    // function qy() mirrors the min and max!!!
    jMax = qy(yMin);
    jMin = qy(yMax);

    sizeI = iMax - iMin + 1;
    sizeJ = jMax - jMin + 1;

    buffer_ = new unsigned int[sizeI*sizeJ];
    memset(buffer_, 0, (sizeI*sizeJ) * sizeof(int));

    transparent = QColor(Qt::transparent).rgba();

    for (k = 0; k < sizeI*sizeJ; k++)
    {
        buffer_[k] = transparent;
    }

    // Loop for all points
    //
    if (radius < 2.0) { // radius == 1
        for (k = 0; k < nrPoints; k++)
        {
            i = qx(xs[k]);
            j = qy(ys[k]);
            if (i >= 0 + 1 && i < sizeI - 1 && j >= 0 + 1 && j < sizeJ - 1)
            {
                // Draw a 'plus' symbol on screen.
                colour = QColor(rgb[k]).rgba();
                buffer_[i - 1 + j   *sizeI] = colour;
                buffer_[i + j   *sizeI] = colour;
                buffer_[i + 1 + j   *sizeI] = colour;
                buffer_[i + (j - 1)*sizeI] = colour;
                buffer_[i + (j + 1)*sizeI] = colour;
            }
        }
    }
    else if (radius < 3.0) { // radius == 2
        for (k = 0; k < nrPoints; k++)
        {
            i = qx(xs[k]);
            j = qy(ys[k]);
            if (i >= 0 + 1 && i < sizeI - 1 && j >= 0 + 1 && j < sizeJ - 1)
            {
                // Draw a '2x2 square' symbol on screen.
                colour = QColor(rgb[k]).rgba();
                buffer_[i + j   *sizeI] = colour;
                buffer_[i + 1 + j   *sizeI] = colour;
                buffer_[i + (j + 1)*sizeI] = colour;
                buffer_[i + 1 + (j + 1)*sizeI] = colour;
            }
        }
    }
    else if (radius < 4.0) { // radius == 3
        for (k = 0; k < nrPoints; k++)
        {
            i = qx(xs[k]);
            j = qy(ys[k]);
            if (i >= 0 + 1 && i < sizeI - 1 && j >= 0 + 1 && j < sizeJ - 1)
            {
                // Draw a '3x3 square' symbol on screen.
                colour = QColor(rgb[k]).rgba();
                buffer_[i - 1 + (j - 1)*sizeI] = colour;
                buffer_[i + (j - 1)*sizeI] = colour;
                buffer_[i + 1 + (j - 1)*sizeI] = colour;
                buffer_[i - 1 + j   *sizeI] = colour;
                buffer_[i + j   *sizeI] = colour;
                buffer_[i + 1 + j   *sizeI] = colour;
                buffer_[i - 1 + (j + 1)*sizeI] = colour;
                buffer_[i + (j + 1)*sizeI] = colour;
                buffer_[i + 1 + (j + 1)*sizeI] = colour;
            }
        }
    }
    mCache_painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    QImage* qimage = new QImage((unsigned char*)buffer_, sizeI, sizeJ, QImage::Format_ARGB32_Premultiplied);
    mCache_painter->drawImage(0, 0, *qimage);
    delete[] buffer_;
    delete qimage;
}
//-----------------------------------------------------------------------------
//
// Draw a polygon. The polygon gets a color according the routine setFillColor
void MyCanvas::drawPolygon(vector<double> xs, vector<double> ys)
{
    QPolygon polygon;
    for (int i=0; i<xs.size(); i++)
    {
        polygon << QPoint(qx(xs[i]), qy(ys[i]));
    }
    mCache_painter->drawPolygon(polygon);
}
//
//-----------------------------------------------------------------------------
//
// Draw a polyline. This line width and line colour has to be set before this call
void MyCanvas::drawPolyline(vector<double> xs, vector<double> ys)
{
    QPolygon polyline;
    for (int i = 0; i < xs.size(); i++)
    {
        polyline << QPoint( qx(xs[i]),  qy(ys[i]));
    }
    mCache_painter->drawPolyline(polyline);
}
void MyCanvas::drawLineGradient(vector<double> xs, vector<double> ys, vector<QColor> rgb)
{
    assert(xs.size() == 2);
    QVector<QPair<QPoint, QColor>> point;
    point << qMakePair(QPoint(qx(xs[0]), qy(ys[0])), rgb[0]);
    point << qMakePair(QPoint(qx(xs[1]), qy(ys[1])), rgb[1]);

    QLinearGradient gradient;
    gradient.setColorAt(0, point[0].second);
    gradient.setColorAt(1, point[1].second);
    gradient.setStart(point[0].first);
    gradient.setFinalStop(point[1].first);
    mCache_painter->setPen(QPen(QBrush(gradient), 9));

    mCache_painter->drawLine(point[0].first, point[1].first);
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::drawRectangle(double x, double y, int width, int height)
{
    mCache_painter->drawRect( qx(x), qy(y), width, height);
}
//
//-----------------------------------------------------------------------------
//
// Draw the text. Textposition, font size and color has to bet set first by routines setFont.....
// and setTextAlignment()
void MyCanvas::drawText(double xleft, double ybottom, int width, int height, const char* text)
{
    Q_UNUSED(width);
    Q_UNUSED(height);

    QPen currentPen(mCache_painter->pen() );
    mCache_painter->setPen( textColour );

    mCache_painter->drawText( QPoint( qx(xleft), qy(ybottom) ), QString(text) );

    mCache_painter->setPen( currentPen );
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::setPointSize(int size)
{
    radius = double(size) /2.; // 2 is diameter/radius
    radius += 1.; // Empirical / correct for edge type "Qt::NoPen"
    if (size == 0) radius = 0.;
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::setLineWidth(int width)
{
    QPen currentPen(mCache_painter->pen() );
    currentPen.setWidth( width );
    currentPen.setCapStyle(Qt::FlatCap);
    currentPen.setJoinStyle(Qt::RoundJoin);
    mCache_painter->setPen( currentPen );
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::setLineColor(QColor rgb)
{
    QPen currentPen(mCache_painter->pen() );
    currentPen.setColor( rgb );
    currentPen.setCapStyle(Qt::FlatCap);
    currentPen.setJoinStyle(Qt::RoundJoin);
    mCache_painter->setPen( currentPen );
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::setFillColor(QColor rgb)
{
    //if (rgb == 0) {
    //    mCache_painter->setBrush(Qt::NoBrush );
    //} else 
    {
		QBrush * qbr =  new QBrush( QColor( rgb ) );
        mCache_painter->setBrush(*qbr);
    }
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::setFontName(const char* name)
{
    assert(mCache_painter); // True only inside a paintEvent

    // The mCache_painter gets a new font.
    mCache_painter->setFont( QFont(name) );
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::setFontColor(int rgb)
{
    textColour.setRgb( QRgb( rgb ) );
    textColour.setRgb( QRgb( rgb ) );
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::setFontPointSize(int size)
{
    QFont currentFont(mCache_painter->font() );
    currentFont.setPointSize( size );
    mCache_painter->setFont( currentFont );
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::setFontItalic(bool value)
{
    QFont currentFont(mCache_painter->font() );
    currentFont.setItalic( value );
    mCache_painter->setFont( currentFont );
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::setFontBold(bool value)
{
    Q_UNUSED(value);
    //=======om te testen met pixmap================
    //buffer0->load( QString("D:\\NL-Zuidwest.bmp") , "BMP" , 0 ) ;
    //buffer0->save( QString("D:\\QtPixmap.jpg") , "JPEG" ) ;

    //=====dit is de originele code
    //QFont newfont( mCache_painter->font() );
    //newfont.setBold( isBold );
    //mCache_painter->setFont( newfont );
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::setFontUnderline(bool value)
{
    QFont currentFont(mCache_painter->font() );
    currentFont.setUnderline( value );
    mCache_painter->setFont( currentFont );
}
//
//-----------------------------------------------------------------------------
//
bool MyCanvas::isFontAvailable(const char* name)
{
    Q_UNUSED(name);
    return (bool) true; //TODO implementation
}
//
//-----------------------------------------------------------------------------
//
int MyCanvas::getTextWidth(const char* name)
{
    int size = (mMapCanvas->fontMetrics()).horizontalAdvance(name);
    return size;
}
//
//-----------------------------------------------------------------------------
//
int MyCanvas::getTextHeight(const char* name)
{
    Q_UNUSED(name);
    int size = (mMapCanvas->fontMetrics()).height();
    return size;
}
//
//-----------------------------------------------------------------------------
//
double MyCanvas::getPixelWidth(double x, double y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    return pix_dx; // height of one pixel in world co-ordinates
}
//
//-----------------------------------------------------------------------------
//
double MyCanvas::getPixelHeight(double x, double y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    return pix_dy; // height of one pixel in world co-ordinates
}
//
//-----------------------------------------------------------------------------
//
double MyCanvas::getMinX()
{
   return min_X;
}
//
//-----------------------------------------------------------------------------
//
double MyCanvas::getMaxX()
{
   return max_X;
}
//
//-----------------------------------------------------------------------------
//
double MyCanvas::getMinY()
{
   return min_Y;
}
//
//-----------------------------------------------------------------------------
//
double MyCanvas::getMaxY()
{
   return max_Y;
}
//
//-----------------------------------------------------------------------------
//
double MyCanvas::getMinVisibleX()
{
    QgsRectangle extent = mMapCanvas->extent();
    return extent.xMinimum();
}
//
//-----------------------------------------------------------------------------
//
double MyCanvas::getMaxVisibleX()
{
    QgsRectangle extent = mMapCanvas->extent();
    return extent.xMaximum();
}
//
//-----------------------------------------------------------------------------
//
double MyCanvas::getMinVisibleY()
{
    QgsRectangle extent = mMapCanvas->extent();
    return extent.yMinimum();
}
//
//-----------------------------------------------------------------------------
//
double MyCanvas::getMaxVisibleY()
{
    QgsRectangle extent = mMapCanvas->extent();
    return extent.yMaximum();
}
//
//-----------------------------------------------------------------------------
//
int MyCanvas::getWidth()
{
    QRect frame = mMapCanvas->frameRect();
    return frame.width();
}
//
//-----------------------------------------------------------------------------
//
int MyCanvas::getHeight()
{
    QRect frame = mMapCanvas->frameRect();
    return frame.height();
}
//
//-----------------------------------------------------------------------------
//
int MyCanvas::getPixelXFromXY(double x, double y)
{
    Q_UNUSED(y);
    return qx((double) x);
}
//
//-----------------------------------------------------------------------------
//
int MyCanvas::getPixelYFromXY(double x, double y)
{
    Q_UNUSED(x);
    return qy((double) y);
}
//
//-----------------------------------------------------------------------------
//
double MyCanvas::getXFromPixel(int pixelX, int pixelY)
{
    Q_UNUSED(pixelY);
    return wx(pixelX);
}
//
//-----------------------------------------------------------------------------
//
double MyCanvas::getYFromPixel(int pixelX, int pixelY)
{
    Q_UNUSED(pixelX); 
    return wy(pixelY);
}
//
//-----------------------------------------------------------------------------
//
int MyCanvas::getPixelColor(double x, double y)
{
    return cacheArray[drawCache]->pixel((int)wx(x), (int)wy(y));
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::setFullExtend(double minX, double maxX, double minY, double maxY)
{
    Q_UNUSED(minX);
    Q_UNUSED(maxX);
    Q_UNUSED(minY);
    Q_UNUSED(maxY);
    // Full exent is set by the MFE (ie QGIS, ArcGIS, ...)
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::zoomToExtend(double minX, double maxX, double minY, double maxY)
{
    // Zoom to exent is done by the MFE (ie QGIS, ArcGIS, ...)
    Q_UNUSED(minX);
    Q_UNUSED(maxX);
    Q_UNUSED(minY);
    Q_UNUSED(maxY);
}

//
//-----------------------------------------------------------------------------
// ********* Events a widget must handle or forward to plugin **********
//-----------------------------------------------------------------------------
//
//

void MyCanvas::MyWheelEvent ( QWheelEvent * we )
{
    //QMessageBox::warning( 0, "Message", QString(tr("MyCanvas::MyWheelEvent")));
    Q_UNUSED(we);
    mMapCanvas->update();
}

//
//-----------------------------------------------------------------------------
//
void MyCanvas::MyMouseDoubleClickEvent( QMouseEvent * me)
{
    QMessageBox::warning( 0, "Message", QString(tr("MyCanvas::mouseDoubleClickEvent: %1").arg(me->button())));
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::MyMouseMoveEvent      ( QMouseEvent * me)
{
    if (listener != NULL)
    {
        //QMessageBox::warning(0, "Message", QString(tr("MyCanvas::MyMouseMoveEvent: %1").arg(me->button())));
        listener->onMouseMove(wx(me->x()), wy(me->y()), (AbstractCanvasListener::ButtonState) me->button() );
    }
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::MyMousePressEvent     ( QMouseEvent * me)
{
    if (listener != NULL)
    {
        listener->onMouseDown(wx(me->x()), wy(me->y()), (AbstractCanvasListener::ButtonState) me->button() );
    }
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::MyMouseReleaseEvent   (QgsMapMouseEvent * me)
{
    double length; 
    // afstand  bepaling
    //  crs = self.iface.mapCanvas().mapRenderer().destinationCrs()
    //distance_calc = QgsDistanceArea()
    //    distance_calc.setSourceCrs(crs)
    //    distance_calc.setEllipsoid(crs.ellipsoidAcronym())
    //    distance_calc.setEllipsoidalMode(crs.geographicFlag())
    //    distance = distance_calc.measureLine([self._startPt, endPt]) / 1000
    QgsPointXY p1 = QgsMapCanvasItem::toMapCoordinates(QPoint(me->x(), me->y()));
    QgsPointXY p2 = QgsMapCanvasItem::toMapCoordinates(QPoint(me->x()+50, me->y()+50));  // 
    length = 12345.0;
    QgsDistanceArea da;
    // QgsCoordinateReferenceSystem new_crs = layer[0]->layer()->crs();
    //da.setSourceCrs(new_crs);
    length = da.measureLine(p1, p2);
    QMessageBox::warning(0, "Message", QString("MyCanvas::MyMouseReleaseEvent\n(p1,p2): (%1, %2), (%3, %4), length: %5")
        .arg(QString::number(p1.x())).arg(QString::number(p1.y()))
        .arg(QString::number(p2.x())).arg(QString::number(p2.y()))
                .arg(QString::number(length))
    );
    if (listener != NULL)
    {
        listener->onMouseUp(wx(me->x()), wy(me->y()), (AbstractCanvasListener::ButtonState) me->button() );
    }
    mMapCanvas->update();
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::MyKeyPressEvent( QKeyEvent* ke)
{
    if (ke->modifiers() & Qt::ShiftModifier)
    {
        int pressed_key = ke->key() & Qt::ShiftModifier;
        {
            QMessageBox::warning(0, "Message", QString(tr("MyCanvas::MyKeyPressEvent: %1").arg(ke->key())));
        }
    }

    if (listener != NULL)
    {
		listener->onKeyDown((AbstractCanvasListener::KeyCode) ke->key(), (AbstractCanvasListener::KeyboardModifier) int(ke->modifiers()));
        ke->accept();
    }
}
//
//-----------------------------------------------------------------------------
//
void MyCanvas::MyKeyReleasedEvent( QKeyEvent * ke)
{
    if (listener != NULL)
    {
        listener->onKeyUp((AbstractCanvasListener::KeyCode) ke->key(), (AbstractCanvasListener::KeyboardModifier) int(ke->modifiers()));
        ke->accept();
    }
}

//
//=============================================================================
//


void MyCanvas::InitDrawEachCaches(void) // debug utility
{
    /* Draw each cache separately */
    vb = new QVBoxLayout(mMapCanvas);
    for (int i = 0; i<NR_CACHES; i++) {
        label[i] = new QLabel(mMapCanvas);
        label[i]->setScaledContents(true);
        newCache(i);
    }
    for (int i = 0; i<NR_CACHES; i++) {
        if (i == 0) cacheArray[i]->fill(Qt::yellow);
        if (i == 1) cacheArray[i]->fill(Qt::yellow);
        if (i == 2) cacheArray[i]->fill(Qt::yellow);
        if (i == 3) cacheArray[i]->fill(Qt::yellow);
        if (i == 4) cacheArray[i]->fill(Qt::yellow);
        if (i == 5) cacheArray[i]->fill(Qt::yellow);
        //if (i==6) cacheArray[i]->fill(Qt::green); // already set in function newcache
        label[i]->setPixmap(QPixmap::fromImage(*cacheArray[i])); // consider QGraphicsScene
        label[i]->setFrameStyle(QFrame::Panel);
        vb->addWidget(label[i]);
    }

    //  sprintf(title, "%s %d %s %d", "Source: ", sourceCacheIndex,"    Destination: ", destCacheIndex);
    w = new QWidget(mMapCanvas);
    w->setWindowTitle("Pixmaps");
    w->setLayout(vb);
    w->setMaximumWidth(250);
    w->setMaximumHeight(850);
    w->show();
}
//-----------------------------------------------------------------------------

void MyCanvas::DrawEachCaches(void) // debug utility
{
    // Draw each cache separately on the canvas
    for (int i = 0; i<NR_CACHES; i++)
    {
        vb->removeWidget(label[i]);
        label[i]->setPixmap(QPixmap::fromImage(*cacheArray[i])); // consider QGraphicsView
        vb->addWidget(label[i]);
    }
    w->setLayout(vb);
    w->update();
}
