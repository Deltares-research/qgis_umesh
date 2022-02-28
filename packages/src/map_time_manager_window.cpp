#include "map_time_manager_window.h"
//
//-----------------------------------------------------------------------------
//
int MapTimeManagerWindow::object_count = 0;
MapProperty * MapProperty::obj;  // Initialize static member of class MapProperty (Singleton)

MapTimeManagerWindow::MapTimeManagerWindow(QgisInterface * QGisIface, UGRIDAPI_WRAPPER* ugrid_file, MyCanvas * MyCanvas) : QDockWidget()
{
    object_count = 1;
    m_QGisIface = QGisIface;
    m_sb_bed_layer = nullptr;
    m_sb_bed_layer_vec = nullptr;
    m_sb_hydro_layer = nullptr;
    m_sb_hydro_layer_vec = nullptr;
    m_ramph_vec_dir = nullptr;
    m_map_property_window = nullptr;
    m_cur_view = nullptr;
    m_ugrid_file = ugrid_file;
    _MyCanvas = MyCanvas;
    _MyCanvas->setUgridFile(m_ugrid_file);
    _q_times = m_ugrid_file->get_qdt_times();
    m_vars_1d = m_ugrid_file->get_variables_1d();  // before create window
    m_vars_1d2d = m_ugrid_file->get_variables_1d2d();  // before create window
    m_vars_2d = m_ugrid_file->get_variables_2d();  // before create window
    m_vars = m_ugrid_file->get_variables();  // before create window
    create_window(); //QMessageBox::information(0, "Information", "DockWindow::DockWindow()");
    _MyCanvas->empty_caches();
    _current_step = 0;

    m_show_map_data_1d = false;  // releated to checkbox MapTimeManagerWindow::show_parameter_1d
    m_show_map_data_1d2d = false;  // releated to checkbox MapTimeManagerWindow::show_parameter_1d2d
    m_show_map_data_2d = false;  // releated to checkbox MapTimeManagerWindow::show_parameter_2d
    m_show_map_data_3d = false;  // releated to checkbox MapTimeManagerWindow::show_parameter_3d
    m_show_map_vector_2d = false;  // releated to checkbox MapTimeManagerWindow::show_parameter_vec_2d
    m_show_map_vector_3d = false;  // releated to checkbox MapTimeManagerWindow::show_parameter_vec_2d

    m_property = MapProperty::getInstance();
    m_vector_draw = VECTOR_NONE;

    connect(m_ramph, &QColorRampEditor::rampChanged, this, &MapTimeManagerWindow::ramp_changed);
    if (m_ramph_vec_dir != nullptr)
    {
        connect(m_ramph_vec_dir, &QColorRampEditor::rampChanged, this, &MapTimeManagerWindow::ramp_changed);
    }
}
MapTimeManagerWindow::~MapTimeManagerWindow()
{
    // Not reached because the window is cleaned by function: MapTimeManagerWindow::closeEvent
}
void MapTimeManagerWindow::MyMouseReleaseEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    QMessageBox::information(0, "Information", "MapTimeManagerWindow::MyMouseReleaseEvent()");
}
void MapTimeManagerWindow::ramp_changed()
{
    //QMessageBox::information(0, "Information", "MapTimeManagerWindow::ramp_changed()");
    _MyCanvas->draw_all();
}

void MapTimeManagerWindow::contextMenu(const QPoint & point)
{
    Q_UNUSED(point);
    //QMessageBox::information(0, "Information", "MapTimeManagerWindow::contextMenu()");
    if (MapPropertyWindow::get_count() == 0)  // create a window if it is not already there.
    {
        m_map_property_window = new MapPropertyWindow(_MyCanvas);
    }
    m_map_property_window->set_dynamic_limits_enabled(true);
    if (m_vector_draw == VECTOR_DIRECTION)
    {
        // set ramp limits en/disabled
        m_map_property_window->set_dynamic_limits_enabled(false);
    }
}
//
//-----------------------------------------------------------------------------
//
void MapTimeManagerWindow::closeEvent(QCloseEvent * ce)
{
    Q_UNUSED(ce);
    //QMessageBox::information(0, "Information", "MapTimeManagerWindow::~closeEvent()");
    this->object_count = 0;
    stop_time_loop = true;
    QStringList coord;
    coord << "";
    _MyCanvas->set_coordinate_type(coord);
    _MyCanvas->set_determine_grid_size(true);
    _MyCanvas->empty_caches();
    for (int i = 0; i < m_vars.size(); ++i)
    {
        m_vars[i]->draw = false;
    }
    this->goto_begin();  // goto to begin of time loop
    if (m_cur_view != nullptr)
    {
        m_cur_view->close();
        m_cur_view = nullptr;
    }
    if (m_map_property_window != nullptr)
    {
        m_map_property_window->close();
        m_map_property_window = nullptr;
    }
}
int MapTimeManagerWindow::get_count()
{
    return object_count;
}

