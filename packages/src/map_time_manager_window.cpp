#include "map_time_manager_window.h"
#define SLEEP_MACRO _sleep(0100)
//
//-----------------------------------------------------------------------------
//
int MapTimeManagerWindow::object_count = 0;
MapProperty * MapProperty::obj;  // Initialize static member of class MapProperty (Singleton)

MapTimeManagerWindow::MapTimeManagerWindow(UGRID * ugrid_file, MyCanvas * MyCanvas) : QDockWidget()
{
    object_count++;
    _ugrid_file = ugrid_file;
    _MyCanvas = MyCanvas;
    _MyCanvas->setUgridFile(_ugrid_file);
    create_window(); //QMessageBox::information(0, "Information", "DockWindow::DockWindow()");
    _current_step = 0;
    _q_times = _ugrid_file->get_qdt_times();
    m_show_map_data = false;  // releated to pushbutton MapTimeManagerWindow::show_parameter
    MapProperty * m_property = MapProperty::getInstance();

    connect(m_ramph, &QColorRampEditor::rampChanged, this, &MapTimeManagerWindow::ramp_changed);

}
void MapTimeManagerWindow::MyMouseReleaseEvent(QMouseEvent* e)
{
    QMessageBox::information(0, "Information", "MapTimeManagerWindow::MyMouseReleaseEvent()");
}
void MapTimeManagerWindow::ramp_changed()
{
    //QMessageBox::information(0, "Information", "MapTimeManagerWindow::ramp_changed()");
    _MyCanvas->draw_all();
}

void MapTimeManagerWindow::contextMenu(const QPoint & point)
{
    //QMessageBox::information(0, "Information", "MapTimeManagerWindow::contextMenu()");
    MapPropertyWindow * map_property = new MapPropertyWindow(_MyCanvas);
}
//
//-----------------------------------------------------------------------------
//
MapTimeManagerWindow::~MapTimeManagerWindow()
{
    //QMessageBox::information(0, "Information", "MapTimeManagerWindow::~MapTimeManagerWindow()");
    closeEvent(NULL);
}
void MapTimeManagerWindow::closeEvent(QCloseEvent * ce)
{
    //QMessageBox::information(0, "Information", "MapTimeManagerWindow::~closeEvent()");
    this->object_count--;
    _MyCanvas->set_variable(nullptr);
    _MyCanvas->empty_caches();
    // TODO Reset timers, ie _current timestep etc
}
int MapTimeManagerWindow::get_count()
{
    return object_count;
}

