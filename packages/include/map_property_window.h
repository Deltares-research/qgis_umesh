#ifndef _INC_MAP_PROPERTY_WINDOW
#define _INC_MAP_PROPERTY_WINDOW
#include <QApplication>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPushButton>
#include "map_property.h"
#include "MyDrawingCanvas.h"

struct _bck_property {
    double opacity;
    double minimum;
    double maximum;
    bool dynamic_legend;
};

class MapPropertyWindow : public QWidget
{
    Q_OBJECT
    
public:
    MapPropertyWindow(MyCanvas *);  // Constructor
    ~MapPropertyWindow();  // Destructor
    
    void create_window();

signals:
    void draw_all();

private:
    QWidget * wid;
    QLabel * lbl_transparency;
    QLineEdit * le_transparency;
    QCheckBox * ckb;
    QLabel * lbl_min;
    QLabel * lbl_max;
    QLineEdit * le_min;
    QLineEdit * le_max;

    MapProperty * m_property;
    struct _bck_property * m_bck_property;
    QVector<QPair<qreal, QColor> > m_default_ramp;
    MyCanvas * m_myCanvas;

    void state_changed(int);
    void clicked_ok();
    void clicked_cancel();
    void clicked_apply();
};
#endif
