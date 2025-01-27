#ifndef _INC_ADD_CURRENT_VIEW_WINDOW
#define _INC_ADD_CURRENT_VIEW_WINDOW
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDockWidget>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QHeaderView>

#include <qgisinterface.h>
#include <qgslayertree.h>
#include <qgslayertreegroup.h>
#include <qgslayertreenode.h>
#include <qgslayertreeview.h>
#include <qgsvectorlayer.h>
#include <qgsmarkersymbollayer.h>
#include <QgsSingleSymbolRenderer.h>
#include <QgsSymbol.h>

class AddCurrentViewWindow 
    : public QWidget
{
    Q_OBJECT

public:
    static int object_count;

    AddCurrentViewWindow();  // Constructor
    AddCurrentViewWindow(QgisInterface *, QString, QString, double *, std::vector<double>, std::vector<double>, long, double);  // Constructor
    ~AddCurrentViewWindow();  // Destructor
    static AddCurrentViewWindow * getInstance()
    {
        if (obj == nullptr)
            obj = new AddCurrentViewWindow();
        return obj;
    }
    static int get_count();

public slots:
    void close();
    void clicked_add();
    void clicked_close();
    void clicked(QModelIndex);

private:
    void create_vector_layer();
    void create_window();
    QgsLayerTreeGroup * get_subgroup(QgsLayerTreeGroup *, QString);
    void add_layer_to_group(QgsVectorLayer *, QgsLayerTreeGroup *);

    QgisInterface * m_QGisIface; // Pointer to the QGIS interface object
    static AddCurrentViewWindow * obj;
    QWidget * wid;
    QTableView * table;
    QStandardItemModel * table_model;
    QItemSelectionModel *selectionModel;
    QString m_label;
    QString m_quantity;
    double * m_z_value;
    std::vector<double> m_x;
    std::vector<double> m_y;
    long m_epsg;
    int m_cur_view;
    double m_missing_value;
};
#endif
