#include "map_property_window.h"

int MapPropertyWindow::object_count = 0;

MapPropertyWindow::MapPropertyWindow(MyCanvas * myCanvas) : QDockWidget()
{
    //QMessageBox::information(0, "Information", "MapPropertyWindow::MapPropertyWindow()");
    object_count = 1;
    m_property = MapProperty::getInstance();
    // make backup, needed when the cancel button is pressed
    m_bck_property = (struct _bck_property *) malloc(sizeof(struct _bck_property));
    m_bck_property->dynamic_legend = m_property->get_dynamic_legend();
    m_bck_property->opacity = m_property->get_opacity();
    m_bck_property->refresh_time = m_property->get_refresh_time();
    m_bck_property->minimum = m_property->get_minimum();
    m_bck_property->maximum = m_property->get_maximum();
    m_bck_property->vector_scaling = m_property->get_vector_scaling();

    m_myCanvas = myCanvas;
    create_window();
}
MapPropertyWindow::~MapPropertyWindow()
{
    //QMessageBox::information(0, "Information", "MapPropertyWindow::~MapPropertyWindow()");
}
void MapPropertyWindow::closeEvent(QCloseEvent * event)
{
    //QMessageBox::information(0, "Information", "MapPropertyWindow::closeEvent()");
    this->object_count = 0;
}
void MapPropertyWindow::close()
{
    //QMessageBox::information(0, "Information", "MapPropertyWindow::~close()");
    this->object_count = 0;
    wid->close();
}
int MapPropertyWindow::get_count()
{
    return object_count;
}
void MapPropertyWindow::create_window()
{
    this->setWindowTitle(QString("Map Output Settings"));
    this->setGeometry(50, 300, 300, 0);
    QString str;
    wid = new QWidget();
    QVBoxLayout * vl_main = new QVBoxLayout();
    QVBoxLayout * vl = new QVBoxLayout();
    QHBoxLayout * hl = new QHBoxLayout();
    QGridLayout* gl = new QGridLayout();
    QGridLayout * gl_vs = new QGridLayout();

    // Refresh time
    lbl_refresh_time = new QLabel("Refresh time: ");
    le_refresh_time = new QLineEdit();
    le_refresh_time->setToolTip("Minimum refresh time of images");
    str = QString("%1").arg(m_property->get_refresh_time());
    le_refresh_time->setText(str);

    QGridLayout* gl_rt = new QGridLayout();  // refresh time
    gl_rt->addWidget(lbl_refresh_time, 0, 0);
    gl_rt->addWidget(le_refresh_time, 0, 1);
    vl_main->addLayout(gl_rt);

    // Transparency (Opcatity)
    QGroupBox* gb_op = new QGroupBox();
    gb_op->setTitle("Transparency");

    QGridLayout* gl_op = new QGridLayout();  // opacity
    QVBoxLayout* vl_op = new QVBoxLayout();  // opacity
    lbl_transparency = new QLabel("Transparency: ");
    le_transparency = new QLineEdit();
    le_transparency->setToolTip("Transparency range: 0 - 100 %");
    double transparency = 1.0 - m_property->get_opacity();
    str = QString("%1").arg(100.*transparency);
    le_transparency->setText(str);
    gl_op->addWidget(lbl_transparency, 0, 0);
    gl_op->addWidget(le_transparency, 0, 1);
    vl_op->addLayout(gl_op);

    sl_transparency = new QSlider(Qt::Horizontal);
    sl_transparency->setMinimum(0.0);
    sl_transparency->setMaximum(100.0);
    sl_transparency->setValue(transparency * 100.0);
    sl_transparency->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    vl_op->addWidget(sl_transparency);
    gb_op->setLayout(vl_op);
    vl_main->addWidget(gb_op);

    // Color limits
    QGroupBox * gb = new QGroupBox();
    gb->setTitle("Colorramp limits");
    m_ckb = new QCheckBox();
    m_ckb->setCheckable(true);
    m_ckb->setEnabled(true);
    m_ckb->setChecked(m_property->get_dynamic_legend());
    m_ckb->setText("Dynamic colorramp");
    vl->addWidget(m_ckb);

    lbl_min = new QLabel("Minimum value:");
    lbl_max = new QLabel("Maximum value:");
    le_min = new QLineEdit();
    le_max = new QLineEdit();
    str = QString("%1").arg(m_property->get_minimum());
    le_min->setText(str);
    str = QString("%1").arg(m_property->get_maximum());
    le_max->setText(str);

    gl->addWidget(lbl_min, 0, 0);
    gl->addWidget(lbl_max, 1, 0);
    gl->addWidget(le_min, 0, 1);
    gl->addWidget(le_max, 1, 1);
    vl->addLayout(gl);

    // Vector scaling
    QGroupBox * gb_vs = new QGroupBox();  // vector scaling
    gb_vs->setTitle("Vector scaling");
    lbl_vs = new QLabel("Vector scaling:");
    le_vs = new QLineEdit();
    str = QString("%1").arg(m_property->get_vector_scaling());
    le_vs->setText(str);
    gl_vs->addWidget(lbl_vs, 0, 0);
    gl_vs->addWidget(le_vs, 0, 1);

    QPushButton * pb_apply = new QPushButton("Apply");
    QPushButton * pb_ok = new QPushButton("OK");
    QPushButton * pb_cancel = new QPushButton("Cancel");

    hl->addWidget(pb_ok);
    hl->addWidget(pb_cancel);
    hl->addWidget(pb_apply);

    gb->setLayout(vl);
    gb_vs->setLayout(gl_vs);

    vl_main->addWidget(gb);  // scalar
    vl_main->addWidget(gb_vs);  // vector scaling

    vl_main->addLayout(hl);  // push buttons
    wid->setLayout(vl_main);

    // Enable/Disable items if the items are already defined
    if (m_property->get_dynamic_legend())
    {
        state_changed(Qt::Checked);
    }
    else
    {
        state_changed(Qt::Unchecked);
    }
    this->setWidget(wid);
    this->setWindowFlags(Qt::WindowStaysOnTopHint);
    this->show();

    connect(this, &MapPropertyWindow::close_map, this, &MapPropertyWindow::close);
    connect(m_ckb, &QCheckBox::stateChanged, this, &MapPropertyWindow::state_changed);
    connect(pb_apply, &QPushButton::clicked, this, &MapPropertyWindow::clicked_apply);
    connect(pb_ok, &QPushButton::clicked, this, &MapPropertyWindow::clicked_ok);
    connect(pb_cancel, &QPushButton::clicked, this, &MapPropertyWindow::clicked_cancel);

    connect(le_transparency, &QLineEdit::textChanged, this, &MapPropertyWindow::clicked_apply);
    connect(le_transparency, &QLineEdit::textChanged, this, &MapPropertyWindow::setOpacitySliderValue);
    connect(sl_transparency, &QAbstractSlider::valueChanged, this, &MapPropertyWindow::setOpacityEditValue);
    connect(le_refresh_time, &QLineEdit::returnPressed, this, &MapPropertyWindow::clicked_apply);
    connect(le_min, &QLineEdit::returnPressed, this, &MapPropertyWindow::clicked_apply);
    connect(le_max, &QLineEdit::returnPressed, this, &MapPropertyWindow::clicked_apply);
    connect(le_vs, &QLineEdit::returnPressed, this, &MapPropertyWindow::clicked_apply);

    connect(this, &MapPropertyWindow::draw_all, m_myCanvas, &MyCanvas::draw_all);
}
void MapPropertyWindow::state_changed(int state)
{
    if (state == Qt::Unchecked)
    {
        lbl_min->setEnabled(true);
        le_min->setEnabled(true);
        lbl_max->setEnabled(true);
        le_max->setEnabled(true);
    }
    else if (state == Qt::Checked)
    {
        lbl_min->setEnabled(false);
        le_min->setEnabled(false);
        lbl_max->setEnabled(false);
        le_max->setEnabled(false);
    }
    //  not supported: Qt::PartiallyChecked
    return;
}
void MapPropertyWindow::set_dynamic_limits_enabled(bool enabled)
{
    if (object_count != 0)
    {
        m_ckb->setEnabled(enabled);
    }
}
void MapPropertyWindow::clicked_apply()
{
    //QMessageBox::information(0, "Information", "MapPropertyWindow::clicked_apply()");
    double transparency = le_transparency->text().toDouble()/100.0;
    double mod_transparency = std::max(0.0, std::min(transparency, 1.0));
    m_property->set_opacity(1.0 - mod_transparency);
    m_property->set_refresh_time(le_refresh_time->text().toDouble());

    m_property->set_dynamic_legend(m_ckb->isChecked());
    m_property->set_minimum(le_min->text().toDouble());
    m_property->set_maximum(le_max->text().toDouble());
    m_property->set_vector_scaling(le_vs->text().toDouble());

    emit draw_all();
    return;
}
void MapPropertyWindow::setOpacitySliderValue(QString text)
{
    double transparency = le_transparency->text().toDouble()/100.;
    double mod_transparency = std::max(0.0, std::min(transparency, 1.0));
    m_property->set_opacity(1.0 - mod_transparency);
    
    sl_transparency->setValue(transparency * 100.0);
    return;
}
void MapPropertyWindow::setOpacityEditValue(int value)
{
    double fraction = double(value) / 100.;
    m_property->set_opacity(1.0 - fraction);
    QString str = QString("%1").arg(value);
    le_transparency->setText(str);
    return;
}
void MapPropertyWindow::clicked_ok()
{
    this->clicked_apply();

    delete m_bck_property;
    emit draw_all();
    emit close_map();
    return;
}
void MapPropertyWindow::clicked_cancel()
{
     m_property->set_dynamic_legend(m_bck_property->dynamic_legend);
     m_property->set_opacity(m_bck_property->opacity);
     m_property->set_refresh_time(m_bck_property->refresh_time);
     m_property->set_minimum(m_bck_property->minimum);
     m_property->set_maximum(m_bck_property->maximum);
     m_property->set_vector_scaling(m_bck_property->vector_scaling);

     delete m_bck_property;
     emit draw_all();
     emit close_map();
     return;
}