void MapTimeManagerWindow::create_window()
{
#include "vsi.xpm"
    this->setWindowTitle(QString("Map Output Animation"));

    QWidget * wid = new QWidget();
    QVBoxLayout * vl = new QVBoxLayout();

    QGridLayout * gl = create_date_time_layout();
    vl->addLayout(gl);

    // Animation 

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    vl->addWidget(line);

    QHBoxLayout * hl1 = create_push_buttons_layout_animation();  // animation
    vl->addLayout(hl1);

    QHBoxLayout * hl2 = create_push_buttons_layout_steps();  // set steps
    vl->addLayout(hl2);

    m_slider = create_time_slider();
    vl->addWidget(m_slider);

    QFrame* line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    vl->addWidget(line2);

    // Selection of parameter and/or vector 

    QTabWidget * tw = new QTabWidget();
    QWidget * iso_patch = new QWidget();
    QWidget * vectors = new QWidget();
    QIcon icon_iso_patch;
    QIcon icon_vectors;

    QString program_files = QProcessEnvironment::systemEnvironment().value("ProgramFiles", "");
    program_files = program_files + QString("/deltares/qgis_umesh");
    QDir program_files_dir = QDir(program_files);
    icon_iso_patch = get_icon_file(program_files_dir, "/icons/iso_patch.png");
    icon_vectors = get_icon_file(program_files_dir, "/icons/vectors.png");

    iso_patch->setToolTip("Patches");
    iso_patch->setStatusTip("Draw patches with colour");
    QVBoxLayout * vl_tw_iso = create_scalar_selection_1d_2d_3d();
    iso_patch->setLayout(vl_tw_iso);
    tw->addTab(iso_patch, "Scalar");

    vectors->setToolTip("Velocity vector");
    vectors->setStatusTip("Draw horizontal velocity vector");
    QVBoxLayout * vl_tw_vec = create_vector_selection_2d_3d();
    if (vl_tw_vec != nullptr)
    {
        vectors->setLayout(vl_tw_vec);
        tw->addTab(vectors, "Vector");
    }
    vl->addWidget(tw);
    vl->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    m_pb_cur_view = new QPushButton(QString("Add current view to QGIS panel"));
    m_pb_cur_view->setEnabled(false);
    m_pb_cur_view->setStatusTip(QString("Add current view as vector layer to the QGIS \'Layers\' panel"));
    connect(m_pb_cur_view, &QPushButton::clicked, this, &MapTimeManagerWindow::clicked_current_view);

    vl->addWidget(m_pb_cur_view);

    wid->setLayout(vl);
    this->setWidget(wid);
    return;
}
QGridLayout * MapTimeManagerWindow::create_date_time_layout()
{
    _q_times = m_ugrid_file->get_qdt_times();
    nr_times = _q_times.size();
    first_date_time_indx = 0;
    last_date_time_indx = nr_times - 1;

    QGridLayout * gl = new QGridLayout();
    QLabel * lbl_start_time = new QLabel(QString("First time of animation"));
    QLabel * lbl_curr_time = new QLabel(QString("Current time (presented)"));
    QLabel * lbl_stop_time = new QLabel(QString("Last time of animation"));

    first_date_time = new MyQDateTimeEdit(_q_times, 0);
    curr_date_time = new MyQDateTimeEdit(_q_times, 0);
    last_date_time = new MyQDateTimeEdit(_q_times, _q_times.size()-1);
    QString format_date_time = QString("yyyy-MM-dd HH:mm:ss");

    // TODO set minimum and maximum within range first datetime and last datetime
    first_date_time->setTimeSpec(Qt::UTC);
    first_date_time->setToolTip(QString("Time frame UTC"));
    first_date_time->setDateTime(_q_times[0]);
    first_date_time->setDisplayFormat(format_date_time);
    first_date_time->setWrapping(true);
    
    curr_date_time->setTimeSpec(Qt::UTC);
    curr_date_time->setToolTip(QString("Time frame UTC"));
    curr_date_time->setDateTime(_q_times[0]);
    curr_date_time->setDisplayFormat(format_date_time);
    curr_date_time->setWrapping(true);
    
    last_date_time->setTimeSpec(Qt::UTC);
    last_date_time->setToolTip(QString("Time frame UTC"));
    last_date_time->setDateTime(_q_times[_q_times.size() - 1]);
    last_date_time->setDisplayFormat(format_date_time);
    last_date_time->setWrapping(true);

    connect(first_date_time, SIGNAL(dateTimeChanged(const QDateTime &)), this, SLOT(first_date_time_changed(const QDateTime &)));
    connect(curr_date_time, SIGNAL(dateTimeChanged(const QDateTime &)), this, SLOT(curr_date_time_changed(const QDateTime &)));
    connect(last_date_time, SIGNAL(dateTimeChanged(const QDateTime &)), this, SLOT(last_date_time_changed(const QDateTime &)));

    gl->addWidget(lbl_start_time, 0, 0);
    gl->addWidget(lbl_curr_time, 1, 0);
    gl->addWidget(lbl_stop_time, 2, 0);

    gl->addWidget(first_date_time, 0, 1);
    gl->addWidget(curr_date_time, 1, 1);
    gl->addWidget(last_date_time, 2, 1);

    return gl;
}
QHBoxLayout * MapTimeManagerWindow::create_push_buttons_layout_animation()
{
    QHBoxLayout * hl = new QHBoxLayout();

    QButtonGroup * button_group = new QButtonGroup();
    button_group->setExclusive(true);
    connect(button_group, SIGNAL(released()), this, SLOT(button_pressed(int)));

    pb_reverse = new QPushButton();
    pb_pauze = new QPushButton();
    pb_start = new QPushButton();
    button_group->addButton(pb_reverse);
    button_group->addButton(pb_start);
    button_group->addButton(pb_pauze);

    connect(pb_reverse, &QPushButton::released, this, &MapTimeManagerWindow::start_reverse);
    connect(pb_pauze, &QPushButton::released, this, &MapTimeManagerWindow::pause_time_loop);
    connect(pb_start, &QPushButton::released, this, &MapTimeManagerWindow::start_forward);

    pb_reverse->setCheckable(true);
    pb_reverse->setToolTip(QString("Start Reverse animation"));
    pb_pauze->setCheckable(true);
    pb_pauze->setToolTip(QString("Stop animation"));
    pb_start->setCheckable(true);
    pb_start->setToolTip(QString("Start Forward animation"));

    pb_reverse->setText(QString("<-")); // -> setIcon(*icon);
    pb_pauze->setText(QString("||"));  // ->setIcon(*icon);
    pb_start->setText(QString("->"));  // ->setIcon(*icon);

    hl->addWidget(pb_reverse);
    hl->addWidget(pb_pauze);
    hl->addWidget(pb_start);

    return hl;
}
QHBoxLayout * MapTimeManagerWindow::create_push_buttons_layout_steps()
{
    QHBoxLayout * hl = new QHBoxLayout();

    QPushButton * pb_begin = new QPushButton();
    QPushButton * pb_back = new QPushButton();
    QPushButton * pb_step = new QPushButton();
    QPushButton * pb_end = new QPushButton();

    pb_begin->setCheckable(false);
    pb_begin->setToolTip(QString("Go to start of animation"));
    pb_back->setCheckable(false);
    pb_back->setToolTip(QString("One time step back"));
    pb_step->setCheckable(false);
    pb_step->setToolTip(QString("One time step forward"));
    pb_end->setCheckable(false);
    pb_end->setToolTip(QString("Go to end of animation"));

    pb_begin->setText(QString("B"));  // ->setIcon(*icon);
    pb_back->setText(QString("-1"));  // ->setIcon(*icon);
    pb_step->setText(QString("+1"));  // ->setIcon(*icon);
    pb_end->setText(QString("E"));  // ->setIcon(*icon);

    pb_begin->setMinimumWidth(5);
    pb_back->setMinimumWidth(5);
    pb_step->setMinimumWidth(5);
    pb_end->setMinimumWidth(5);

    connect(pb_begin, SIGNAL(released()), this, SLOT(goto_begin()));
    connect(pb_back, SIGNAL(released()), this, SLOT(one_step_backward()));
    connect(pb_step, SIGNAL(released()), this, SLOT(one_step_forward()));
    connect(pb_end, SIGNAL(released()), this, SLOT(goto_end()));
    
    hl->addWidget(pb_begin);
    hl->addWidget(pb_back);
    hl->addWidget(pb_step);
    hl->addWidget(pb_end);
    
    return hl;
}
QColorRampEditor * MapTimeManagerWindow::create_color_ramp(vector_quantity vector_draw)
{
    QColorRampEditor* ramph = new QColorRampEditor(NULL, Qt::Horizontal);
    m_vector_draw = vector_draw;
    if (vector_draw == VECTOR_NONE)
    {
        QVector<QPair<qreal, QColor> > initramp;
        initramp.push_back(QPair<qreal, QColor>(0.00, QColor(0, 0, 128)));
        initramp.push_back(QPair<qreal, QColor>(0.125, QColor(0, 0, 255)));
        initramp.push_back(QPair<qreal, QColor>(0.375, QColor(0, 255, 255)));
        initramp.push_back(QPair<qreal, QColor>(0.50, QColor(0, 255, 0)));
        initramp.push_back(QPair<qreal, QColor>(0.625, QColor(255, 255, 0)));
        initramp.push_back(QPair<qreal, QColor>(0.875, QColor(255, 0, 0)));
        initramp.push_back(QPair<qreal, QColor>(1.00, QColor(128, 0, 0)));

        ramph->setSlideUpdate(true);
        ramph->setMappingTextVisualize(true);
        ramph->setMappingTextColor(Qt::black);
        ramph->setMappingTextAccuracy(2);
        ramph->setNormRamp(initramp);
        ramph->setRamp(initramp);
        ramph->setFixedHeight(40);  // bar breedte 40 - text(= 16) - indicator

        _MyCanvas->setColorRamp(ramph);
    }
    else if (vector_draw == VECTOR_DIRECTION)
    {
        QVector<QPair<qreal, QColor> > initramp;
        initramp.push_back(QPair<qreal, QColor>(0.00, QColor(0, 0, 255)));  // west
        initramp.push_back(QPair<qreal, QColor>(0.25, QColor(255, 0, 0)));  // south
        initramp.push_back(QPair<qreal, QColor>(0.50, QColor(255, 255, 255)));  // east
        initramp.push_back(QPair<qreal, QColor>(0.75, QColor(0, 255, 0)));  // north
        initramp.push_back(QPair<qreal, QColor>(1.00, QColor(0, 0, 255)));  // west

        ramph->setSlideUpdate(true);
        ramph->setMappingTextVisualize(true);
        ramph->setMappingTextColor(Qt::black);
        ramph->setMappingTextAccuracy(2);
        ramph->setNormRamp(initramp);
        ramph->setRamp(initramp);
        ramph->setFixedHeight(40);  // bar breedte 40 - text(= 16) - indicator
        ramph->setMinMax(-180.0, 180.0);

        _MyCanvas->setColorRampVector(ramph);
    }

    return ramph;
}
void MapTimeManagerWindow::button_group_pressed(int)
{
    // get selected variable name (for each mesh in the UGRID Mesh group; dependent on the long_name; not for every variable is a standard_name available, such as the D-WAQ variables)
    // get location of the variable (node, edge, ...)
    // get first time which need to visualised
    // get the variable values
    // get the variable locations
    // show the variable on canvas
}
QSlider * MapTimeManagerWindow::create_time_slider()
{
    QSlider * sl = new QSlider(Qt::Horizontal);
    sl->setMinimum(0);
    sl->setMaximum(nr_times - 1);

    connect(sl, &QAbstractSlider::valueChanged, this, &MapTimeManagerWindow::setValue);
    connect(curr_date_time, &MyQDateTimeEdit::dateTimeChanged, this, &MapTimeManagerWindow::setSliderValue);

    return sl;  
}

QCheckBox * MapTimeManagerWindow::check_parameter_1d()
{
    QCheckBox * checkb = new QCheckBox("1D");
    checkb->setToolTip("Show/Hide Map results");
    checkb->setStatusTip("Show/Hide Map results");
    checkb->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
    checkb->setCheckable(true);
    checkb->setChecked(false);
    checkb->setEnabled(true);
    m_show_map_data_1d = false;
    connect(checkb, &QCheckBox::stateChanged, this, &MapTimeManagerWindow::show_hide_map_data_1d);

    return checkb;
}