void MapTimeManagerWindow::create_window()
{
    this->setWindowTitle(QString("Map Output Animation"));

    QWidget * wid = new QWidget();
    QVBoxLayout * vl = new QVBoxLayout();
    QHBoxLayout * hl= new QHBoxLayout();
    QAction * _show_p = new QAction();

    QGridLayout * gl = create_date_time_layout();
    vl->addLayout(gl);

    QHBoxLayout * hl1 = create_push_buttons_layout_animation();  // animation
    vl->addLayout(hl1);

    QHBoxLayout * hl2 = create_push_buttons_layout_steps();  // set steps
    vl->addLayout(hl2);

    m_slider = create_time_slider();

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    vl->addWidget(line);
    vl->addWidget(m_slider);

    QPushButton * _show_pb = show_parameter();
    hl->addWidget(_show_pb, 1);
    _cb = create_parameter_selection();
    hl->addWidget(_cb, 99);
    vl->addLayout(hl);

    m_ramph = create_color_ramp();
    vl->addWidget(m_ramph);

    vl->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    wid->setLayout(vl);
    this->setWidget(wid);
    return;
}
QGridLayout * MapTimeManagerWindow::create_date_time_layout()
{
    _q_times = _ugrid_file->get_qdt_times();
    nr_times = _q_times.size();
    first_date_time_indx = 0;
    last_date_time_indx = nr_times - 1;

    QGridLayout * gl = new QGridLayout();
    QLabel * lbl_start_time = new QLabel(QString("First time of animation"));
    QLabel * lbl_curr_time = new QLabel(QString("Current time (presented)"));
    QLabel * lbl_stop_time = new QLabel(QString("Last time of animation"));

    first_date_time = new MyQDateTimeEdit(_q_times);
    curr_date_time = new MyQDateTimeEdit(_q_times);
    last_date_time = new MyQDateTimeEdit(_q_times);
    QString format_date_time = QString("yyyy-MM-dd HH:mm:ss");

    // TODO set minimum and maximum within range first datetime and last datetime
    first_date_time->setDateTime(_q_times[0]);
    first_date_time->setMinimumDateTime(_q_times[0]);
    first_date_time->setMaximumDateTime(_q_times[nr_times - 1]);
    first_date_time->setDisplayFormat(format_date_time);
    first_date_time->setWrapping(true);
    
    curr_date_time->setDateTime(_q_times[0]);
    curr_date_time->setMinimumDateTime(_q_times[0]);
    curr_date_time->setMaximumDateTime(_q_times[nr_times - 1]);
    curr_date_time->setDisplayFormat(format_date_time);
    curr_date_time->setWrapping(true);
    
    last_date_time->setDateTime(_q_times[_q_times.size() - 1]);
    last_date_time->setMinimumDateTime(_q_times[0]);
    last_date_time->setMaximumDateTime(_q_times[nr_times - 1]);
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
    int width = 30;

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
QColorRampEditor * MapTimeManagerWindow::create_color_ramp()
{
    QVector<QPair<qreal, QColor> > initramp;
    initramp.push_back(QPair<qreal, QColor>(0.00, QColor(  0,   0, 255)));
    initramp.push_back(QPair<qreal, QColor>(0.25, QColor(  0, 255, 255)));
    initramp.push_back(QPair<qreal, QColor>(0.50, QColor(  0, 255,   0)));
    initramp.push_back(QPair<qreal, QColor>(0.75, QColor(255, 255,   0)));
    initramp.push_back(QPair<qreal, QColor>(1.00, QColor(255,   0,   0)));

    QColorRampEditor* ramph = new QColorRampEditor(NULL, Qt::Horizontal);
    ramph->setSlideUpdate(true);
    ramph->setMappingTextVisualize(true);
    ramph->setMappingTextColor(Qt::black);
    ramph->setMappingTextAccuracy(2);
    ramph->setNormRamp(initramp);
    ramph->setRamp(initramp);
    ramph->setFixedHeight(40);  // bar breedte 40 - text(= 16) - indicator

    _MyCanvas->setColorRamp(ramph);

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
    m_show_map_data = false;

    connect(pb, &QPushButton::released, this, &MapTimeManagerWindow::show_hide_map_data);

    //pb->setSizeIncrement(vsi_size);
    return pb;
}
QComboBox * MapTimeManagerWindow::create_parameter_selection()
{
    QComboBox * cb = new QComboBox();
    cb->setMinimumSize(100, 22);
    struct _mesh_variable * var = _ugrid_file->get_variables();

    cb->blockSignals(true);
    for (int i = 0; i < var->nr_vars; i++)
    {
        if (var->variable[i]->time_series)
        {
            QMap<QString, int> map;
            QString name = QString::fromStdString(var->variable[i]->long_name).trimmed();
            map[name] = i;
            QString mesh_var_name = QString::fromStdString(var->variable[i]->mesh).trimmed();
            // Tree types of meshes: 1D 1D2D and 2D
            //struct _mesh1d_string ** m1d = _ugrid_file->get_mesh1d_string();
            //if (m1d != nullptr)
            //{
                //if (mesh_var_name == QString::fromStdString(m1d[0]->var_name))
                //{
                    //mesh_var_name = QString::fromStdString(m1d[0]->long_name);
                //}
                cb->addItem(mesh_var_name + " - " + name, map[name]);
            //}
        }
    }
    cb->blockSignals(false);

    connect(cb, SIGNAL(activated(int)), this, SLOT(cb_clicked(int)));

    return cb;
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
        SLEEP_MACRO;
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
        SLEEP_MACRO;
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

void MapTimeManagerWindow::cb_clicked(int item)
{
    _MyCanvas->reset_min_max();
    if (!m_show_map_data)
    {
        _MyCanvas->set_variable(nullptr);
        _MyCanvas->empty_caches();
        return;
    }
    else
    {
        QString str = _cb->itemText(item);
        QVariant j = _cb->itemData(item);
        int jj = j.toInt();

        //QMessageBox::information(0, "MapTimeManagerWindow::cb_clicked", QString("Selected: %1\nQMap value: %2").arg(str).arg(jj));

        struct _mesh_variable * vars = _ugrid_file->get_variables();
        struct _variable * var = vars->variable[jj];
        string var_name = var->var_name;
        string location = var->location;

        if (location == "edge")
        {
            //QMessageBox::warning(0, tr("Message"), QString("Variable \"%1\" location \"%2\"").arg(var_name.c_str()).arg(location.c_str()));
            _MyCanvas->set_variable(var);
            int i = _q_times.indexOf(curr_date_time->dateTime());
            _MyCanvas->set_current_step(i);
            _MyCanvas->draw_all();
        }
        else if (location == "face")
        {
            //QMessageBox::warning(0, tr("Message"), QString("Variable \"%1\" location \"%2\"").arg(var_name.c_str()).arg(location.c_str()));
            _MyCanvas->set_variable(var);
            int i = _q_times.indexOf(curr_date_time->dateTime());
            _MyCanvas->draw_all();
        }
        else if (location == "node")
        {
            _MyCanvas->set_variable(var);
            int i = _q_times.indexOf(curr_date_time->dateTime());
            _MyCanvas->set_current_step(i);
            _MyCanvas->draw_all();
        }
        else
        {
            QMessageBox::warning(0, tr("Message"), QString("Variable \"%1\" location \"%2\"").arg(var_name.c_str()).arg(location.c_str()));
        }
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
    int j = _q_times.indexOf(curr_date_time->dateTime());
    if (j == i) { return; }  // prevent firing a signal by curr_date_time->setDateTime if date_time is already in the edit field
    m_slider->setValue(i);
}
void MapTimeManagerWindow::show_hide_map_data()
{
    if (m_show_map_data)
    {
        m_show_map_data = false;
    }
    else
    {
        m_show_map_data = true;
    }
    cb_clicked(_cb->currentIndex());
}

