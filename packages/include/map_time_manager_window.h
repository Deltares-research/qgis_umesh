#ifndef dock_window_H
#define dock_window_H

#include <QObject>
#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>

#include <qmath.h>
#include <qgsapplication.h>
#include <qgisplugin.h>

#include <direct.h> // for getcwd
#include <stdlib.h> // for MAX_PATH

#include "ugrid.h"
#include "MyDrawingCanvas.h"
#include "my_date_time_edit.h"
#include "QColorRampEditor.h"
#include "map_property_window.h"
#include "map_property.h"

class MapTimeManagerWindow
    : public QDockWidget

{
    Q_OBJECT

public:
    static int object_count;

    public:
        MapTimeManagerWindow(UGRID *, MyCanvas *);
        ~MapTimeManagerWindow();
        static int get_count();

    public slots:
        void closeEvent(QCloseEvent *);
        void button_group_pressed(int);
        void cb_clicked_1d(int);
        void cb_clicked_1d2d(int);
        void cb_clicked_2d(int);
        void cb_clicked_3d(int);
        void cb_clicked_vec_2d(int);

        void start_reverse();
        void pause_time_loop();
        void start_forward();

        void goto_begin();
        void one_step_backward();
        void one_step_forward();
        void goto_end();

        void first_date_time_changed(const QDateTime &);
        void last_date_time_changed(const QDateTime &);
        void curr_date_time_changed(const QDateTime &); 

    public slots:
        void MyMouseReleaseEvent(QMouseEvent* e);
        void ramp_changed();
        void show_hide_map_data_1d();
        void show_hide_map_data_1d2d();
        void show_hide_map_data_2d();
        void show_hide_map_data_3d();
        void show_hide_map_vector_2d();
        void contextMenu(const QPoint &);

    public:
        QDockWidget * map_panel;

    private:
        UGRID * _ugrid_file;
        MyCanvas * _MyCanvas;
        void create_window();
        QGridLayout * create_date_time_layout();
        QHBoxLayout * create_push_buttons_layout_animation();
        QHBoxLayout * create_push_buttons_layout_steps();
        QSlider * create_time_slider();
        QPushButton * show_parameter();
        QCheckBox * check_parameter_1d();
        QCheckBox * check_parameter_1d2d();
        QCheckBox * check_parameter_2d();
        QCheckBox * check_parameter_3d();
        QCheckBox * check_vector_2d();
        QSpinBox * spinbox_layer(int);

        QComboBox * create_parameter_selection_1d(QString);
        QComboBox * create_parameter_selection_1d2d(QString);
        int create_parameter_selection_2d_3d(QString, QComboBox *, QComboBox *);
        QVBoxLayout * create_scalar_selection_1d_2d_3d();
        QVBoxLayout * create_vector_selection_2d_3d();

        QColorRampEditor * create_color_ramp();
        void draw_time_dependent_data_1d(QComboBox *, int);
        void draw_time_dependent_data(QComboBox *, int);
        void draw_time_dependent_vector(QComboBox *, int);
        QColorRampEditor * m_ramph;

        void setValue(int);
        void setSliderValue(QDateTime);
        QIcon get_icon_file(QDir, QString);

        QPushButton * pb_reverse;
        QPushButton * pb_start;
        QPushButton * pb_pauze;

        MyQDateTimeEdit * first_date_time;
        MyQDateTimeEdit * curr_date_time;
        MyQDateTimeEdit * last_date_time;

        int nr_times;
        QVector<QDateTime> _q_times;
        int _current_step;
        QSlider * m_slider;
        bool stop_time_loop;
        int first_date_time_indx;
        int last_date_time_indx;
        QComboBox * m_cb_1d = nullptr;
        QComboBox * m_cb_1d2d = nullptr;
        QComboBox * m_cb_2d = nullptr;
        QComboBox * m_cb_3d = nullptr;
        QComboBox * m_cb_vec_2d = nullptr;
        QCheckBox * m_show_check_1d = nullptr;
        QCheckBox * m_show_check_1d2d = nullptr;
        QCheckBox * m_show_check_2d = nullptr;
        QCheckBox * m_show_check_3d = nullptr;
        QCheckBox * m_show_check_vec_2d = nullptr;
        QLabel * m_layerLabelPrefix;
        QLabel * m_layerLabelSuffix;
        QSpinBox * m_sb_layer;
        bool m_show_map_data_1d;
        bool m_show_map_data_1d2d;
        bool m_show_map_data_2d;
        bool m_show_map_data_3d;
        bool m_show_map_vector_2d;
};

#endif