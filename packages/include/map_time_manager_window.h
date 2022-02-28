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
#include <QProcessEnvironment>
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
#include <qgisinterface.h>
#include <qgisplugin.h>

#include <direct.h> // for getcwd
#include <stdlib.h> // for MAX_PATH

#include "ugridapi_wrapper.h"
#include "MyDrawingCanvas.h"
#include "my_date_time_edit.h"
#include "QColorRampEditor.h"
#include "map_property_window.h"
#include "map_property.h"
#include "add_current_view_window.h"

#if defined WIN64
#define SLEEP _sleep
#endif

class MapTimeManagerWindow
    : public QDockWidget

{
    Q_OBJECT

public:
    static int object_count;

    public:
        MapTimeManagerWindow(QgisInterface *, UGRIDAPI_WRAPPER*, MyCanvas *);
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
        void cb_clicked_vec_3d(int);

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
        void show_hide_map_vector_3d();
        void spinbox_value_changed(int);
        void spinbox_vec_value_changed(int);
        void contextMenu(const QPoint &);
        void clicked_current_view();

    public:
        QDockWidget * map_panel;

    private:
        QgisInterface * m_QGisIface; // Pointer to the QGIS interface object
        UGRIDAPI_WRAPPER* m_ugrid_file;
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
        QCheckBox * check_vector_3d();
        QSpinBox * spinbox_layer(int);

        QComboBox * create_parameter_selection_1d();
        QComboBox * create_parameter_selection_1d2d();
        int create_parameter_selection_2d_3d(QComboBox *, QComboBox *);
        QVBoxLayout * create_scalar_selection_1d_2d_3d();
        QVBoxLayout * create_vector_selection_2d_3d();
        int create_parameter_selection_vector_2d_3d(QString, QComboBox *, QComboBox *);

        QColorRampEditor * create_color_ramp(vector_quantity);
        void draw_time_dependent_data_1d(QComboBox *, int);
        void draw_time_dependent_data_1d2d(QComboBox*, int);
        void draw_time_dependent_data_2d(QComboBox*, int);
        void draw_time_dependent_vector(QComboBox *, int);
        QColorRampEditor * m_ramph;
        QColorRampEditor * m_ramph_vec_dir;

        void setValue(int);
        void setSliderValue(QDateTime);
        QIcon get_icon_file(QDir, QString);

        QPushButton * pb_reverse;
        QPushButton * pb_start;
        QPushButton * pb_pauze;
        QPushButton * m_pb_cur_view;

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
        QComboBox * m_cb_vec_3d = nullptr;
        QCheckBox * m_show_check_1d = nullptr;
        QCheckBox * m_show_check_1d2d = nullptr;
        QCheckBox * m_show_check_2d = nullptr;
        QCheckBox * m_show_check_3d = nullptr;
        QCheckBox * m_show_check_vec_2d = nullptr;
        QCheckBox * m_show_check_vec_3d = nullptr;
        QLabel * m_layerLabelPrefix;
        QLabel * m_layerLabelPrefix_vec;
        QLabel * m_layerLabelSuffix;
        QLabel * m_layerLabelSuffix_vec;
        QSpinBox * m_sb_layer;
        QSpinBox * m_sb_hydro_layer_vec;
        QSpinBox * m_sb_hydro_layer;
        QSpinBox * m_sb_bed_layer_vec;
        QSpinBox * m_sb_bed_layer;
        bool m_show_map_data_1d;
        bool m_show_map_data_1d2d;
        bool m_show_map_data_2d;
        bool m_show_map_data_3d;
        bool m_show_map_vector_2d;
        bool m_show_map_vector_3d;
        std::vector<_variable_ugridapi*> m_vars;
        std::vector<_variable_ugridapi*> m_vars_1d;
        std::vector<_variable_ugridapi*> m_vars_1d2d;
        std::vector<_variable_ugridapi*> m_vars_2d;

        MapProperty * m_property;
        vector_quantity m_vector_draw;
        MapPropertyWindow * m_map_property_window;
        AddCurrentViewWindow * m_cur_view;
};
#endif