QCheckBox * MapTimeManagerWindow::check_parameter_1d2d()
{
    QCheckBox * checkb = new QCheckBox("1D2D");
    checkb->setToolTip("Show/Hide Map results");
    checkb->setStatusTip("Show/Hide Map results");
    checkb->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
    checkb->setCheckable(true);
    checkb->setChecked(false);
    checkb->setEnabled(true);
    m_show_map_data_1d2d = false;
    connect(checkb, &QCheckBox::stateChanged, this, &MapTimeManagerWindow::show_hide_map_data_1d2d);

    return checkb;
}

QCheckBox * MapTimeManagerWindow::check_parameter_2d()
{
    QCheckBox * checkb = new QCheckBox();
    checkb->setText("2D");
    checkb->setToolTip("Show/Hide Map results");
    checkb->setStatusTip("Show/Hide Map results");
    checkb->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
    checkb->setCheckable(true);
    checkb->setChecked(false);
    checkb->setEnabled(true);
    m_show_map_data_2d = false;
    connect(checkb, &QCheckBox::stateChanged, this, &MapTimeManagerWindow::show_hide_map_data_2d);

    return checkb;
}

QCheckBox * MapTimeManagerWindow::check_parameter_3d()
{
    QCheckBox * checkb = new QCheckBox("3D");
    checkb->setToolTip("Show/Hide Map results");
    checkb->setStatusTip("Show/Hide Map results");
    checkb->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
    checkb->setCheckable(true);
    checkb->setChecked(false);
    checkb->setEnabled(true);
    m_show_map_data_3d = false;
    connect(checkb, &QCheckBox::stateChanged, this, &MapTimeManagerWindow::show_hide_map_data_3d);

    return checkb;
}
QCheckBox * MapTimeManagerWindow::check_vector_2d()
{
    QCheckBox * checkb = new QCheckBox();
    checkb->setText("2DH");
    checkb->setToolTip("Show/Hide 2DH vector velocity");
    checkb->setStatusTip("Show/Hide depth averaged vector velocity");
    checkb->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
    checkb->setCheckable(true);
    checkb->setChecked(false);
    checkb->setEnabled(true);
    m_show_map_vector_2d = false;
    connect(checkb, &QCheckBox::stateChanged, this, &MapTimeManagerWindow::show_hide_map_vector_2d);

    return checkb;
}
QCheckBox * MapTimeManagerWindow::check_vector_3d()
{
    QCheckBox * checkb = new QCheckBox();
    checkb->setText("2D");
    checkb->setToolTip("Show/Hide layer velocity");
    checkb->setStatusTip("Show/Hide layer vector velocity");
    checkb->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
    checkb->setCheckable(true);
    checkb->setChecked(false);
    checkb->setEnabled(true);
    m_show_map_vector_3d = false;
    connect(checkb, &QCheckBox::stateChanged, this, &MapTimeManagerWindow::show_hide_map_vector_3d);

    return checkb;
}

QSpinBox * MapTimeManagerWindow::spinbox_layer(int max_val)
{
    QSpinBox * sb_layer = new QSpinBox();
    sb_layer->setRange(1, max_val);
    sb_layer->setValue(max_val);  // surface layer
    sb_layer->setEnabled(true);

    return sb_layer;
}

