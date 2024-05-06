#ifndef _INC_MAP_PROPERTY_WINDOW
#define _INC_MAP_PROPERTY_WINDOW

#include <QCheckBox>
#include <QCloseEvent>
#include <QDateTimeEdit>
#include <QDockWidget>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include "map_property.h"
#include "MyDrawingCanvas.h"

struct _bck_property {
    double opacity;
    double refresh_time;
    bool dynamic_legend;
    double minimum;
    double maximum;
    double vector_scaling;
};

class MapPropertyWindow 
    : public QDockWidget
{
    Q_OBJECT

public:
    static int object_count;

    MapPropertyWindow(MyCanvas *);  // Constructor
    ~MapPropertyWindow();  // Destructor

    void create_window();
    static int get_count();
    void set_dynamic_limits_enabled(bool);

public slots:
    void close();
    void closeEvent(QCloseEvent *);
    void clicked_ok();
    void clicked_cancel();
    void clicked_apply();
    void setOpacityEditValue(int);
    void setOpacitySliderValue(QString);

signals:
    void draw_all();
    void close_map();

private:
    QWidget * wid;
    QLabel * lbl_opacity;
    QLineEdit * le_opacity;
    QSlider * sl_opacity;
    QLabel * lbl_refresh_time;
    QLineEdit * le_refresh_time;
    QLabel * lbl_min;
    QLabel * lbl_max;
    QLabel * lbl_vs;  // vector scaling
    QLineEdit * le_min;
    QLineEdit * le_max;
    QLineEdit * le_vs;  // vector scaling

    MapProperty * m_property;
    struct _bck_property * m_bck_property;
    QVector<QPair<qreal, QColor> > m_default_ramp;
    MyCanvas * m_myCanvas;
    QCheckBox * m_ckb;

    void state_changed(int);

};
#endif
