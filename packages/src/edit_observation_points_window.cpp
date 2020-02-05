#include "edit_observation_points_window.h"
#define SLEEP_MACRO _sleep(0100)
//
//-----------------------------------------------------------------------------
//
int EditObsPoints::object_count = 0;


EditObsPoints::EditObsPoints(QgsMapLayer * obs_layer, QgsMapLayer * geom_layer, UGRID * ugrid_file, QgisInterface * QGisIface) :
    QDockWidget()
{
    object_count++;
    m_QGisIface = QGisIface;
    m_MyCanvas = new MyCanvas(QGisIface);
    QGisIface->mapCanvas();
    m_obs_layer = obs_layer;
    m_geom_layer = geom_layer;
    m_ugrid_files = ugrid_file;
    m_ntw_geom = ugrid_file->get_network_geometry();

    create_window(); //QMessageBox::information(0, "Information", "DockWindow::DockWindow()");

    QObject::connect(m_MyCanvas, &MyCanvas::MouseReleaseEvent, this, &EditObsPoints::MyMouseReleaseEvent);
    //QObject::connect(m_MyCanvas, &QComboBox::activated, this, &EditObsPoints::cb_clicked);
    m_QGisIface->mapCanvas()->setMapTool(m_MyCanvas);
}
//
//-----------------------------------------------------------------------------
//
EditObsPoints::~EditObsPoints()
{
    //QMessageBox::information(0, "Information", "EditObsPoints::~EditObsPoints()");
    // Only used when closing the application QGIS
}
void EditObsPoints::closeEvent(QCloseEvent * ce)
{
    //QMessageBox::information(0, "Information", "EditObsPoints::~closeEvent()");
    this->object_count--;
    m_QGisIface->mapCanvas()->unsetMapTool(m_MyCanvas);
    m_MyCanvas->set_variable(nullptr);
    m_MyCanvas->empty_caches();
}
int EditObsPoints::get_count()
{
    return object_count;
}

void EditObsPoints::create_window()
{
    //QFont serifFont("Times", 10, QFont::Bold);
    //this->setFont(serifFont);
    this->setWindowTitle(QString("Edit observation points"));
    QFrame * frame = new QFrame(this);
    QWidget * wid = new QWidget(frame);
    QVBoxLayout * vl = new QVBoxLayout();
    QHBoxLayout * hl = new QHBoxLayout();
    QGridLayout * gl = new QGridLayout();
    QAction * _show_p = new QAction();

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    //gl->addWidget(line);

    QLabel * lbl00 = new QLabel("Coordinate space: ");
    QLabel * lbl01 = new QLabel("---");;
    if (m_geom_layer != nullptr)
    {
        lbl01 = new QLabel(m_geom_layer->name());
    }
    QLabel * lbl10 = new QLabel("Observation points: ");
    QLabel * lbl11 = new QLabel(m_obs_layer->name());

    gl->addWidget(lbl00, 0, 0);
    gl->addWidget(lbl01, 0, 1);
    gl->addWidget(lbl10, 1, 0);
    gl->addWidget(lbl11, 1, 1);

    //int i = 0;
    //for (int j = 0; j < m_ntw_geom->geom[0]->count; j++)
    //{
    //    // TODO: if branch is visible then
    //    for (int k = 0; k < m_ntw_geom->geom[i]->nodes[j]->count; k++)
    //    {
    //        double x1 = m_ntw_geom->geom[i]->nodes[j]->x[k];
    //        double y1 = m_ntw_geom->geom[i]->nodes[j]->y[k];
    //    }
    //}
    //m_MyCanvas->

    //QObject::connect(cb, &QComboBox::activated, this, &EditObsPoints::cb_clicked);

    gl->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    wid->setLayout(gl);
    this->setWidget(wid);
    return;
}
void EditObsPoints::cb_clicked(int)
{
    int a = 1;;
}
void EditObsPoints::MyMouseReleaseEvent(QgsMapMouseEvent * me)
{
    double length;
    QgsPointXY p1 = QPoint(me->x(), me->y()); //QgsMapCanvasItem::toMapCoordinates(QPoint(me->x(), me->y()));
    QgsPointXY p2 = QPoint(0, 0); //QgsMapCanvasItem::toMapCoordinates(QPoint(me->x() + 20, me->y()));  // 
    length = 12345.0;
    QgsDistanceArea da;
    length = da.measureLine(p1, p2);
    QMessageBox::warning(0, "Message", QString("EditObsPoints::MyMouseReleaseEvent\n(x,y): (%1, %2), (%3, %4), length: %5")
        .arg(QString::number(p1.x())).arg(QString::number(p1.y()))
        .arg(QString::number(p2.x())).arg(QString::number(p2.y()))
        .arg(QString::number(length))
    );
}