QPushButton * MapTimeManagerWindow::show_parameter()
{
#include "vsi.xpm"
#include "vsi_disabled.xpm"

    QPushButton * pb = new QPushButton();
    //QPixmap vsi_pm = QPixmap(vsi);
    //QSize vsi_size = vsi_pm.size();
    QIcon * icon_picture = new QIcon();
    icon_picture->addPixmap(QPixmap(vsi), QIcon::Normal, QIcon::On);
    icon_picture->addPixmap(QPixmap(vsi_disabled), QIcon::Normal, QIcon::Off);
    QAction * sp = new QAction();

    pb->addAction(sp);
    pb->setIcon(*icon_picture);
    pb->setToolTip("Show/Hide Map results");
    pb->setStatusTip("Show/Hide Map results");
    pb->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
    pb->setCheckable(true);
    pb->setChecked(false);
    pb->setEnabled(true);
    //m_show_map_data = false;

    //connect(pb, &QPushButton::released, this, &MapTimeManagerWindow::show_hide_map_data);

    //pb->setSizeIncrement(vsi_size);
    return pb;
}
QComboBox * MapTimeManagerWindow::create_parameter_selection_1d()
{
    QComboBox * cb = new QComboBox();
    cb->setMinimumSize(100, 22);

    cb->blockSignals(true);
    for (int i = 0; i < m_vars_1d.size(); i++)
    {
        if (m_vars_1d[i]->time_series)
        {
            int error_code;
            std::string att_value;
            QMap<QString, int> map;
            error_code = ugridapi::ug_variable_get_attribute_value(m_ugrid_file->m_ncid, m_vars[i]->var_name, "long_name", att_value);
            QString name = QString::fromStdString(att_value).trimmed();
            map[name] = i;
            cb->addItem(name, map[name]);
        }
    }
    cb->model()->sort(0);  // sort alphabetical order, should be set after combobox is filled
    cb->blockSignals(false);

    connect(cb, SIGNAL(activated(int)), this, SLOT(cb_clicked_1d(int)));

    return cb;
}
QComboBox * MapTimeManagerWindow::create_parameter_selection_1d2d()
{
    QComboBox * cb = new QComboBox();
    cb->setMinimumSize(100, 22);

    cb->blockSignals(true);
    for (int i = 0; i < m_vars_1d2d.size(); i++)
    {
        if (m_vars_1d2d[i]->time_series)
        {
            int error_code;
            std::string att_value;
            QMap<QString, int> map;
            error_code = ugridapi::ug_variable_get_attribute_value(m_ugrid_file->m_ncid, m_vars_1d2d[i]->var_name, "long_name", att_value);
            QString name = QString::fromStdString(att_value).trimmed();
            map[name] = i;
            cb->addItem(name, map[name]);
        }
    }
    cb->model()->sort(0);  // sort alphabetical order, should be set after combobox is filled
    cb->blockSignals(false);

    connect(cb, SIGNAL(activated(int)), this, SLOT(cb_clicked_1d2d(int)));

    return cb;
}
int MapTimeManagerWindow::create_parameter_selection_2d_3d(QComboBox * cb_2d, QComboBox * cb_3d)
{
    cb_2d->setMinimumSize(100, 22);
    cb_3d->setMinimumSize(100, 22);

    cb_2d->blockSignals(true);
    cb_3d->blockSignals(true);

    cb_2d->setInsertPolicy(QComboBox::InsertAlphabetically);
    cb_3d->setInsertPolicy(QComboBox::InsertAlphabetically);

    for (int i = 0; i < m_vars_2d.size(); i++)
    {
        if (m_vars_2d[i]->time_series)
        {
            int error_code;
            std::string att_value;
            QMap<QString, int> map;
            error_code = ugridapi::ug_variable_get_attribute_value(m_ugrid_file->m_ncid, m_vars_2d[i]->var_name, "long_name", att_value);
            QString name = QString::fromStdString(att_value).trimmed();
            map[name] = i;

            int dimension_count;
            error_code = ugridapi::ug_variable_count_dimensions(m_ugrid_file->m_ncid, m_vars_2d[i]->var_name, dimension_count);

            if (dimension_count == 2)  // Todo: HACK: assumed time, xy-space
            {
                cb_2d->addItem(name, map[name]);
            }
            else if (dimension_count == 3)  // Todo: HACK: assumed time, xy-space, layer
            {
                if (m_vars_2d[i]->sediment_index != -1)
                {
                    cb_2d->addItem(name, map[name]);
                }
                else
                {
                    cb_3d->addItem(name, map[name]);
                }
            }
            else if (dimension_count == 4)  // Todo: HACK Dimensions are: time, xy_space, hydro/bed-layer, sediment
            {
                cb_3d->addItem(name, map[name]);
            }
        }
    }
    cb_2d->model()->sort(0);  // sort alphabetical order, should be set after combobox is filled
    cb_3d->model()->sort(0);  // sort alphabetical order, should be set after combobox is filled

    cb_2d->blockSignals(false);
    cb_3d->blockSignals(false);

    connect(cb_2d, SIGNAL(activated(int)), this, SLOT(cb_clicked_2d(int)));
    connect(cb_3d, SIGNAL(activated(int)), this, SLOT(cb_clicked_3d(int)));

    return 0;
}
QVBoxLayout * MapTimeManagerWindow::create_scalar_selection_1d_2d_3d()
{
    QVBoxLayout * vl_tw_iso = new QVBoxLayout();  // VerticalLayout_TabWidget
    QGridLayout * hl = new QGridLayout();
    hl->setColumnStretch(0, 0);
    hl->setColumnStretch(1, 100);

    int status = 1;
    int row = 0;
    struct _mesh_1d m1d = m_ugrid_file->get_mesh_1d();
    if (m1d.name.size() != 0)
    {
        m_show_check_1d = check_parameter_1d();
        hl->addWidget(m_show_check_1d, row, 0);
        m_cb_1d = create_parameter_selection_1d();
        hl->addWidget(m_cb_1d, row, 1);
    }

    struct _contacts  m1d2d = m_ugrid_file->get_mesh_contacts();
    if (m1d2d.name.size() != 0)
    {
        row += 1;
        m_show_check_1d2d = check_parameter_1d2d();
        hl->addWidget(m_show_check_1d2d, row, 0);
        m_cb_1d2d = create_parameter_selection_1d2d();
        hl->addWidget(m_cb_1d2d, row, 1);
    }

    struct _mesh_2d m2d = m_ugrid_file->get_mesh_2d();
    if (m2d.name.size() != 0)
    {
        m_cb_2d = new QComboBox();
        m_cb_3d = new QComboBox();
        status = create_parameter_selection_2d_3d(m_cb_2d, m_cb_3d);
        if (m_cb_2d->count() != 0)
        {
            row += 1;
            m_show_check_2d = check_parameter_2d();
            hl->addWidget(m_show_check_2d, row, 0);
            hl->addWidget(m_cb_2d, row, 1);
        }
        if (m_cb_3d->count() != 0)
        {
            row += 1;
            // checkbox and combobox
            m_show_check_3d = check_parameter_3d();
            hl->addWidget(m_show_check_3d, row, 0);
            hl->addWidget(m_cb_3d, row, 1);

            // spinbox and layer selection
            QVariant j = m_cb_3d->itemData(0);
            int jj = j.toInt();
            struct _variable_ugridapi * var = m_vars_2d[jj];
            if (var->nr_hydro_layers > 0)
            {
                m_layerLabelPrefix = new QLabel(tr("Layer"));
                m_layerLabelSuffix = new QLabel(tr("[0,0]"));
                m_layerLabelSuffix->setText(tr("z/sigma: %1"));  //.arg(var->layer_center[var->nr_hydro_layers - 1]));
                m_sb_hydro_layer = spinbox_layer(var->nr_hydro_layers);
                connect(m_sb_hydro_layer, SIGNAL(valueChanged(int)), this, SLOT(spinbox_value_changed(int)));

                QHBoxLayout * sp_group_3d = new QHBoxLayout();
                sp_group_3d->addWidget(m_layerLabelPrefix);
                sp_group_3d->addWidget(m_sb_hydro_layer);
                sp_group_3d->addWidget(m_layerLabelSuffix);
                sp_group_3d->addStretch();

                row += 1;
                hl->addLayout(sp_group_3d, row, 1);
            }
            if (var->nr_bed_layers > 0)
            {
                m_layerLabelPrefix = new QLabel(tr("Layer"));
                m_layerLabelSuffix = new QLabel();
                m_layerLabelSuffix->setText(tr("Bed layer: %1"));  // .arg(var->layer_center[0]));
                m_sb_bed_layer = spinbox_layer(var->nr_bed_layers);
                m_sb_bed_layer->setValue(1);
                connect(m_sb_bed_layer, SIGNAL(valueChanged(int)), this, SLOT(spinbox_value_changed(int)));

                QHBoxLayout * sp_group_3d_bed = new QHBoxLayout();
                sp_group_3d_bed->addWidget(m_layerLabelPrefix);
                sp_group_3d_bed->addWidget(m_sb_bed_layer);
                sp_group_3d_bed->addWidget(m_layerLabelSuffix);
                sp_group_3d_bed->addStretch();

                row += 1;
                hl->addLayout(sp_group_3d_bed, row, 1);
            }
        }
    }
    vl_tw_iso->addLayout(hl);
    m_ramph = create_color_ramp(VECTOR_NONE);
    vl_tw_iso->addWidget(m_ramph);

    vl_tw_iso->addStretch();

    return vl_tw_iso;
}
QVBoxLayout * MapTimeManagerWindow::create_vector_selection_2d_3d()
{
    int status = 1;
    int row = -1;

    struct _mesh_2d m2d = m_ugrid_file->get_mesh_2d();
    if (m2d.name.size() == 0) { return nullptr; }

    m_cb_vec_2d = new QComboBox();
    m_cb_vec_3d = new QComboBox();
    m_cb_vec_2d->setMinimumSize(100, 22);
    QString text = QString::fromStdString(m2d.name);
    status = create_parameter_selection_vector_2d_3d(text, m_cb_vec_2d, m_cb_vec_3d);
    if (m_cb_vec_2d->count() == 0 && m_cb_vec_3d->count() == 0) { return nullptr;  }

    QVBoxLayout * vl_tw_vec = new QVBoxLayout();  // VerticalLayout_TabWidget
    QGridLayout * gl = new QGridLayout();
    gl->setColumnStretch(0, 0);
    gl->setColumnStretch(1, 100);

    if (m_cb_vec_2d->count() > 0)
    {
        row += 1;
        m_show_check_vec_2d = check_vector_2d();
        gl->addWidget(m_show_check_vec_2d, row, 0);
        gl->addWidget(m_cb_vec_2d, row, 1);
    }
    if (m_cb_vec_3d->count() > 0)
    {
        row += 1;
        m_show_check_vec_3d = check_vector_3d();
        gl->addWidget(m_show_check_vec_3d, row, 0);
        gl->addWidget(m_cb_vec_3d, row, 1);

        // spinbox and layer selection
        QVariant j = m_cb_vec_3d->itemData(0);
        QStringList strings= j.toStringList();
        struct _variable_ugridapi * var = m_vars_2d[strings[3].toInt()];
        m_layerLabelPrefix_vec = new QLabel(tr("Layer"));
        m_layerLabelSuffix_vec = new QLabel(tr("[0,0]"));
        m_layerLabelSuffix_vec->setText(tr("z/sigma: %1"));  // .arg(var->layer_center[var->nr_hydro_layers - 1]));
        m_sb_hydro_layer_vec = spinbox_layer(var->nr_hydro_layers);
        connect(m_sb_hydro_layer_vec, SIGNAL(valueChanged(int)), this, SLOT(spinbox_vec_value_changed(int)));

        QHBoxLayout * sp_group_vec = new QHBoxLayout();
        sp_group_vec->addWidget(m_layerLabelPrefix_vec);
        sp_group_vec->addWidget(m_sb_hydro_layer_vec);
        sp_group_vec->addWidget(m_layerLabelSuffix_vec);
        sp_group_vec->addStretch();
        row += 1;
        gl->addLayout(sp_group_vec, row, 1);
    }
    
    vl_tw_vec->addLayout(gl);
    m_ramph_vec_dir = create_color_ramp(VECTOR_DIRECTION);
    vl_tw_vec->addWidget(m_ramph_vec_dir);
    vl_tw_vec->addStretch();

    connect(m_cb_vec_2d, SIGNAL(activated(int)), this, SLOT(cb_clicked_vec_2d(int)));
    connect(m_cb_vec_3d, SIGNAL(activated(int)), this, SLOT(cb_clicked_vec_3d(int)));

    return vl_tw_vec;
}
int MapTimeManagerWindow::create_parameter_selection_vector_2d_3d(QString text, QComboBox * cb_vec_2d, QComboBox * cb_vec_3d)
{
    int vec_cartesian_component_2dh = 0;
    int vec_spherical_component_2dh = 0;
    int vec_cartesian_component = 0;
    int vec_spherical_component = 0;
    QStringList cart_2dh{ "","","","" };
    QStringList cart_layer{ "","","","" };
    QStringList spher_2dh{ "","","","" };
    QStringList spher_layer{ "","","","" };

    cb_vec_2d->setMinimumSize(100, 22);
    cb_vec_3d->setMinimumSize(100, 22);

    cb_vec_2d->blockSignals(true);
    cb_vec_3d->blockSignals(true);
    for (int i = 0; i < m_vars_2d.size(); i++)
    {
        if (m_vars_2d[i]->time_series)
        {
            int error_code;
            QMap<QString, int> map;
            std::string att_value;
            int dimension_count;
            error_code = ugridapi::ug_variable_get_attribute_value(m_ugrid_file->m_ncid, m_vars_2d[i]->var_name, "standard_name", att_value);
            QString std_name = QString::fromStdString(att_value).trimmed();
            if (std_name.isEmpty())
            {
                error_code = ugridapi::ug_variable_get_attribute_value(m_ugrid_file->m_ncid, m_vars_2d[i]->var_name, "long_name", att_value);
                std_name = QString::fromStdString(att_value).trimmed();  // Todo: HACK for sediment, they do not have standard names

            }
            map[std_name] = i;
            error_code = ugridapi::ug_variable_get_attribute_value(m_ugrid_file->m_ncid, m_vars_2d[i]->var_name, "mesh", att_value);
            QString mesh_var_name = QString::fromStdString(att_value).trimmed();
            error_code = ugridapi::ug_variable_count_dimensions(m_ugrid_file->m_ncid, m_vars_2d[i]->var_name, dimension_count);
            if (dimension_count == 2)  // Todo: HACK: assumed time, xy-space
            {
                if (mesh_var_name == text)
                {
                    if (std_name.contains("sea_water_x_velocity") || std_name.contains("sea_water_y_velocity"))
                    {
                        vec_cartesian_component_2dh += 1;
                        if (std_name.contains("sea_water_x_velocity")) { cart_2dh[1] = QString::fromStdString(m_vars_2d[i]->var_name).trimmed(); }
                        if (std_name.contains("sea_water_y_velocity")) { cart_2dh[2] = QString::fromStdString(m_vars_2d[i]->var_name).trimmed(); }
                    }
                    if (std_name.contains("eastward_sea_water_velocity") || std_name.contains("northward_sea_water_velocity"))
                    {
                        vec_spherical_component_2dh += 1;
                        if (std_name.contains("eastward_sea_water_velocity")) { spher_2dh[1] = QString::fromStdString(m_vars_2d[i]->var_name).trimmed(); }
                        if (std_name.contains("northward_sea_water_velocity")) { spher_2dh[2] = QString::fromStdString(m_vars_2d[i]->var_name).trimmed(); }
                    }
                    if (vec_cartesian_component_2dh == 2 || vec_spherical_component_2dh == 2)
                    {
                        QString name("Depth Averaged velocity vector");
                        QString name_dir("Depth Averaged velocity vector direction");
                        if (vec_cartesian_component_2dh == 2) {
                            vec_cartesian_component_2dh = 0;
                            cart_2dh[0] = QString("Cartesian");
                            cart_2dh[3] = QString("%1").arg(i);
                            m_cb_vec_2d->addItem(name, cart_2dh);
                            m_cb_vec_2d->addItem(name_dir, cart_2dh);
                        }
                        if (vec_spherical_component_2dh == 2) {
                            vec_spherical_component_2dh = 0;
                            spher_2dh[0] = QString("Spherical");
                            spher_2dh[3] = QString("%1").arg(i);
                            m_cb_vec_2d->addItem(name, spher_2dh);
                            m_cb_vec_2d->addItem(name_dir, spher_2dh);
                        }
                    }
                }
            }
            else if (dimension_count == 3)  // Todo: HACK: assumed time, xy-space, layer
            {
                if (std_name.contains("sea_water_x_velocity") || std_name.contains("sea_water_y_velocity"))
                {
                    vec_cartesian_component += 1;
                    if (std_name.contains("sea_water_x_velocity")) { cart_layer[1] = QString::fromStdString(m_vars_2d[i]->var_name).trimmed(); }
                    if (std_name.contains("sea_water_y_velocity")) { cart_layer[2] = QString::fromStdString(m_vars_2d[i]->var_name).trimmed(); }
                }
                if (std_name.contains("eastward_sea_water_velocity") || std_name.contains("northward_sea_water_velocity"))
                {
                    vec_spherical_component += 1;
                    if (std_name.contains("eastward_sea_water_velocity")) { spher_layer[1] = QString::fromStdString(m_vars_2d[i]->var_name).trimmed(); }
                    if (std_name.contains("northward_sea_water_velocity")) { spher_layer[2] = QString::fromStdString(m_vars_2d[i]->var_name).trimmed(); }
                }
                if (vec_cartesian_component == 2 || vec_spherical_component == 2)
                {
                    QString name("Horizontal velocity vector");
                    QString name_dir("Horizontal velocity vector direction");
                    if (vec_cartesian_component == 2)
                    {
                        vec_cartesian_component = 0;
                        cart_layer[0] = QString("Cartesian");
                        cart_layer[3] = QString("%1").arg(i);
                        m_cb_vec_3d->addItem(name, cart_layer);
                        m_cb_vec_3d->addItem(name_dir, cart_layer);
                    }
                    if (vec_spherical_component == 2)
                    {
                        vec_spherical_component = 0;
                        spher_layer[0] = QString("Spherical");
                        spher_layer[3] = QString("%1").arg(i); 
                        m_cb_vec_3d->addItem(name, spher_layer);
                        m_cb_vec_3d->addItem(name_dir, spher_layer);
                    }
                }
            }
        }
    }
    cb_vec_2d->model()->sort(0);  // sort alphabetical order, should be set after combobox is filled
    cb_vec_3d->model()->sort(0);  // sort alphabetical order, should be set after combobox is filled

    cb_vec_2d->blockSignals(false);
    cb_vec_3d->blockSignals(false);

    return 0;
}

