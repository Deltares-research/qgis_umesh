#include "add_current_view_window.h"

int AddCurrentViewWindow::object_count = 0;

AddCurrentViewWindow::AddCurrentViewWindow() : QWidget()
{
    object_count = 1;
}
AddCurrentViewWindow::AddCurrentViewWindow(QgisInterface * QGisIface, QString label, QString quantity, double * z_value, std::vector<double> x, std::vector<double> y, long epsg, double missing_value) : QWidget()
{
    //QMessageBox::information(0, "Information", "AddCurrentViewWindow::AddCurrentViewWindow()");
    object_count = 1;
    m_QGisIface = QGisIface;
    m_label = label;
    m_quantity = quantity;
    m_z_value = z_value;
    m_x = x;
    m_y = y;
    m_epsg = epsg;
    m_missing_value = missing_value;
    // quantity; time; layer: flow element center velocity magnitude; 2020-07-07 12:00:00; z/sigma = -2.5
    create_window();
}
AddCurrentViewWindow::~AddCurrentViewWindow()
{
    //QMessageBox::information(0, "Information", "MapPropertyWindow::~MapPropertyWindow()");
}
void AddCurrentViewWindow::close()
{
    //QMessageBox::information(0, "Information", "AddCurrentViewWindow::~close()");
    this->object_count = 0;
    wid->close();
    return;
}
int AddCurrentViewWindow::get_count()
{
    return object_count;
}
void AddCurrentViewWindow::create_window()
{
    wid = new QWidget();
    wid->setWindowTitle(QString("Add current view to QGIS layer panel"));
    wid->setMinimumWidth(700);

    QVBoxLayout * vl_main = new QVBoxLayout();
    QHBoxLayout * hl = new QHBoxLayout();

    table = new QTableView();
    long rows = 1;
    long cols = 2;
    table_model = new QStandardItemModel(rows, cols, this);
    table_model->setHeaderData(0, Qt::Horizontal, tr("Layer name"));
    table_model->setHeaderData(1, Qt::Horizontal, tr("Quantity"));

    table->setModel(table_model);
    selectionModel = new QItemSelectionModel(this->table_model);
    table->setSelectionModel(selectionModel);

    QHeaderView * headerView = table->horizontalHeader();
    headerView->setStretchLastSection(true);
    //headerView->setStyleSheet("QHeaderView::section { background-color: rgb(240, 243, 254) }");
    QFont font;
    font.setBold(true);
    headerView->setFont(font);

    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setShowGrid(true);
    table->verticalHeader()->hide();
    //int tab_width = table->horizontalHeader()->length() + table->verticalHeader()->width();
    int tab_height = table->verticalHeader()->length() + table->horizontalHeader()->height();
    table->setFixedHeight(tab_height);
    table->setColumnWidth(0, 200);
    table->setColumnWidth(1, 375);
    connect(table, SIGNAL(clicked(QModelIndex)), this, SLOT(clicked(QModelIndex)));

    //table->setStyleSheet("QTableView { background-color: rgb(255, 255, 0); }");

    QStandardItem * itm1 = new QStandardItem();
    itm1->setEditable(true);
    itm1->setText(m_label);
    table_model->setItem(0, 0, itm1);

    QStandardItem * itm2 = new QStandardItem();
    itm2->setEditable(false);
    itm2->setText(m_quantity);
    table_model->setItem(0, 1, itm2);

    vl_main->addWidget(table);

    QPushButton * pb_add = new QPushButton("Add");
    QPushButton * pb_close = new QPushButton("Close");

    hl->addStretch();
    hl->addWidget(pb_add);
    hl->addWidget(pb_close);

    vl_main->addLayout(hl);  // push buttons
    wid->setLayout(vl_main);

    wid->show();

    connect(pb_add, &QPushButton::clicked, this, &AddCurrentViewWindow::clicked_add);
    connect(pb_close, &QPushButton::clicked, this, &AddCurrentViewWindow::clicked_close);

}
void AddCurrentViewWindow::clicked(QModelIndex indx)
{
    int selRow = indx.row();
    int selCol = indx.column();
    Q_UNUSED(selRow);
    Q_UNUSED(selCol);

}
void AddCurrentViewWindow::clicked_add()
{
    //QMessageBox::information(0, "Information", "AddCurrentViewWindow::clicked_ok()");
    create_vector_layer();
}
void AddCurrentViewWindow::clicked_close()
{
    //QMessageBox::information(0, "Information", "AddCurrentViewWindow::clicked_cancel()");
    this->close();
}
void AddCurrentViewWindow::create_vector_layer()
{
    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
    QgsLayerTreeGroup * treeGroup;
    treeGroup = get_subgroup(treeRoot, QString("Current view"));
    QString layer_name = table_model->index(0, 0).data().toString();

    QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
    bool layer_found = false;
    for (int i = 0; i < tmp_layers.length(); i++)
    {
        if (layer_name == tmp_layers[i]->name())
        {
            layer_found = true;
        }
    }

    if (!layer_found)
    {
        QgsVectorLayer * vl;
        QgsVectorDataProvider * dp_vl;
        QList <QgsField> lMyAttribField;

        int nr_attrib_fields = 0;
        QString field_name = m_quantity;
        lMyAttribField << QgsField(field_name, QVariant::Double);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Point Id (0-based)", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Point Id (1-based)", QVariant::String);
        nr_attrib_fields++;

        QString uri = QString("Point?crs=epsg:") + QString::number(m_epsg);
        vl = new QgsVectorLayer(uri, layer_name, "memory");
        vl->startEditing();
        dp_vl = vl->dataProvider();
        dp_vl->addAttributes(lMyAttribField);
        //dp_vl->createSpatialIndex();
        vl->updatedFields();

        for (int j = 0; j < m_x.size(); j++)
        {
            if (m_z_value[j] == m_missing_value)
            {
                continue;
            }

            QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(m_x[j], m_y[j]));
            QgsFeature MyFeature;
            MyFeature.setGeometry(MyPoints);

            MyFeature.initAttributes(nr_attrib_fields);
            int k = -1;
            k++;
            MyFeature.setAttribute(k, m_z_value[j]);
            k++;
            MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
            k++;
            MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

            dp_vl->addFeature(MyFeature);
        }
        vl->commitChanges();
        vl->setTitle(field_name);

        QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
        simple_marker->setStrokeStyle(Qt::NoPen);

        QgsSymbol * marker = QgsSymbol::defaultSymbol(QgsWkbTypes::PointGeometry);
        marker->changeSymbolLayer(0, simple_marker);
        //set up a renderer for the layer
        QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
        vl->setRenderer(mypRenderer);

        add_layer_to_group(vl, treeGroup);
        //connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer

        tmp_layers = treeGroup->findLayers();
        int ind = tmp_layers.size() - 1;
        tmp_layers[ind]->setItemVisibilityChecked(false);
    }
}
QgsLayerTreeGroup * AddCurrentViewWindow::get_subgroup(QgsLayerTreeGroup * treeGroup, QString sub_group_name)
{
    QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();

    QgsLayerTreeGroup * subTreeGroup = treeGroup->findGroup(sub_group_name);
    if (subTreeGroup == nullptr)  // Treegroup Area does not exist create it
    {
        //treeGroup->addGroup(sub_group_name);
        treeGroup->insertGroup(0, sub_group_name);
        subTreeGroup = treeGroup->findGroup(sub_group_name);
        subTreeGroup->setExpanded(true);  // true is the default
        subTreeGroup->setItemVisibilityChecked(true);
        //QMessageBox::warning(0, "Message", QString("Create group: %1.").arg(name));
        subTreeGroup->setItemVisibilityCheckedRecursive(true);
    }
    return subTreeGroup;
}
void AddCurrentViewWindow::add_layer_to_group(QgsVectorLayer * vl, QgsLayerTreeGroup * treeGroup)
{
    QgsLayerTree * root = QgsProject::instance()->layerTreeRoot();
    QgsMapLayer * map_layer = QgsProject::instance()->addMapLayer(vl, false);
    treeGroup->addLayer(map_layer);
    root->removeLayer(map_layer);
}

