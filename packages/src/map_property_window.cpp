#include "map_property_window.h"

MapPropertyWindow::MapPropertyWindow(MyCanvas * myCanvas) : QDockWidget()
{
    //QMessageBox::information(0, "Information", "MapPropertyWindow::MapPropertyWindow()");
    m_property = MapProperty::getInstance();
    // make backup, needed when the cancel button is pressed
    m_bck_property = (struct _bck_property *) malloc(sizeof(struct _bck_property));
    m_bck_property->dynamic_legend = m_property->get_dynamic_legend();
    m_bck_property->opacity = m_property->get_opacity();
    m_bck_property->minimum = m_property->get_minimum();
    m_bck_property->maximum = m_property->get_maximum();
    m_bck_property->vector_scaling = m_property->get_vector_scaling();

    m_myCanvas = myCanvas;
    create_window();
}
MapPropertyWindow::~MapPropertyWindow()
{
}
void MapPropertyWindow::create_window()
{
    this->setWindowTitle(QString("Map properties"));
    QString str;
    wid = new QWidget();
    QVBoxLayout * vl_main = new QVBoxLayout();
    QVBoxLayout * vl = new QVBoxLayout();
    QHBoxLayout * hl = new QHBoxLayout();
    QGridLayout * gl = new QGridLayout();
    QGridLayout * gl_vs = new QGridLayout();

    QHBoxLayout * hl_transparency = new QHBoxLayout();
    lbl_transparency = new QLabel("Tansparency: ");
    le_transparency = new QLineEdit();
    le_transparency->setToolTip("Transparency range: 0.0 - 1.0");
    double tranparency = 1.0 - m_property->get_opacity();
    str = QString("%1").arg(tranparency);
    le_transparency->setText(str);

    hl_transparency->addWidget(lbl_transparency);
    hl_transparency->addWidget(le_transparency);
    vl_main->addLayout(hl_transparency);

    QGroupBox * gb = new QGroupBox();
    gb->setTitle("Colorramp limits");
    ckb = new QCheckBox();
    ckb->setCheckable(true);
    ckb->setChecked(m_property->get_dynamic_legend());
    ckb->setText("Dynamic colorramp");
    vl->addWidget(ckb);

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
    this->show();

    connect(ckb, &QCheckBox::stateChanged, this, &MapPropertyWindow::state_changed);
    connect(pb_apply, &QPushButton::clicked, this, &MapPropertyWindow::clicked_apply);
    connect(pb_ok, &QPushButton::clicked, this, &MapPropertyWindow::clicked_ok);
    connect(pb_cancel, &QPushButton::clicked, this, &MapPropertyWindow::clicked_cancel);

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
void MapPropertyWindow::clicked_apply()
{
    //QMessageBox::information(0, "Information", "MapPropertyWindow::clicked_apply()");
    double transparency = le_transparency->text().toDouble();
    double mod_transparency = std::max(0.0, std::min(transparency, 1.0));
    m_property->set_opacity(1.0 - mod_transparency);

    m_property->set_dynamic_legend(ckb->isChecked());
    m_property->set_minimum(le_min->text().toDouble());
    m_property->set_maximum(le_max->text().toDouble());
    m_property->set_vector_scaling(le_vs->text().toDouble());

    emit draw_all();
    return;
}
void MapPropertyWindow::clicked_ok()
{
    //QMessageBox::information(0, "Information", "MapPropertyWindow::released_ok()");
    double transparency = le_transparency->text().toDouble();
    double mod_transparency = std::max(0.0, std::min(transparency, 1.0));
    m_property->set_opacity(1.0 - mod_transparency);

    m_property->set_dynamic_legend(ckb->isChecked());
    m_property->set_minimum(le_min->text().toDouble());
    m_property->set_maximum(le_max->text().toDouble());
    m_property->set_vector_scaling(le_vs->text().toDouble());

    emit draw_all();
    delete m_bck_property;
    wid->close();
    return;
}
void MapPropertyWindow::clicked_cancel()
{
     m_property->set_dynamic_legend(m_bck_property->dynamic_legend);
     m_property->set_opacity(m_bck_property->opacity);
     m_property->set_minimum(m_bck_property->minimum);
     m_property->set_maximum(m_bck_property->maximum);
     m_property->set_vector_scaling(m_bck_property->vector_scaling);

     emit draw_all();
     delete m_bck_property;
     wid->close();
     return;
}