void MapTimeManagerWindow::start_reverse()
{
    //QMessageBox::warning(0, tr("Message"), QString("start_reverse pressed"));
    stop_time_loop = false;
    _current_step = _q_times.indexOf(curr_date_time->dateTime());
    for (int i = _current_step ; i >= first_date_time_indx; i--)
    {
        one_step_backward();
        QApplication::processEvents();
        if (stop_time_loop)
        {
            break;
        }
        double msec = 1000.*m_property->get_refresh_time();
        SLEEP(msec);
    }
    if (_current_step <= first_date_time_indx)
    {
        pb_pauze->setChecked(true);
    }

}
void MapTimeManagerWindow::pause_time_loop()
{
    //QMessageBox::warning(0, tr("Message"), QString("Goto_begin pressed"));
    stop_time_loop = true;
}
void MapTimeManagerWindow::start_forward()
{
    //QMessageBox::warning(0, tr("Message"), QString("start_forward pressed"));
    stop_time_loop = false;
    _current_step = _q_times.indexOf(curr_date_time->dateTime());
    for (int i = _current_step; i < _q_times.size(); i++)
    {
        one_step_forward();
        QApplication::processEvents();
        if (stop_time_loop)
        {
            break;
        }
        double msec = 1000.*m_property->get_refresh_time();
        SLEEP(msec);
    }
    if (_current_step >= last_date_time_indx)
    {
        pb_pauze->setChecked(true);
    }
}
void MapTimeManagerWindow::goto_begin()
{
    //QMessageBox::warning(0, tr("Message"), QString("Goto_begin pressed"));
    curr_date_time->setDateTime(_q_times[first_date_time_indx]);
    curr_date_time->setAnsatz(first_date_time_indx, first_date_time_indx);
    m_slider->setValue(first_date_time_indx);

    _MyCanvas->set_current_step(first_date_time_indx);
    //_MyCanvas->draw_all();
}
void MapTimeManagerWindow::one_step_backward()
{
    //QMessageBox::warning(0, tr("Message"), QString("One_step_backward pressed"));
    _current_step = _q_times.indexOf(curr_date_time->dateTime());
    _current_step = std::max(first_date_time_indx, std::min(--_current_step, last_date_time_indx));
    curr_date_time->setDateTime(_q_times[_current_step]);
    curr_date_time->setAnsatz(_current_step, _current_step);
    m_slider->setValue(_current_step);
    if (_current_step == first_date_time_indx) { stop_time_loop = true; }

    _MyCanvas->set_current_step(_current_step);
    //_MyCanvas->draw_all();
}
void MapTimeManagerWindow::one_step_forward()
{
    //QMessageBox::warning(0, tr("Message"), QString("One_step_forward pressed"));
    _current_step = _q_times.indexOf(curr_date_time->dateTime());
    _current_step = std::max(first_date_time_indx, std::min(++_current_step, last_date_time_indx));
    curr_date_time->setDateTime(_q_times[_current_step]);
    curr_date_time->setAnsatz(_current_step, _current_step);
    m_slider->setValue(_current_step);
    if (_current_step == last_date_time_indx) { stop_time_loop = true; }

    _MyCanvas->set_current_step(_current_step);
    //_MyCanvas->draw_all();
}
void MapTimeManagerWindow::goto_end()
{
    //QMessageBox::warning(0, tr("Message"), QString("Goto_end pressed"));
    curr_date_time->setDateTime(_q_times[last_date_time_indx]);
    curr_date_time->setAnsatz(last_date_time_indx, last_date_time_indx);
    m_slider->setValue(last_date_time_indx);

    _MyCanvas->set_current_step(last_date_time_indx);
    //_MyCanvas->draw_all();
}

void MapTimeManagerWindow::first_date_time_changed(const QDateTime & date_time)
{
    if (!_q_times.contains(date_time))
    {
        int i;
        for (i = 0; i < last_date_time_indx + 1; i++)
        {
            if (date_time < _q_times[i])
            {
                break;
            }
        }
        first_date_time->setStyleSheet("background-color: red");
        first_date_time->setAnsatz(std::max(0, i - 1), std::min(i, nr_times));
    }
    else
    {
        first_date_time->setStyleSheet("");
        first_date_time_indx = _q_times.indexOf(date_time);
    }
}
void MapTimeManagerWindow::curr_date_time_changed(const QDateTime & date_time)
{
    if (!_q_times.contains(date_time))
    {
        int i;
        for (i = first_date_time_indx; i < last_date_time_indx + 1; i++)
        {
            if (date_time < _q_times[i])
            {
                break;
            }
        }
        curr_date_time->setStyleSheet("background-color: red");
        curr_date_time->setAnsatz(std::max(first_date_time_indx, i - 1), std::min(i, last_date_time_indx));
    }
    else
    {
        curr_date_time->setStyleSheet("");
        int i = _q_times.indexOf(date_time);
        curr_date_time->setAnsatz(i, i);
    }
}
void MapTimeManagerWindow::last_date_time_changed(const QDateTime & date_time)
{
    if (!_q_times.contains(date_time))
    {
        int i;
        for (i = first_date_time_indx; i < _q_times.size(); i++)
        {
            if (date_time < _q_times[i])
            {
                break;
            }
        }
        last_date_time->setStyleSheet("background-color: red");
        last_date_time->setAnsatz(std::max(0, i - 1), std::min(i, nr_times));
    }
    else
    {
        last_date_time->setStyleSheet("");
        last_date_time_indx = _q_times.indexOf(date_time);
    }
}

void MapTimeManagerWindow::cb_clicked_1d(int item)
{
    for (int i = 0; i < m_cb_1d->count(); ++i)
    {
        QVariant k = m_cb_1d->itemData(i);
        int kk = k.toInt();
        m_vars_1d[kk]->draw = false;
    }
    _MyCanvas->empty_cache(CACHE_1D);

    QString str = m_cb_1d->itemText(item);
    QVariant j = m_cb_1d->itemData(item);
    int jj = j.toInt();
    struct _variable_ugridapi* var = m_vars_1d[jj];

    _MyCanvas->reset_min_max();
    _MyCanvas->set_draw_vector(VECTOR_NONE);
    if (m_map_property_window != nullptr)
    {
        m_map_property_window->set_dynamic_limits_enabled(true);
    }

    if (!m_show_map_data_1d)
    {
        QStringList coord{ "" };
        _MyCanvas->set_coordinate_type(coord);
        var->draw = false;
        return;
    }
    else
    {
        //if (m_show_check_1d2d != nullptr) { m_show_check_1d2d->setChecked(false); }
        //if (m_show_check_2d != nullptr) { m_show_check_2d->setChecked(false); }
        if (m_show_check_3d != nullptr) { m_show_check_3d->setChecked(false); }
        if (m_show_check_vec_2d != nullptr) { m_show_check_vec_2d->setChecked(false); }
        if (m_show_check_vec_3d != nullptr) { m_show_check_vec_3d->setChecked(false); }
        draw_time_dependent_data_1d(m_cb_1d, item);
    }
}

void MapTimeManagerWindow::cb_clicked_1d2d(int item)
{
    for (int i = 0; i < m_cb_1d2d->count(); ++i)
    {
        QVariant k = m_cb_1d2d->itemData(i);
        int kk = k.toInt();
        m_vars_1d2d[kk]->draw = false;
    }
    _MyCanvas->empty_cache(CACHE_1D2D);

    QString str = m_cb_1d2d->itemText(item);
    QVariant j = m_cb_1d2d->itemData(item);
    int jj = j.toInt();
    struct _variable_ugridapi* var = m_vars_1d2d[jj];

    _MyCanvas->reset_min_max();
    _MyCanvas->set_draw_vector(VECTOR_NONE);
    if (m_map_property_window != nullptr)
    {
        m_map_property_window->set_dynamic_limits_enabled(true);
    }

    if (!m_show_map_data_1d2d)
    {
        QStringList coord{ "" };
        _MyCanvas->set_coordinate_type(coord);
        _MyCanvas->set_variable(nullptr);
        var->draw = false;
        return;
    }
    else
    {
        //if (m_show_check_1d != nullptr) { m_show_check_1d->setChecked(false); }
        //if (m_show_check_2d != nullptr) { m_show_check_2d->setChecked(false); }
        if (m_show_check_3d != nullptr) { m_show_check_3d->setChecked(false); }
        if (m_show_check_vec_2d != nullptr) { m_show_check_vec_2d->setChecked(false); }
        if (m_show_check_vec_3d != nullptr) { m_show_check_vec_3d->setChecked(false); }
        draw_time_dependent_data_1d2d(m_cb_1d2d, item);
    }
}
void MapTimeManagerWindow::cb_clicked_2d(int item)
{
    for (int i = 0; i < m_cb_2d->count(); ++i)
    {
        QVariant k = m_cb_2d->itemData(i);
        int kk = k.toInt();
        m_vars_2d[kk]->draw = false;
    }
    _MyCanvas->empty_cache(CACHE_2D);

    QString str = m_cb_2d->itemText(item);
    QVariant j = m_cb_2d->itemData(item);
    int jj = j.toInt();
    struct _variable_ugridapi * var = m_vars_2d[jj];

    _MyCanvas->reset_min_max();
    _MyCanvas->set_draw_vector(VECTOR_NONE);
    if (m_map_property_window != nullptr)
    {
        m_map_property_window->set_dynamic_limits_enabled(true);
    }

    if (!m_show_map_data_2d)
    {
        QStringList coord{ "" };
        _MyCanvas->set_coordinate_type(coord);
        _MyCanvas->set_variable(nullptr);
        var->draw = false;
        return;
    }
    else
    {
        //if (m_show_check_1d != nullptr) { m_show_check_1d->setChecked(false); }
        //if (m_show_check_1d2d != nullptr) { m_show_check_1d2d->setChecked(false); }
        if (m_show_check_3d != nullptr) { m_show_check_3d->setChecked(false); }
        if (m_show_check_vec_2d != nullptr) { m_show_check_vec_2d->setChecked(false); }
        if (m_show_check_vec_3d != nullptr) { m_show_check_vec_3d->setChecked(false); }
        draw_time_dependent_data_2d(m_cb_2d, item);
    }
}
void MapTimeManagerWindow::cb_clicked_3d(int item)
{
    for (int i = 0; i < m_cb_3d->count(); ++i)
    {
        QVariant k = m_cb_3d->itemData(i);
        int kk = k.toInt();
        m_vars_2d[kk]->draw = false;
    }
    _MyCanvas->empty_caches();

    _MyCanvas->set_draw_vector(VECTOR_NONE);
    if (m_map_property_window != nullptr)
    {
        m_map_property_window->set_dynamic_limits_enabled(true);
    }

    QString str = m_cb_3d->itemText(item);
    QVariant j = m_cb_3d->itemData(item);
    int jj = j.toInt();

    struct _variable_ugridapi * var = m_vars_2d[jj];

    if (var->nr_hydro_layers > 0)
    {
        m_sb_hydro_layer->setRange(1, var->nr_hydro_layers);
        if (m_sb_hydro_layer->value() == 0)
        {
            m_sb_hydro_layer->setValue(var->nr_hydro_layers);
        }
        int i_lay = m_sb_hydro_layer->value();
        m_layerLabelSuffix->setText(tr("z/sigma: %1"));  // .arg(var->layer_center[i_lay - 1]));
    }

    if (var->nr_bed_layers > 0)
    {
        m_sb_bed_layer->setRange(1, var->nr_bed_layers);
        if (m_sb_bed_layer->value() == 0)
        {
            m_sb_bed_layer->setValue(var->nr_bed_layers);
        }
        int i_lay = m_sb_bed_layer->value();
        m_layerLabelSuffix->setText(tr("Bed layer: %1"));  // .arg(var->layer_center[i_lay - 1]));
    }
    
    _MyCanvas->reset_min_max();
    if (!m_show_map_data_3d)
    {
        QStringList coord;
        coord << "";
        _MyCanvas->set_coordinate_type(coord);
        _MyCanvas->set_variable(nullptr);
        return;
    }
    else
    {
        if (m_show_check_1d != nullptr) { m_show_check_1d->setChecked(false); }
        if (m_show_check_1d2d != nullptr) { m_show_check_1d2d->setChecked(false); }
        if (m_show_check_2d != nullptr) { m_show_check_2d->setChecked(false); }
        if (m_show_check_vec_2d != nullptr) { m_show_check_vec_2d->setChecked(false); }
        if (m_show_check_vec_3d != nullptr) { m_show_check_vec_3d->setChecked(false); }
        draw_time_dependent_data_2d(m_cb_3d, item);
    }
}
void MapTimeManagerWindow::cb_clicked_vec_2d(int item)
{
    for (int i = 0; i < m_cb_vec_2d->count(); ++i)
    {
        QVariant k = m_cb_vec_2d->itemData(i);
        int kk = k.toInt();
        m_vars_2d[kk]->draw = false;
    }
    _MyCanvas->empty_caches();

    _MyCanvas->reset_min_max();
    if (m_map_property_window != nullptr)
    {
        m_map_property_window->set_dynamic_limits_enabled(true);
    }
    if (!m_show_map_vector_2d)
    {
        QStringList coord;
        coord << "";
        _MyCanvas->set_coordinate_type(coord);
        _MyCanvas->set_variable(nullptr);
        return;
    }
    else
    {
        if (m_show_check_1d != nullptr) { m_show_check_1d->setChecked(false); }
        if (m_show_check_1d2d != nullptr) { m_show_check_1d2d->setChecked(false); }
        if (m_show_check_2d != nullptr) { m_show_check_2d->setChecked(false); }
        if (m_show_check_3d != nullptr) { m_show_check_3d->setChecked(false); }
        if (m_show_check_vec_3d != nullptr) { m_show_check_vec_3d->setChecked(false); }
        draw_time_dependent_vector(m_cb_vec_2d, item);
    }
}
void MapTimeManagerWindow::cb_clicked_vec_3d(int item)
{
    for (int i = 0; i < m_cb_vec_3d->count(); ++i)
    {
        QVariant k = m_cb_vec_3d->itemData(i);
        int kk = k.toInt();
        m_vars_2d[kk]->draw = false;
    }
    _MyCanvas->empty_caches();

    QString str = m_cb_vec_3d->itemText(item);
    QVariant j = m_cb_vec_3d->itemData(item);
    QStringList slist = j.toStringList();
    int jj = slist[3].toInt();

    struct _variable_ugridapi * var = m_vars_2d[jj];

    m_sb_hydro_layer_vec->setRange(1, var->nr_hydro_layers);
    if (m_sb_hydro_layer_vec->value() == 0)
    {
        m_sb_hydro_layer_vec->setValue(var->nr_hydro_layers);
    }
    int i_lay = m_sb_hydro_layer_vec->value();
    m_layerLabelSuffix_vec->setText(tr("z/sigma: %1"));  // .arg(var->layer_center[i_lay - 1]));

    _MyCanvas->reset_min_max();
    if (!m_show_map_vector_3d)
    {
        QStringList coord;
        coord << "";
        _MyCanvas->set_coordinate_type(coord);
        _MyCanvas->set_variable(nullptr);
        return;
    }
    else
    {
        if (m_show_check_1d != nullptr) { m_show_check_1d->setChecked(false); }
        if (m_show_check_1d2d != nullptr) { m_show_check_1d2d->setChecked(false); }
        if (m_show_check_2d != nullptr) { m_show_check_2d->setChecked(false); }
        if (m_show_check_3d != nullptr) { m_show_check_3d->setChecked(false); }
        if (m_show_check_vec_2d != nullptr) { m_show_check_vec_2d->setChecked(false); }
        draw_time_dependent_vector(m_cb_vec_3d, item);
    }
}
void MapTimeManagerWindow::draw_time_dependent_vector(QComboBox * cb, int item)
{
    // Vectors are only drawn on the cell circumcentres, i.e. the computational location of the pressure point
    QString str = cb->itemText(item);
    QVariant j = cb->itemData(item);
    QStringList coord = j.toStringList();
    int i = _q_times.indexOf(curr_date_time->dateTime());

    _MyCanvas->set_current_step(i);
    _MyCanvas->set_determine_grid_size(true);
    _MyCanvas->set_variables(m_vars_2d);
    _MyCanvas->set_coordinate_type(coord);

    if (!str.contains("Depth Averaged"))
    {
        if (m_sb_hydro_layer_vec != nullptr) {
            _MyCanvas->set_hydro_layer(m_sb_hydro_layer_vec->value());
        }
    }
    if (!str.contains("velocity vector direction"))
    {
        m_vector_draw = VECTOR_ARROW;  // draw the vector arrow
    }
    else
    {
        m_vector_draw = VECTOR_DIRECTION; // draw the vector direction
        if (m_map_property_window != nullptr)
        {
            m_map_property_window->set_dynamic_limits_enabled(false);
        }
    }
    _MyCanvas->set_draw_vector(m_vector_draw);
}
void MapTimeManagerWindow::draw_time_dependent_data_1d(QComboBox * cb, int item)
{
    int error_code{ -1 }; 

    QString str = cb->itemText(item);
    QVariant j = cb->itemData(item);
    int jj = j.toInt();

    //QMessageBox::information(0, "MapTimeManagerWindow::cb_clicked", QString("Selected: %1\nQMap value: %2").arg(str).arg(jj));

    struct _variable_ugridapi * var = m_vars_1d[jj];
    std::string var_name = var->var_name;
    std::string location;
    error_code = ugridapi::ug_variable_get_attribute_value(m_ugrid_file->m_ncid, var_name, "location", location);

    // begin HACK edge vs contact
    if (location == "edge" || location == "contact")
    {
        var->draw = true;;
        int i = _q_times.indexOf(curr_date_time->dateTime());
        _MyCanvas->set_current_step(i);
        _MyCanvas->draw_all();
    }
    else if (location == "face")
    {
        var->draw = true;;
        //int i = _q_times.indexOf(curr_date_time->dateTime());
        _MyCanvas->draw_all();
    }
    else if (location == "node")
    {
        var->draw = true;;
        int i = _q_times.indexOf(curr_date_time->dateTime());
        _MyCanvas->set_current_step(i);
        _MyCanvas->draw_all();
    }
    else
    {
        QMessageBox::warning(0, tr("draw_time_dependent_data_1d"), QString("Variable \"%1\" location \"%2\"").arg(var_name.c_str()).arg(location.c_str()));
    }
}
void MapTimeManagerWindow::draw_time_dependent_data_1d2d(QComboBox* cb, int item)
{
    //QMessageBox::information(0, "MapTimeManagerWindow::cb_clicked", QString("Selected: %1\nQMap value: %2").arg(str).arg(jj));
    int error_code{ -1 };

    QString str = cb->itemText(item);
    QVariant j = cb->itemData(item);
    int jj = j.toInt();

    struct _variable_ugridapi* var = m_vars_1d2d[jj];
    std::string var_name = var->var_name;
    std::string location;
    error_code = ugridapi::ug_variable_get_attribute_value(m_ugrid_file->m_ncid, var_name, "location", location);

    // begin HACK edge vs contact
    if (location == "edge" || location == "contact")
    {
        var->draw = true;
        int i = _q_times.indexOf(curr_date_time->dateTime());
        _MyCanvas->set_current_step(i);
        _MyCanvas->draw_all();
    }
    else if (location == "node")
    {
        var->draw = true;
        int i = _q_times.indexOf(curr_date_time->dateTime());
        _MyCanvas->set_current_step(i);
        _MyCanvas->draw_all();
    }
    else
    {
        QMessageBox::warning(0, tr("draw_time_dependent_data_1d2d"), QString("Variable \"%1\" location \"%2\"").arg(var_name.c_str()).arg(location.c_str()));
    }
}
void MapTimeManagerWindow::draw_time_dependent_data_2d(QComboBox* cb, int item)
{
    //QMessageBox::information(0, "MapTimeManagerWindow::cb_clicked", QString("Selected: %1\nQMap value: %2").arg(str).arg(jj));
    int error_code{ -1 };

    QString str = cb->itemText(item);
    QVariant j = cb->itemData(item);
    int jj = j.toInt();

    struct _variable_ugridapi* var = m_vars_2d[jj];
    std::string var_name = var->var_name;
    std::string location;
    error_code = ugridapi::ug_variable_get_attribute_value(m_ugrid_file->m_ncid, var_name, "location", location);

    // begin HACK edge vs contact
    if (location == "edge" || location == "contact")
    {
        var->draw = true;
        int i = _q_times.indexOf(curr_date_time->dateTime());
        _MyCanvas->set_current_step(i);
        _MyCanvas->draw_all();
    }
    else if (location == "face")  // || location == "volume")  // volume at same location as face, because of topview in a GIS system
    {
        var->draw = true;
        if (m_sb_hydro_layer != nullptr) {
            _MyCanvas->set_hydro_layer(m_sb_hydro_layer->value());
        }
        if (m_sb_bed_layer != nullptr) {
            _MyCanvas->set_bed_layer(m_sb_bed_layer->value());
        }
        //int i = _q_times.indexOf(curr_date_time->dateTime());
        _MyCanvas->draw_all();
    }
    else if (location == "node")
    {
        var->draw = true;
        int i = _q_times.indexOf(curr_date_time->dateTime());
        _MyCanvas->set_current_step(i);
        _MyCanvas->draw_all();
    }
    else
    {
        QMessageBox::warning(0, tr("draw_time_dependent_data_2d"), QString("Variable \"%1\" location \"%2\"").arg(var_name.c_str()).arg(location.c_str()));
    }
}

void MapTimeManagerWindow::setValue(int i)
{
    curr_date_time->setDateTime(_q_times[i]);
    curr_date_time->setAnsatz(i,i);

    _MyCanvas->set_current_step(i);
    _MyCanvas->draw_all();
}
void MapTimeManagerWindow::setSliderValue(QDateTime date_time)
{
    int i = _q_times.indexOf(date_time);
    m_slider->setValue(i);
}
void MapTimeManagerWindow::show_hide_map_data_1d()
{
    if (m_show_check_1d->checkState() == Qt::Checked)
    {
        m_show_map_data_1d = true;
        m_pb_cur_view->setEnabled(true);
    }
    else if (m_show_check_1d->checkState() == Qt::Unchecked)
    {
        m_show_map_data_1d = false;
        if (m_show_check_2d == nullptr || m_show_check_2d->checkState() == Qt::Unchecked)
        {
            m_pb_cur_view->setEnabled(false);
        }
    }
    cb_clicked_1d(m_cb_1d->currentIndex());
}
void MapTimeManagerWindow::show_hide_map_data_1d2d()
{
    if (m_show_check_1d2d->checkState() == Qt::Checked)
    {
        m_show_map_data_1d2d = true;
        m_pb_cur_view->setEnabled(false);
    }
    else if (m_show_check_1d2d->checkState() == Qt::Unchecked)
    {
        m_show_map_data_1d2d = false;
        if (m_show_check_1d->checkState() == Qt::Unchecked &&
            m_show_check_2d->checkState() == Qt::Unchecked)
        {
            m_pb_cur_view->setEnabled(false);
        }
    }
    cb_clicked_1d2d(m_cb_1d2d->currentIndex());
}
void MapTimeManagerWindow::show_hide_map_data_2d()
{
    if (m_show_check_2d->checkState() == Qt::Checked)
    {
        m_show_map_data_2d = true;
        m_pb_cur_view->setEnabled(true);
    }
    else if (m_show_check_2d->checkState() == Qt::Unchecked)
    {
        m_show_map_data_2d = false;
        if (m_show_check_1d == nullptr || m_show_check_1d->checkState() == Qt::Unchecked)
        {
            m_pb_cur_view->setEnabled(false);
        }
    }

    cb_clicked_2d(m_cb_2d->currentIndex());
}
void MapTimeManagerWindow::show_hide_map_data_3d()
{
    if (m_show_check_3d->checkState() == Qt::Checked)
    {
        m_show_map_data_3d = true;
    }
    else if (m_show_check_3d->checkState() == Qt::Unchecked)
    {
        m_show_map_data_3d = false;
    }
    cb_clicked_3d(m_cb_3d->currentIndex());
}
void MapTimeManagerWindow::show_hide_map_vector_2d()
{
    if (m_show_check_vec_2d->checkState() == Qt::Checked)
    {
        m_show_map_vector_2d = true;
    }
    else if (m_show_check_vec_2d->checkState() == Qt::Unchecked)
    {
        m_show_map_vector_2d = false;
    }
    cb_clicked_vec_2d(m_cb_vec_2d->currentIndex());
}
void MapTimeManagerWindow::show_hide_map_vector_3d()
{
    if (m_show_check_vec_3d->checkState() == Qt::Checked)
    {
        m_show_map_vector_3d = true;
    }
    else if (m_show_check_vec_3d->checkState() == Qt::Unchecked)
    {
        m_show_map_vector_3d = false;
    }
    cb_clicked_vec_3d(m_cb_vec_3d->currentIndex());
}
void MapTimeManagerWindow::spinbox_value_changed(int i_lay)
{
    QString str = m_cb_3d->currentText();
    QVariant j = m_cb_3d->currentData();
    int jj = j.toInt();

    struct _variable_ugridapi* var = m_vars_2d[jj];
    if (var->nr_hydro_layers > 0)
    {
        m_layerLabelSuffix->setText(tr("z/sigma: %1"));  // .arg(var->layer_center[i_lay - 1]));
    }
    if (var->nr_bed_layers > 0)
    {
        m_layerLabelSuffix->setText(tr("Bed layer: %1"));  // .arg(var->layer_center[i_lay - 1]));
    }
    return;
}
void MapTimeManagerWindow::spinbox_vec_value_changed(int i_lay)
{
    QString str = m_cb_vec_3d->currentText();
    QVariant j = m_cb_vec_3d->currentData();
    QStringList slist = j.toStringList();
    int jj = slist[3].toInt();

    struct _variable_ugridapi* var = m_vars_2d[jj];
    m_layerLabelSuffix_vec->setText(tr("z/sigma: %1"));  // .arg(var->layer_center[i_lay - 1]));
    return;
}
void MapTimeManagerWindow::clicked_current_view()
{
    //QMessageBox::information(0, "Information", "MapPropertyWindow::current_view()");
    DataValuesProvider2<double> dvp2;
    int error_code;

    if (AddCurrentViewWindow::get_count() == 0)  // create a window if it is not already there.
    {
        if (m_show_map_data_1d)
        {
            struct _mesh_1d mesh1d = m_ugrid_file->get_mesh_1d();
            struct _mapping* mapping = m_ugrid_file->get_grid_mapping();

            QString date_time = curr_date_time->dateTime().toString("1D - yyyy-MM-dd, HH:mm:ss");
            int time_index = _q_times.indexOf(curr_date_time->dateTime());
            QString text = m_cb_1d->currentText();
            QString quantity = date_time + "; " + text;

            QVariant j = m_cb_1d->currentData();
            int jj = j.toInt();
            struct _variable_ugridapi* var = m_vars_2d[jj];

            error_code = m_ugrid_file->get_var(var->var_name, dvp2);
            double * z_value = dvp2.get_timestep(var->var_name, time_index);  // xy_space

            std::string location;
            error_code = ugridapi::ug_variable_get_attribute_value(m_ugrid_file->m_ncid, var->var_name, "location", location);
            if (location == "edge")
            {
                m_cur_view = new AddCurrentViewWindow(m_QGisIface, date_time, quantity, z_value, mesh1d.edge_x, mesh1d.edge_y, mapping->epsg);
            }
            else if (location == "node")
            {
                m_cur_view = new AddCurrentViewWindow(m_QGisIface, date_time, quantity, z_value, mesh1d.node_x, mesh1d.node_y, mapping->epsg);
            }
        }
        else if (m_show_map_data_2d)
        {
            struct _mesh_2d mesh2d = m_ugrid_file->get_mesh_2d();
            struct _mapping * mapping = m_ugrid_file->get_grid_mapping();

            QString date_time = curr_date_time->dateTime().toString("2D - yyyy-MM-dd, HH:mm:ss");
            int time_index = _q_times.indexOf(curr_date_time->dateTime());
            QString text = m_cb_2d->currentText();
            QString quantity = date_time + "; " + text;

            QVariant j = m_cb_2d->currentData();
            int jj = j.toInt();
            struct _variable_ugridapi * var = m_vars_2d[jj];
            //struct _edge * edges = mesh2d->edge[0];

            error_code = m_ugrid_file->get_var(var->var_name, dvp2);
            double * z_value = dvp2.get_timestep(var->var_name, time_index);  // xy_space

            int error_code;
            std::string location;
            error_code = ugridapi::ug_variable_get_attribute_value(m_ugrid_file->m_ncid, var->var_name, "location", location);
            if (location == "edge")
            {
                m_cur_view = new AddCurrentViewWindow(m_QGisIface, date_time, quantity, z_value, mesh2d.edge_x, mesh2d.edge_y, mapping->epsg);
            }
            else if (location == "face")
            {
                m_cur_view = new AddCurrentViewWindow(m_QGisIface, date_time, quantity, z_value, mesh2d.face_x, mesh2d.face_y, mapping->epsg);
            }
            else if (location == "node")
            {
                m_cur_view = new AddCurrentViewWindow(m_QGisIface, date_time, quantity, z_value, mesh2d.node_x, mesh2d.node_y, mapping->epsg);
            }
        }
    }
    return;
}
//
//-----------------------------------------------------------------------------
//
QIcon MapTimeManagerWindow::get_icon_file(QDir home_dir, QString file)
{
    // if file does not exists, return the default icon 
#include "pattern.xpm"
    QIcon icon_path;
    QFileInfo path = QString(home_dir.canonicalPath()) + file;
    if (path.exists())
    {
        icon_path = QIcon(path.absoluteFilePath());  // todo: Should be from the PlotCFTS install directory
    }
    else
    {
        icon_path = QIcon(QPixmap(pattern));
    }
    return icon_path;
}
