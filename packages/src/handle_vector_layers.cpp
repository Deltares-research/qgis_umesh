#include "handle_vector_layers.h"

//------------------------------------------------------------------------------
HVL::HVL(QgisInterface* iface):
m_QGisIface(iface)
{
}
//------------------------------------------------------------------------------
//HVL::CVL(QgsMapLayer * obs_layer, QgsMapLayer * geom_layer, GRID * grid_file, QgisInterface * QGisIface)
//{
//}
//------------------------------------------------------------------------------
HVL::~HVL()
{
}
//------------------------------------------------------------------------------
void HVL::set_grid_file(std::vector<GRID *> grid_file)
{
    m_grid_file = grid_file;
}
//------------------------------------------------------------------------------
void HVL::CrsChanged()
{
    // Set the individual CRS via the group CRS, NOT the individual layers separately
    int checked = 0;
    QgsCoordinateReferenceSystem new_crs;
    QgsCoordinateReferenceSystem s_crs = QgsProject::instance()->crs();
    QgsMapLayer* active_layer = m_QGisIface->activeLayer();
    QString layer_name;  //
    if (active_layer == nullptr)
    {
        QgsLayerTree* treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
        QList<QgsLayerTreeNode*> selNodes = m_QGisIface->layerTreeView()->selectedNodes(true);
        for (int i = 0; i < selNodes.count(); i++)
        {
            checked = 0;
            QgsLayerTreeNode* selNode = selNodes[i];

            if (selNode->nodeType() == QgsLayerTreeNode::NodeType::NodeGroup)
            {
                checked = 1;
                QgsLayerTreeGroup* myGroup = treeRoot->findGroup(selNode->name());
                QList <QgsLayerTreeLayer*> layer = myGroup->findLayers();
                new_crs = layer[0]->layer()->crs();
                layer_name = layer[0]->layer()->name();
                //QMessageBox::information(0, "qgis_umesh::CrsChanged()", QString("Selected group: %1\nNew CRS: %2\nPrev. CRS: %3\nScreen CRS: %4").arg(selNode->name()).arg(new_crs.authid()).arg(m_crs.authid()).arg(s_crs.authid()));
                //QMessageBox::information(0, "qgis_umesh::CrsChanged()", QString("Layer[0] name: %1.").arg(layer[0]->layer()->name()));
                // Change coordinates to new_crs, by overwriting the mapping->epsg and mapping->epsg_code
                GRID* active_grid_file = get_active_grid_file(layer[0]->layer()->id());
                if (active_grid_file != nullptr)
                {
                    QString epsg_code = new_crs.authid();
                    QStringList parts = epsg_code.split(':');
                    long epsg = parts.at(1).toInt();
                    active_grid_file->set_grid_mapping_epsg(epsg, epsg_code.toUtf8().constData());
                }
            }
        }
        if (checked == 0)
        {
            QMessageBox::warning(0, "qgis_umesh::CrsChanged()", QString("Layer name: %1\nChecked: %2.").arg(active_layer->name()).arg(checked));
        }
    }
    else
    {
        // QMessageBox::information(0, "Message", QString("You have to select a group layer."));
    }
}
//-----------------------------------------------------------
void HVL::create_vector_layer_nodes(QString fname, QString layer_name, struct _feature * nodes, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (nodes != NULL)
    {
        START_TIMERN(create_vector_layer_nodes);
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        bool layer_found = false;
        for (int i = 0; i < tmp_layers.length(); i++)
        {
            //QMessageBox::warning(0, tr("Message: create_vector_layer_nodes"), QString(tr("Layers in group by name: ")) + tmp_layers[i]->name() + QString(" Look for: ") + layer_name);
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
            if (nodes->name.size() != 0)
            {
                lMyAttribField << QgsField("Node name", QMetaType::QString);
                nr_attrib_fields++;
            }
            if (nodes->long_name.size() != 0)
            {
                lMyAttribField << QgsField("Node long name", QMetaType::QString);
                nr_attrib_fields++;
            }
            lMyAttribField << QgsField("Node Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Node Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QVector<QVariant> attribute(nr_attrib_fields);
            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();
            QgsFeatureList MyFeatures;
            QgsFeature MyFeature;
            QgsGeometry MyPoints;
            // int nsig = long( log10(nodes->count) ) + 1;
            //START_TIMER(create_vector_layer_nodes_add_features);
            for (int j = 0; j < nodes->count; j++)
            {
                int k = -1;
                MyPoints = QgsGeometry::fromPointXY(QgsPointXY(nodes->x[j], nodes->y[j]));
                MyFeature.setGeometry(MyPoints);

                MyFeature.initAttributes(nr_attrib_fields);
                if (nodes->name.size() != 0)
                {
                    ++k;
                    attribute[k] = QString("%1").arg(QString::fromStdString(nodes->name[j]).trimmed());
                }
                if (nodes->long_name.size() != 0)
                {
                    ++k;
                    attribute[k] = QString("%1").arg(QString::fromStdString(nodes->long_name[j]).trimmed());
                }
                ++k;
                attribute[k] = QString("%1:0").arg(j);  // arg(j, nsig, 10, QLatin1Char('0')));
                ++k;
                attribute[k] = QString("%1:1").arg(j + 1);
                MyFeature.setAttributes(attribute);
                MyFeature.setValid(true);
                MyFeatures.append(MyFeature);
            }
            dp_vl->addFeatures(MyFeatures);
            vl->commitChanges();
            //STOP_TIMER(create_vector_layer_nodes_add_features);

            vl->serverProperties()->setTitle(layer_name + ": " + fname);

            if (layer_name == QString("Mesh1D Connection nodes") ||
                layer_name == QString("Mesh1D nodes") ||
                layer_name == QString("Mesh2D nodes") ||
                layer_name == QString("Mesh2D faces"))
            {
                QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
                if (layer_name == QString("Mesh1D Connection nodes"))
                {
                    simple_marker->setSize(3.5);
                    simple_marker->setColor(QColor(0, 0, 0));
                    simple_marker->setFillColor(QColor(0, 255, 0));
                }
                else if (layer_name.contains("Mesh1D nodes"))
                {
                    simple_marker->setSize(1.5);
                    simple_marker->setColor(QColor(0, 0, 0));
                    simple_marker->setFillColor(QColor(255, 255, 255));  // water level point
                }
                else if (layer_name.contains("Mesh2D faces") )
                {
                    simple_marker->setSize(1.5);
                    simple_marker->setColor(QColor(0, 0, 0));
                    simple_marker->setFillColor(QColor(255, 255, 255));  // water level point
                    vl->setSubLayerVisibility(layer_name, false);
                }
                else if (layer_name.contains("Mesh2D nodes"))
                {
                    simple_marker->setSize(0.50);
                    simple_marker->setColor(QColor(0, 0, 0));
                    simple_marker->setFillColor(QColor(0, 0, 1));  // mesh node point
                }

                QgsSymbol * marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
                marker->changeSymbolLayer(0, simple_marker);

                //set up a renderer for the layer
                QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
                vl->setRenderer(mypRenderer);
            }
            vl->blockSignals(false);

            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        STOP_TIMER(create_vector_layer_nodes);
    }
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_data_on_edges(QString fname, _variable * var, struct _feature * nodes, struct _edge * edges, double * z_value, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (nodes != nullptr && edges != nullptr)
    {
        START_TIMERN(create_vector_layer_data_on_edges);
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        QString layer_name = QString::fromStdString(var->long_name).trimmed();

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

            int nr_attrib_fields = 1;
            lMyAttribField << QgsField("Value", QMetaType::Double);

            QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            vl->updatedFields();

            QVector<QgsPointXY> point(2);
            QgsPolylineXY line;
            QgsFeatureList MyFeatures;
            MyFeatures.reserve(edges->count);

            for (int j = 0; j < edges->count; j++)
            {
                point.clear();
                int p1 = edges->edge_nodes[j][0];
                int p2 = edges->edge_nodes[j][1];
                double x1 = nodes->x[p1];
                double y1 = nodes->y[p1];
                double x2 = nodes->x[p2];
                double y2 = nodes->y[p2];
                point[0] = QgsPointXY(x1, y1);
                point[1] = QgsPointXY(x2, y2);
                //QMessageBox::warning(0, tr("Warning"), tr("Edge: %1 (%2, %3)->(%4, %5).").arg(j).arg(x1).arg(y1).arg(x2).arg(y2));
                line = point;

                QgsFeature MyFeature;
                MyFeature.setGeometry(QgsGeometry::fromPolylineXY(line));

                MyFeature.initAttributes(nr_attrib_fields);
                MyFeature.setAttribute(0, z_value[j]);
                MyFeature.setValid(true);
                MyFeatures.append(MyFeature);
            }
            dp_vl->addFeatures(MyFeatures);
            vl->commitChanges();
            vl->serverProperties()->setTitle(layer_name + ": " + fname);

            QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
            line_marker->setWidth(0.5);
            line_marker->setColor(QColor(200, 2000, 200));

            QgsSymbol * symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
            symbol->changeSymbolLayer(0, line_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));

            tmp_layers = treeGroup->findLayers();
            int ind = tmp_layers.size() - 1;
            tmp_layers[ind]->setItemVisibilityChecked(false);
        }
        STOP_TIMER(create_vector_layer_data_on_edges);
    }
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_edge_type(QString fname, _variable * var, struct _feature * nodes, struct _edge * edges, double * z_value, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (nodes != nullptr && edges != nullptr)
    {
        START_TIMERN(create_vector_layer_edge_type);
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        bool layer_found = false;
        if (!layer_found)
        {
            for (int i = 0; i < var->flag_meanings.size(); ++i)
            {
                QString layer_name = QString::fromStdString(var->flag_meanings[i]);

                QgsVectorLayer * vl;
                QgsVectorDataProvider * dp_vl;
                QList <QgsField> lMyAttribField;

                int nr_attrib_fields = 1;
                lMyAttribField << QgsField("Value", QMetaType::Double);

                QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
                vl = new QgsVectorLayer(uri, layer_name, "memory");
                vl->blockSignals(true);
                vl->startEditing();
                dp_vl = vl->dataProvider();
                dp_vl->addAttributes(lMyAttribField);
                vl->updatedFields();

                QVector<QgsPointXY> point(2);
                QgsFeatureList MyFeatures;
                MyFeatures.reserve(edges->count);

                for (int j = 0; j < edges->count; j++)
                {
                    if (var->flag_values[i] == int(z_value[j]))
                    {
                        int p1 = edges->edge_nodes[j][0];
                        int p2 = edges->edge_nodes[j][1];
                        point[0] = QgsPointXY(nodes->x[p1], nodes->y[p1]);
                        point[1] = QgsPointXY(nodes->x[p2], nodes->y[p2]);

                        QgsFeature MyFeature;
                        MyFeature.setGeometry(QgsGeometry::fromPolylineXY(point));

                        MyFeature.initAttributes(nr_attrib_fields);
                        MyFeature.setAttribute(0, z_value[j]);
                        MyFeature.setValid(true);
                        MyFeatures.append(MyFeature);
                    }
                }
                dp_vl->addFeatures(MyFeatures);
                vl->commitChanges();

                if (vl->featureCount() != 0)
                {
                    vl->serverProperties()->setTitle(layer_name + ": " + fname);

                    QgsSimpleLineSymbolLayer* line_marker = new QgsSimpleLineSymbolLayer();
                    if (i == 0) {
                        line_marker->setWidth(0.5);
                        line_marker->setColor(QColor(1, 1, 1));
                    }
                    if (i == 1) {
                        line_marker->setWidth(0.25);
                        line_marker->setColor(QColor(0, 170, 255));
                    }
                    if (i == 2) {
                        line_marker->setWidth(0.5);
                        line_marker->setColor(QColor(0, 0, 255));
                    }
                    if (i == 3) {
                        line_marker->setWidth(0.5);
                        line_marker->setColor(QColor(50, 50, 50));
                    }

                    QgsSymbol* symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
                    symbol->changeSymbolLayer(0, line_marker);

                    //set up a renderer for the layer
                    QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(symbol);
                    vl->setRenderer(mypRenderer);
                    vl->blockSignals(false);

                    add_layer_to_group(vl, treeGroup);
                    connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));

                    tmp_layers = treeGroup->findLayers();
                    int ind = tmp_layers.size() - 1;
                    tmp_layers[ind]->setItemVisibilityChecked(true);
                    if (i == 1)
                    {
                        tmp_layers[ind]->setItemVisibilityChecked(false);  // internal points
                    }
                }
            }
        }
        STOP_TIMER(create_vector_layer_edge_type);
    }
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_data_on_nodes(QString fname, _variable * var, struct _feature * nodes, double * z_value, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (nodes != nullptr)
    {
        START_TIMERN(create_vector_layer_data_on_nodes);
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        QString layer_name = QString::fromStdString(var->long_name).trimmed();

        bool layer_found = false;
        for (int i = 0; i < tmp_layers.length(); i++)
        {
            //QMessageBox::warning(0, tr("Message: create_vector_layer_nodes"), QString(tr("Layers in group by name: ")) + tmp_layers[i]->name() + QString(" Look for: ") + layer_name);
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

            int nr_attrib_fields = 1;
            lMyAttribField << QgsField("Value", QMetaType::Double);

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsFeatureList MyFeatures;
            QgsFeature MyFeature;
            for (int j = 0; j < nodes->count; j++)
            {
                MyFeature.setGeometry(QgsGeometry::fromPointXY(QgsPointXY(nodes->x[j], nodes->y[j])));

                MyFeature.initAttributes(nr_attrib_fields);
                MyFeature.setAttribute(0, z_value[j]);
                MyFeatures.append(MyFeature);
            }
            dp_vl->addFeatures(MyFeatures);
            vl->commitChanges();
            vl->serverProperties()->setTitle(layer_name + ": " + fname);

            QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
            simple_marker->setStrokeStyle(Qt::NoPen);

            QgsSymbol * marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
            marker->changeSymbolLayer(0, simple_marker);
            //set up a renderer for the layer
            if (true)
            {
                // previous
                QgsSingleSymbolRenderer * myRend = new QgsSingleSymbolRenderer(marker);
                vl->setRenderer(myRend);
            }
            else
            {
                /*
                QgsColorRampShader * shader = new QgsColorRampShader();
                QgsGraduatedSymbolRenderer * myRend = new QgsGraduatedSymbolRenderer("Value");
                myRend->updateSymbols(marker);
                qgscolorramp* ramp = new qgscolorramp();

                myRend->setSourceColorRamp(ramp);
                //QgsStyleV2.defaultStyle().addColorRamp('Cividis', ramp)
                QStringList list = myRend->listSchemeNames();
                myRend->setSourceColorRamp();
                vl->setRenderer(myRend);
                */
            }
            vl->blockSignals(false);

            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer

            tmp_layers = treeGroup->findLayers();
            int ind = tmp_layers.size()-1;
            tmp_layers[ind]->setItemVisibilityChecked(false);
        }
        STOP_TIMER(create_vector_layer_data_on_nodes);
    }
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_geometry(QString fname, QString layer_name, struct _ntw_geom * ntw_geom, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (ntw_geom != nullptr)
    {
        if (ntw_geom->nr_ntw == 0)
        {
            QString msg = QString("No Mesh1D geometry given on file: %1").arg(fname);
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Warning, true);
            return;
        }
        START_TIMERN(create_vector_layer_geometry);
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();

        bool layer_found = false;
        for (int i = 0; i < tmp_layers.length(); i++)
        {
            //QMessageBox::warning(0, tr("Message: create_vector_layer_geometry"), QString(tr("Layers in group by name: ")) + tmp_layers[i]->name() + QString(" Look for: ") + layer_name);
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
            if (ntw_geom->geom[0]->name.size() != 0)
            {
                lMyAttribField << QgsField("Geometry edge name", QMetaType::QString);
                nr_attrib_fields++;
            }
            if (ntw_geom->geom[0]->long_name.size() != 0)
            {
                lMyAttribField << QgsField("Geometry edge long name", QMetaType::QString);
                nr_attrib_fields++;
            }
            lMyAttribField << QgsField("Geometry Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Geometry Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            for (int i = 0; i < ntw_geom->nr_ntw; i++)
            {
                QgsFeatureList MyFeatures;
                MyFeatures.reserve(ntw_geom->geom[i]->count);
                for (int j = 0; j < ntw_geom->geom[i]->count; j++)
                {
                    // int nsig = long(log10(ntw_geom->geom[i]->nodes[j]->count)) + 1;
                    QVector<QgsPointXY> points(ntw_geom->geom[i]->nodes[j]->count);
                    for (int k = 0; k < ntw_geom->geom[i]->nodes[j]->count; k++)
                    {
                        double x1 = ntw_geom->geom[i]->nodes[j]->x[k];
                        double y1 = ntw_geom->geom[i]->nodes[j]->y[k];
                        points[k] = QgsPointXY(x1, y1);
                    }

                    QgsFeature MyFeature;
                    MyFeature.setGeometry(QgsGeometry::fromPolylineXY(points));

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    if (ntw_geom->geom[i]->name.size() != 0)
                    {
                        k++;
                        MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(ntw_geom->geom[i]->name[j]).trimmed()));
                    }
                    if (ntw_geom->geom[i]->long_name.size() != 0)
                    {
                        k++;
                        MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(ntw_geom->geom[i]->long_name[j]).trimmed()));
                    }
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));
                    MyFeatures.append(MyFeature);
                }
                dp_vl->addFeatures(MyFeatures);
                vl->commitChanges();
            }
            vl->serverProperties()->setTitle(layer_name + ": " + fname);

            if (layer_name == QString("Mesh1D geometry"))
            {
                QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
                line_marker->setWidth(0.5);
                line_marker->setColor(QColor(0, 0, 255));

                QgsSymbol * symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
                symbol->changeSymbolLayer(0, line_marker);

                //set up a renderer for the layer
                QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
                vl->setRenderer(mypRenderer);
            }
            vl->blockSignals(false);

            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        STOP_TIMER(create_vector_layer_geometry);
    }
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_edges(QString fname, QString layer_name, struct _feature * nodes, struct _edge * edges, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (nodes != nullptr && edges != nullptr)
    {
        START_TIMERN(create_vector_layer_edges);
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
            if (edges->edge_length.size() > 0)
            {
                lMyAttribField << QgsField("Edge length", QMetaType::Double);
                nr_attrib_fields++;
            }
            if (edges->name.size() > 0)
            {
                lMyAttribField << QgsField("Edge name", QMetaType::QString);
                nr_attrib_fields++;
            }
            if (edges->long_name.size() > 0)
            {
                lMyAttribField << QgsField("Edge long name", QMetaType::QString);
                nr_attrib_fields++;
            }
            lMyAttribField << QgsField("Edge Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Edge Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QVector<QVariant> attribute(nr_attrib_fields);
            QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QVector<QgsPointXY> point(2);
            QgsFeatureList MyFeatures;
            QgsFeature MyFeature;

            // int nsig = long(log10(edges->count)) + 1;
            bool msg_given = false;
            //START_TIMER(create_vector_layer_edges_add_features);
            for (int j = 0; j < edges->count; j++)
            {
                point[0] = QgsPointXY(nodes->x[edges->edge_nodes[j][0]], nodes->y[edges->edge_nodes[j][0]]);
                point[1] = QgsPointXY(nodes->x[edges->edge_nodes[j][1]], nodes->y[edges->edge_nodes[j][1]]);
                MyFeature.setGeometry(QgsGeometry::fromPolylineXY(point));

                int k = -1;
                if (edges->edge_length.size() > 0)
                {
                    ++k;
                    attribute[k] = edges->edge_length[j];
                    if (edges->edge_length[j] < 0)
                    {
                        if (!msg_given)
                        {
                            QString msg = QString("Some edge lengths are negative (due to an error in the orientation of the edge) \'%1\'.").arg(layer_name);
                            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Warning, true);
                            msg_given = true;
                        }
                    }
                }
                if (edges->name.size() > 0)
                {
                    ++k;
                    attribute[k] = QString("%1").arg(QString::fromStdString(edges->name[j]).trimmed());
                }
                if (edges->long_name.size() > 0)
                {
                    ++k;
                    attribute[k] = QString("%1").arg(QString::fromStdString(edges->long_name[j]).trimmed());
                }
                ++k;
                attribute[k] = QString("%1:0").arg(j);  // arg(j, nsig, 10, QLatin1Char('0')));
                ++k;
                attribute[k] = QString("%1:1").arg(j + 1);
                MyFeature.setAttributes(attribute);
                MyFeature.setValid(true);
                MyFeatures.append(MyFeature);
            }
            dp_vl->addFeatures(MyFeatures);
            vl->commitChanges();
            //STOP_TIMER(create_vector_layer_edges_add_features);
            vl->serverProperties()->setTitle(layer_name + ": " + fname);

            QgsSimpleLineSymbolLayer* line_marker = new QgsSimpleLineSymbolLayer();
            if (layer_name == QString("Mesh2D edges"))
            {
                line_marker->setWidth(0.05);
                line_marker->setColor(QColor(0, 0, 0));
            }
            else if (layer_name == QString("Mesh contact edges"))
            {
                line_marker->setWidth(0.5);
                line_marker->setColor(QColor(255, 153, 204));
            }

            QgsSymbol* symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
            symbol->changeSymbolLayer(0, line_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(symbol);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        STOP_TIMER(create_vector_layer_edges);
    }
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_observation_point(QString fname, QString layer_name, _location_type * obs_points, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (obs_points != NULL && obs_points->type == OBSERVATION_TYPE::OBS_POINT  || obs_points->type == OBSERVATION_TYPE::OBS_POLYLINE)
    {
        START_TIMERN(create_vector_layer_observation_point);
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        bool layer_found = false;
        for (int i = 0; i < tmp_layers.length(); i++)
        {
            //QMessageBox::warning(0, tr("Message: create_vector_layer_nodes"), QString(tr("Layers in group by name: ")) + tmp_layers[i]->name() + QString(" Look for: ") + layer_name);
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
            if (obs_points->location_long_name != nullptr)
            {
                lMyAttribField << QgsField(obs_points->location_long_name, QMetaType::QString);
                nr_attrib_fields++;
            }
            lMyAttribField << QgsField("Observation point Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Observation point Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsFeatureList MyFeatures;
            MyFeatures.reserve(obs_points->location.size());
            // int nsig = long(log10(obs_points->location.size())) + 1;
            for (int j = 0; j < obs_points->location.size(); j++)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Count: %1, %5\nCoord: (%2,%3)\nName : %4\nLong name: %5").arg(ntw_nodes->nodes[i]->count).arg(ntw_nodes->nodes[i]->x[j]).arg(ntw_nodes->nodes[i]->y[j]).arg(ntw_nodes->nodes[i]->id[j]).arg(ntw_nodes->nodes[i]->name[j]));

                QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(obs_points->location[j].x[0], obs_points->location[j].y[0]));
                QgsFeature MyFeature;
                MyFeature.setGeometry(MyPoints);

                MyFeature.initAttributes(nr_attrib_fields);
                int k = -1;
                if (obs_points->location[j].name != nullptr)
                {
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString(obs_points->location[j].name).trimmed()));
                }
                k++;
                MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                k++;
                MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));
                MyFeatures.append(MyFeature);
            }
            dp_vl->addFeatures(MyFeatures);
            vl->commitChanges();
            vl->serverProperties()->setTitle(layer_name + ": " + fname);

            QgsSymbol * marker = new QgsMarkerSymbol();
            if (std::string(obs_points->location_dim_name).find("lateral") != std::string::npos)
            {
                QgsSvgMarkerSymbolLayer * simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/lateral.svg"));
                simple_marker->setSize(2.7);
                simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Lateral point rotation")));
                marker->changeSymbolLayer(0, simple_marker);
            }
            else
            {
                QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
                simple_marker->setSize(4.0);
                simple_marker->setColor(QColor(1, 1, 1));  // 0,0,0 could have a special meaning
                simple_marker->setFillColor(QColor(255, 255, 255));
                simple_marker->setShape(Qgis::MarkerShape::Star);
                marker->changeSymbolLayer(0, simple_marker);
            }

            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
                                                                          //QgsCoordinateReferenceSystem crs = vl->crs();
                                                                          //QMessageBox::information(0, tr("Message: create_vector_layer_geometry"), QString("CRS layer: %1").arg(crs.authid()));
                                                                          // todo: Probeersel symbology adjustements
        }
        STOP_TIMER(create_vector_layer_observation_point);
    }
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_observation_polyline(QString fname, QString layer_name, _location_type * obs_points, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (obs_points != NULL && obs_points->type == OBSERVATION_TYPE::OBS_POLYLINE)
    {
        START_TIMERN(create_vector_layer_observation_polyline);
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        bool layer_found = false;
        for (int i = 0; i < tmp_layers.length(); i++)
        {
            //QMessageBox::warning(0, tr("Message: create_vector_layer_nodes"), QString(tr("Layers in group by name: ")) + tmp_layers[i]->name() + QString(" Look for: ") + layer_name);
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
            if (obs_points->location_long_name != nullptr)
            {
                lMyAttribField << QgsField(obs_points->location_long_name, QMetaType::QString);
                nr_attrib_fields++;
            }
            lMyAttribField << QgsField("Cross section Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Cross section Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QVector<QgsPointXY> point;
            QgsFeatureList MyFeatures;
            MyFeatures.reserve(obs_points->location.size());

            for (int i = 0; i < obs_points->location.size(); i++)
            {
                //for (int j = 0; j < obs_points->location[i]; j++)
                {
                    point.clear();
                    for (int m = 0; m < obs_points->location[i].x.size(); m++)
                    {
                        if (obs_points->location[i].x[m] != NC_FILL_DOUBLE)
                        {
                            double x1 = obs_points->location[i].x[m];
                            double y1 = obs_points->location[i].y[m];
                            point.append(QgsPointXY(x1, y1));
                        }
                    }

                    QgsFeature MyFeature;
                    MyFeature.setGeometry(QgsGeometry::fromPolylineXY(point));

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    if (obs_points->location[i].name != nullptr)
                    {
                        k++;
                        MyFeature.setAttribute(k, QString("%1").arg(QString(obs_points->location[i].name).trimmed()));
                    }
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(i));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(i + 1));
                    MyFeatures.append(MyFeature);
                }
            }
            dp_vl->addFeatures(MyFeatures);
            vl->commitChanges();
            vl->serverProperties()->setTitle(layer_name + ": " + fname);

            QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
            line_marker->setWidth(0.75);
            if (std::string(obs_points->location_dim_name).find("structures") != std::string::npos)
            {
                line_marker->setColor(QColor(255, 0, 0));
            }
            else
            {
                line_marker->setColor(QColor(255, 0, 255));
            }

            QgsSymbol * symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
            symbol->changeSymbolLayer(0, line_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        STOP_TIMER(create_vector_layer_observation_polyline);
    }
}
//------------------------------------------------------------------------------
// Create vector layer for the structures defined by the chainage on a branch, so it is a point
void HVL::create_vector_layer_1D_structure(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (prop_tree != nullptr)
    {
        START_TIMERN(create_vector_layer_1D_structure);
        long status = -1;
        std::string json_key = "data.structure.id";
        std::vector<std::string> id;
        status = prop_tree->get(json_key, id);

        json_key = "data.structure.branchid";
        std::vector<std::string> branch_name;
        status = prop_tree->get(json_key, branch_name);

        json_key = "data.structure.chainage";
        std::vector<double> chainage;
        status = prop_tree->get(json_key, chainage);

        if (branch_name.size() == 0)
        {
            STOP_TIMER(create_vector_layer_1D_structure);
            return;
        }

        json_key = "data.structure.type";
        std::vector<std::string> structure_type;
        status = prop_tree->get(json_key, structure_type);

        // Count the differen types of weirs
        int cnt_bridges = 0;
        int cnt_bridgepillars = 0;
        int cnt_culverts = 0;
        int cnt_dambreaks = 0;
        int cnt_compound_structures = 0;
        int cnt_extra_resistances = 0;
        int cnt_generalstructures = 0;
        int cnt_inverted_siphons = 0;
        int cnt_pumps = 0;
        int cnt_orifices = 0;
        int cnt_riverweirs = 0;
        int cnt_universalweirs = 0;
        int cnt_weirs = 0;
        for (int j = 0; j < id.size(); j++)
        {
            if (structure_type[j] == "bridge") { cnt_bridges += 1; }
            if (structure_type[j] == "bridgePillar") { cnt_bridgepillars += 1; }
            if (structure_type[j] == "compound") { cnt_compound_structures += 1; }
            if (structure_type[j] == "culvert") { cnt_culverts += 1; }
            if (structure_type[j] == "dambreak") { cnt_dambreaks += 1; }
            if (structure_type[j] == "extraresistance") { cnt_extra_resistances += 1; }
            if (structure_type[j] == "generalstructure") { cnt_generalstructures += 1; }
            if (structure_type[j] == "invertedSiphon") { cnt_inverted_siphons += 1; }
            if (structure_type[j] == "orifice") { cnt_orifices += 1; }
            if (structure_type[j] == "pump") { cnt_pumps += 1; }
            if (structure_type[j] == "riverWeir") { cnt_riverweirs += 1; }
            if (structure_type[j] == "universalWeir") { cnt_universalweirs += 1; }
            if (structure_type[j] == "weir") { cnt_weirs += 1; }
        }
        int total_cnt =
            cnt_bridges +
            cnt_bridgepillars +
            cnt_compound_structures +
            cnt_culverts +
            cnt_dambreaks +
            cnt_extra_resistances +
            cnt_generalstructures +
            cnt_inverted_siphons +
            cnt_orifices +
            cnt_pumps +
            cnt_riverweirs +
            cnt_universalweirs +
            cnt_weirs;

        if (id.size() != total_cnt)
        {
            QString fname = QString::fromStdString(prop_tree->get_filename());
            QString msg = QString(tr("Not all structure locations are supported.\nInvestigate file \"%1\".")).arg(fname);
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Warning, true);
        }
        //
        if (cnt_bridges > 0)
        {
            QgsLayerTreeGroup* mainTreeGroup;
            QgsLayerTreeGroup* subTreeGroup;
            mainTreeGroup = get_subgroup(treeGroup, QString("Area"));
            subTreeGroup = get_subgroup(mainTreeGroup, QString("Structures (1D)"));
            QString layer_name = QString("Bridge");

            // create the vector layer for structures
            QgsVectorLayer* vl;
            QgsVectorDataProvider* dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Bridge name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Bridge Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Bridge Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsPointXY janm;
            struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            for (int j = 0; j < id.size(); j++)
            {
                if (structure_type[j] == "bridge")
                {
                    status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
                    QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyPoints);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(id[j]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                    dp_vl->addFeature(MyFeature);
                }
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSvgMarkerSymbolLayer* simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/structure_bridge_1d.svg"));
            simple_marker->setSize(5.0);
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Bridge location")));

            QgsSymbol* marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
            for (int i = 0; i < tmp_layers.size(); i++)
            {
                if (tmp_layers[i]->name() == layer_name)
                {
                    tmp_layers[i]->setItemVisibilityChecked(false);
                }
            }
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        //
        if (cnt_bridgepillars > 0)
        {
            QgsLayerTreeGroup* mainTreeGroup;
            QgsLayerTreeGroup* subTreeGroup;
            mainTreeGroup = get_subgroup(treeGroup, QString("Area"));
            subTreeGroup = get_subgroup(mainTreeGroup, QString("Structures (1D)"));
            QString layer_name = QString("Bridge pillars");

            // create the vector layer for structures
            QgsVectorLayer* vl;
            QgsVectorDataProvider* dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Bridge pillars name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Bridge pillars Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Bridge pillars Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsPointXY janm;
            struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            for (int j = 0; j < id.size(); j++)
            {
                if (structure_type[j] == "bridgePillar")
                {
                    status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
                    QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyPoints);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(id[j]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                    dp_vl->addFeature(MyFeature);
                }
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSvgMarkerSymbolLayer* simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/structure_bridgepillar_1d.svg"));
            simple_marker->setSize(5.0);
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Bridge pillar location")));

            QgsSymbol* marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
            for (int i = 0; i < tmp_layers.size(); i++)
            {
                if (tmp_layers[i]->name() == layer_name)
                {
                    tmp_layers[i]->setItemVisibilityChecked(false);
                }
            }
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        //
        if (cnt_compound_structures > 0)
        {
            QgsLayerTreeGroup* mainTreeGroup;
            QgsLayerTreeGroup* subTreeGroup;
            mainTreeGroup = get_subgroup(treeGroup, QString("Area"));
            subTreeGroup = get_subgroup(mainTreeGroup, QString("Structures (1D)"));
            QString layer_name = QString("Compound structures");

            // create the vector layer for structures
            QgsVectorLayer* vl;
            QgsVectorDataProvider* dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Compound structures name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Compound structures Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Compound structures Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsPointXY janm;
            struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            std::vector<std::string> numstructures;
            json_key = "data.structure.numstructures";
            status = prop_tree->get(json_key, numstructures);
            std::vector<std::string> structure_ids;
            json_key = "data.structure.structureids";
            status = prop_tree->get(json_key, structure_ids);
            int i_cmp_struct = -1;
            int cnt = 0;
            int j = -1;
            int m = -1;
            bool do_while = true;
            while (do_while)
            {
                ++j;
                ++m;
                if (structure_type[j] == "dambreak")
                {
                    ++j;
                }
                if (j == id.size()) {
                    do_while = false;
                }
                if (structure_type[j] == "compound")
                {
                    i_cmp_struct += 1;
                    if (i_cmp_struct == 0)
                    {
                        cnt = 0;
                    }
                    else
                    {
                        cnt += atoi(numstructures[i_cmp_struct-1].c_str());
                    }
                    for (int k = 0; k < id.size(); ++k)
                    {
                        if (structure_ids[cnt] == id[k])
                        {
                            branch_name[m] = branch_name[k];
                            chainage.push_back(chainage[k]);
                            break;
                        }
                    }
                    status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[m], chainage[m], &xp, &yp, &rotation);
                    QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyPoints);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(id[j]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                    dp_vl->addFeature(MyFeature);
                }
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSvgMarkerSymbolLayer* simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/structure_compound_1d.svg"));
            simple_marker->setSize(5.0);
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Compound structure location")));

            QgsSymbol* marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
            for (int i = 0; i < tmp_layers.size(); i++)
            {
                if (tmp_layers[i]->name() == layer_name)
                {
                    tmp_layers[i]->setItemVisibilityChecked(false);
                }
            }
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        //
        if (cnt_culverts > 0)
        {
            QgsLayerTreeGroup* mainTreeGroup;
            QgsLayerTreeGroup* subTreeGroup;
            mainTreeGroup = get_subgroup(treeGroup, QString("Area"));
            subTreeGroup = get_subgroup(mainTreeGroup, QString("Structures (1D)"));
            QString layer_name = QString("Culvert");

            // create the vector layer for structures
            QgsVectorLayer* vl;
            QgsVectorDataProvider* dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Culvert name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Culvert Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Culvert Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsPointXY janm;
            struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            for (int j = 0; j < id.size(); j++)
            {
                if (structure_type[j] == "culvert")
                {
                    status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
                    QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyPoints);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(id[j]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                    dp_vl->addFeature(MyFeature);
                }
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSvgMarkerSymbolLayer* simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/structure_culvert_1d.svg"));
            simple_marker->setSize(5.0);
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Culvert location")));

            QgsSymbol* marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
            for (int i = 0; i < tmp_layers.size(); i++)
            {
                if (tmp_layers[i]->name() == layer_name)
                {
                    tmp_layers[i]->setItemVisibilityChecked(false);
                }
            }
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        //
        if (cnt_dambreaks > 0)
        {
        }
        //
        if (cnt_extra_resistances > 0)
        {
            QgsLayerTreeGroup* mainTreeGroup;
            QgsLayerTreeGroup* subTreeGroup;
            mainTreeGroup = get_subgroup(treeGroup, QString("Area"));
            subTreeGroup = get_subgroup(mainTreeGroup, QString("Structures (1D)"));
            QString layer_name = QString("Extra resistance");

            // create the vector layer for structures
            QgsVectorLayer* vl;
            QgsVectorDataProvider* dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Extra resistance name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Extra resistance Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Extra resistance Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsPointXY janm;
            struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            for (int j = 0; j < id.size(); j++)
            {
                if (structure_type[j] == "extraresistance")
                {
                    status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
                    QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyPoints);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(id[j]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                    dp_vl->addFeature(MyFeature);
                }
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSvgMarkerSymbolLayer* simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/structure_extra_resistance_1d.svg"));
            simple_marker->setSize(5.0);
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Extra resistance location")));

            QgsSymbol* marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
            for (int i = 0; i < tmp_layers.size(); i++)
            {
                if (tmp_layers[i]->name() == layer_name)
                {
                    tmp_layers[i]->setItemVisibilityChecked(false);
                }
            }
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        //
        if (cnt_generalstructures > 0)
        {
            QgsLayerTreeGroup* mainTreeGroup;
            QgsLayerTreeGroup* subTreeGroup;
            mainTreeGroup = get_subgroup(treeGroup, QString("Area"));
            subTreeGroup = get_subgroup(mainTreeGroup, QString("Structures (1D)"));
            QString layer_name = QString("General structure");

            // create the vector layer for structures
            QgsVectorLayer* vl;
            QgsVectorDataProvider* dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("General structure name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("General structure Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("General structure Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsPointXY janm;
            struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            for (int j = 0; j < id.size(); j++)
            {
                if (structure_type[j] == "generalstructure")
                {
                    status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
                    QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyPoints);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(id[j]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                    dp_vl->addFeature(MyFeature);
                }
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSvgMarkerSymbolLayer* simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/structure_general_structure_1d.svg"));
            simple_marker->setSize(5.0);
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("General structure location")));

            QgsSymbol* marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
            for (int i = 0; i < tmp_layers.size(); i++)
            {
                if (tmp_layers[i]->name() == layer_name)
                {
                    tmp_layers[i]->setItemVisibilityChecked(false);
                }
            }
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        //
        if (cnt_inverted_siphons > 0)
        {
            QgsLayerTreeGroup* mainTreeGroup;
            QgsLayerTreeGroup* subTreeGroup;
            mainTreeGroup = get_subgroup(treeGroup, QString("Area"));
            subTreeGroup = get_subgroup(mainTreeGroup, QString("Structures (1D)"));
            QString layer_name = QString("Inverted siphon");

            // create the vector layer for structures
            QgsVectorLayer* vl;
            QgsVectorDataProvider* dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Inverted siphon name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Inverted siphon Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Inverted siphon Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            for (int j = 0; j < id.size(); j++)
            {
                if (structure_type[j] == "invertedSiphon")
                {
                    status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
                    QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyPoints);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(id[j]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                    dp_vl->addFeature(MyFeature);
                }
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSvgMarkerSymbolLayer* simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/structure_inverted_siphon_1d.svg"));
            simple_marker->setSize(5.0);
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Inverted siphon location")));

            QgsSymbol* marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
            for (int i = 0; i < tmp_layers.size(); i++)
            {
                if (tmp_layers[i]->name() == layer_name)
                {
                    tmp_layers[i]->setItemVisibilityChecked(false);
                }
            }
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        //
        if (cnt_orifices > 0)
        {
            QgsLayerTreeGroup* mainTreeGroup;
            QgsLayerTreeGroup* subTreeGroup;
            mainTreeGroup = get_subgroup(treeGroup, QString("Area"));
            subTreeGroup = get_subgroup(mainTreeGroup, QString("Structures (1D)"));
            QString layer_name = QString("Orifice");

            // create the vector layer for structures
            QgsVectorLayer* vl;
            QgsVectorDataProvider* dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Orifice name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Orifice Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Orifice Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsPointXY janm;
            struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            for (int j = 0; j < id.size(); j++)
            {
                if (structure_type[j] == "orifice")
                {
                    status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
                    QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyPoints);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(id[j]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                    dp_vl->addFeature(MyFeature);
                }
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSvgMarkerSymbolLayer* simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/structure_orifice_1d.svg"));
            simple_marker->setSize(5.0);
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("River weir location")));

            QgsSymbol* marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
            for (int i = 0; i < tmp_layers.size(); i++)
            {
                if (tmp_layers[i]->name() == layer_name)
                {
                    tmp_layers[i]->setItemVisibilityChecked(false);
                }
            }
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        //
        if (cnt_pumps > 0)
        {
            QgsLayerTreeGroup* mainTreeGroup;
            QgsLayerTreeGroup* subTreeGroup;
            mainTreeGroup = get_subgroup(treeGroup, QString("Area"));
            subTreeGroup = get_subgroup(mainTreeGroup, QString("Structures (1D)"));
            QString layer_name = QString("Pump");

            // create the vector layer for structures
            QgsVectorLayer* vl;
            QgsVectorDataProvider* dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Pump name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Pump Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Pump Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsPointXY janm;
            struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            for (int j = 0; j < id.size(); j++)
            {
                if (structure_type[j] == "pump")
                {
                    status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
                    QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyPoints);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(id[j]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                    dp_vl->addFeature(MyFeature);
                }
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSvgMarkerSymbolLayer* simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/structure_pump_1d.svg"));
            simple_marker->setSize(5.0);
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Pump location")));

            QgsSymbol* marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
            for (int i = 0; i < tmp_layers.size(); i++)
            {
                if (tmp_layers[i]->name() == layer_name)
                {
                    tmp_layers[i]->setItemVisibilityChecked(false);
                }
            }
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        //
        if (cnt_riverweirs > 0)
        {
            QgsLayerTreeGroup* mainTreeGroup;
            QgsLayerTreeGroup* subTreeGroup;
            mainTreeGroup = get_subgroup(treeGroup, QString("Area"));
            subTreeGroup = get_subgroup(mainTreeGroup, QString("Structures (1D)"));
            QString layer_name = QString("River Weir");

            // create the vector layer for structures
            QgsVectorLayer* vl;
            QgsVectorDataProvider* dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("River weir name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("River weir Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("River weir Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsPointXY janm;
            struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            for (int j = 0; j < id.size(); j++)
            {
                if (structure_type[j] == "riverWeir")
                {
                    status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
                    QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyPoints);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(id[j]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                    dp_vl->addFeature(MyFeature);
                }
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSvgMarkerSymbolLayer* simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/structure_riverweir_1d.svg"));
            simple_marker->setSize(5.0);
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("River weir location")));

            QgsSymbol* marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
            for (int i = 0; i < tmp_layers.size(); i++)
            {
                if (tmp_layers[i]->name() == layer_name)
                {
                    tmp_layers[i]->setItemVisibilityChecked(false);
                }
            }
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        //
        if (cnt_universalweirs > 0)
        {
        }
        //
        if (cnt_weirs > 0)
        {
            QgsLayerTreeGroup* mainTreeGroup;
            QgsLayerTreeGroup* subTreeGroup;
            mainTreeGroup = get_subgroup(treeGroup, QString("Area"));
            subTreeGroup = get_subgroup(mainTreeGroup, QString("Structures (1D)"));
            QString layer_name = QString("Weir");

            // create the vector layer for structures
            QgsVectorLayer* vl;
            QgsVectorDataProvider* dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Weir name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Weir Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Weir Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsPointXY janm;
            struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            for (int j = 0; j < id.size(); j++)
            {
                if (structure_type[j] == "weir")
                {
                    status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
                    QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyPoints);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(id[j]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                    dp_vl->addFeature(MyFeature);
                }
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSvgMarkerSymbolLayer* simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/structure_weir_1d.svg"));
            simple_marker->setSize(5.0);
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Weir location")));

            QgsSymbol* marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
            for (int i = 0; i < tmp_layers.size(); i++)
            {
                if (tmp_layers[i]->name() == layer_name)
                {
                    tmp_layers[i]->setItemVisibilityChecked(false);
                }
            }
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
        STOP_TIMER(create_vector_layer_1D_structure);
    }  // end proptree != nullptr
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_observation_point(GRID * grid_file, JSON_READER * pt_structures, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (pt_structures != nullptr)
    {
        START_TIMERN(create_vector_layer_observation_point);
        long status = -1;
        std::string json_key = "data.general.fileversion";
        std::vector<std::string> version;
        status = pt_structures->get(json_key, version);
        if (version[0] == "1.00")
        {
            // observation point defined by coordinate reference systeem (crs)
            create_vector_layer_crs_observation_point(grid_file, pt_structures, epsg_code, treeGroup);
        }
        else if (version[0] == "2.00")
        {
            // observation point defined by chainage
            create_vector_layer_chainage_observation_point(grid_file, pt_structures, epsg_code, treeGroup);
        }
        STOP_TIMER(create_vector_layer_observation_point);
    }
}
//------------------------------------------------------------------------------
// observation point defined by coordinate reference systeem (crs)
void HVL::create_vector_layer_crs_observation_point(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    Q_UNUSED(grid_file);
    START_TIMERN(create_vector_layer_crs_observation_point);

    long status = -1;
    std::string json_key = "data.observationpoint.name";
    std::vector<std::string> obs_name;
    status = prop_tree->get(json_key, obs_name);
    if (obs_name.size() == 0)
    {
        QMessageBox::warning(0, tr("Message: create_1D_observation_point_vector_layer"), QString(tr("Number of observation points is zero. JSON data: ")) + QString::fromStdString(json_key));
        STOP_TIMER(create_vector_layer_crs_observation_point);
        return;
    }
    json_key = "data.observationpoint.x";
    std::vector<double> x;
    status = prop_tree->get(json_key, x);
    if (x.size() == 0)
    {
        QMessageBox::warning(0, tr("Message: create_vector_layer_observation_point"), QString(tr("Number of x-coordinates is zero. JSON data: ")) + QString::fromStdString(json_key));
        STOP_TIMER(create_vector_layer_crs_observation_point);
        return;
    }
    json_key = "data.observationpoint.y";
    std::vector<double> y;
    status = prop_tree->get(json_key, y);
    if (y.size() == 0)
    {
        QMessageBox::warning(0, tr("Message: create_1D_observation_point_vector_layer"), QString(tr("Number of y-coordinates is zero. JSON data: ")) + QString::fromStdString(json_key));
        STOP_TIMER(create_vector_layer_crs_observation_point);
        return;
    }
    QgsLayerTreeGroup * subTreeGroup;
    subTreeGroup = get_subgroup(treeGroup, QString("Area"));
    QString layer_name = QString("Observation points");

    // create the vector layer for observation point
    QgsVectorLayer * vl;
    QgsVectorDataProvider * dp_vl;
    QList <QgsField> lMyAttribField;

    int nr_attrib_fields = 3;

    lMyAttribField << QgsField("Observation point name", QMetaType::QString);
    lMyAttribField << QgsField("Observation point Id (0-based)", QMetaType::QString);
    lMyAttribField << QgsField("Observation point Id (1-based)", QMetaType::QString);

    QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
    vl = new QgsVectorLayer(uri, layer_name, "memory");
    vl->blockSignals(true);
    vl->startEditing();
    dp_vl = vl->dataProvider();
    dp_vl->addAttributes(lMyAttribField);
    vl->updatedFields();
    QgsFeatureList MyFeatures;
    MyFeatures.reserve(obs_name.size());

    for (int j = 0; j < obs_name.size(); j++)
    {
        QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(x[j], y[j]));
        QgsFeature MyFeature;
        MyFeature.setGeometry(MyPoints);

        MyFeature.initAttributes(nr_attrib_fields);
        int k = -1;
        k++;
        MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(obs_name[j]).trimmed()));
        k++;
        MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
        k++;
        MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));
        MyFeatures.append(MyFeature);
    }
    dp_vl->addFeatures(MyFeatures);
    vl->commitChanges();
    std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
    vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

    QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
    simple_marker->setSize(4.0);
    simple_marker->setColor(QColor(255, 0, 0));
    simple_marker->setFillColor(QColor(255, 255, 255));
    simple_marker->setShape(Qgis::MarkerShape::Star);

    QgsSymbol * marker = new QgsMarkerSymbol();
    marker->changeSymbolLayer(0, simple_marker);

    //set up a renderer for the layer
    QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
    vl->setRenderer(mypRenderer);
    vl->blockSignals(false);

    add_layer_to_group(vl, subTreeGroup);
    connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer

    STOP_TIMER(create_vector_layer_crs_observation_point);
}
//------------------------------------------------------------------------------
// observation point defined by chainage
void HVL::create_vector_layer_chainage_observation_point(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (prop_tree != nullptr)
    {
        START_TIMERN(create_vector_layer_chainage_observation_point);
        long status = -1;
        std::string json_key = "data.observationpoint.id";  // if "id" is not found try "name"
        std::vector<std::string> obs_name;
        status = prop_tree->get(json_key, obs_name);
        if (obs_name.size() == 0)
        {
            json_key = "data.observationpoint.name";
            status = prop_tree->get(json_key, obs_name);
            if (obs_name.size() == 0)
            {
                QMessageBox::warning(0, tr("Message: create_1D_observation_point_vector_layer"), QString(tr("Number of observation points is zero. JSON data: ")) + QString::fromStdString(json_key));
                STOP_TIMER(create_vector_layer_chainage_observation_point);
                return;
            }
        }
        json_key = "data.observationpoint.branchid";
        std::vector<std::string> branch_name;
        status = prop_tree->get(json_key, branch_name);
        if (branch_name.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_vector_layer_observation_point"), QString(tr("Number of branch names is zero. JSON data: ")) + QString::fromStdString(json_key));
            STOP_TIMER(create_vector_layer_chainage_observation_point);
            return;
        }
        json_key = "data.observationpoint.chainage";
        std::vector<double> chainage;
        status = prop_tree->get(json_key, chainage);
        if (chainage.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_1D_observation_point_vector_layer"), QString(tr("Number of chainages is zero. JSON data: ")) + QString::fromStdString(json_key));
            STOP_TIMER(create_vector_layer_chainage_observation_point);
            return;
        }
        if (obs_name.size() != branch_name.size() || branch_name.size() != chainage.size() || obs_name.size() != chainage.size())
        {
            QMessageBox::warning(0, tr("Message: create_1D_observation_point_vector_layer"), QString(tr("Inconsistent data set. JSON data: ")) + QString::fromStdString(json_key)
                + "\nObservation points: " + (int)obs_name.size()
                + "\nBranches: " + (int)branch_name.size()
                + "\nChainage: " + (int)chainage.size());
            STOP_TIMER(create_vector_layer_chainage_observation_point);
            return;
        }

        QgsLayerTreeGroup * subTreeGroup;
        subTreeGroup = get_subgroup(treeGroup, QString("Area"));
        QString layer_name = QString("Observation points");

        // create the vector layer for structures
        QgsVectorLayer * vl;
        QgsVectorDataProvider * dp_vl;
        QList <QgsField> lMyAttribField;

        int nr_attrib_fields = 0;
        lMyAttribField << QgsField("Observation point name", QMetaType::QString);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation point Id (0-based)", QMetaType::QString);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation point Id (1-based)", QMetaType::QString);
        nr_attrib_fields++;

        QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
        vl = new QgsVectorLayer(uri, layer_name, "memory");
        vl->blockSignals(true);
        vl->startEditing();
        dp_vl = vl->dataProvider();
        dp_vl->addAttributes(lMyAttribField);
        //dp_vl->createSpatialIndex();
        vl->updatedFields();
        QgsFeatureList MyFeatures;
        MyFeatures.reserve(obs_name.size());

        struct _ntw_geom * ntw_geom = grid_file->get_network_geometry();
        struct _ntw_edges * ntw_edges = grid_file->get_network_edges();

        double xp;
        double yp;
        double rotation;
        for (int j = 0; j < obs_name.size(); j++)
        {
            status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
            QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
            QgsFeature MyFeature;
            MyFeature.setGeometry(MyPoints);

            MyFeature.initAttributes(nr_attrib_fields);
            int k = -1;
            k++;
            MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(obs_name[j]).trimmed()));
            k++;
            MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
            k++;
            MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));
            MyFeatures.append(MyFeature);
        }
        dp_vl->addFeatures(MyFeatures);
        vl->commitChanges();
        std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
        vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

        QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
        simple_marker->setSize(4.0);
        simple_marker->setColor(QColor(255, 0, 0));
        simple_marker->setFillColor(QColor(255, 255, 255));
        simple_marker->setShape(Qgis::MarkerShape::Star);
        simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Observation point rotation")));

        QgsSymbol * marker = new QgsMarkerSymbol();
        marker->changeSymbolLayer(0, simple_marker);

        //set up a renderer for the layer
        QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
        vl->setRenderer(mypRenderer);
        vl->blockSignals(false);

        add_layer_to_group(vl, subTreeGroup);
        QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
        for (int i = 0; i < tmp_layers.size(); i++)
        {
            if (tmp_layers[i]->name() == layer_name)
            {
                tmp_layers[i]->setItemVisibilityChecked(false);
            }
        }
        connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer

        STOP_TIMER(create_vector_layer_chainage_observation_point);
    }
}
//------------------------------------------------------------------------------
// sample point (x, y,z) defined by coordinate reference systeem (crs)
void HVL::create_vector_layer_sample_point(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    Q_UNUSED(grid_file);
    START_TIMERN(create_vector_layer_sample_point);

    long status = -1;
    std::string json_key = "data.samplepoint.z";
    std::vector<double> z_value;
    status = prop_tree->get(json_key, z_value);
    if (z_value.size() == 0)
    {
        QMessageBox::warning(0, tr("Message: create_vector_layer_sample_point"), QString(tr("Number of sample points is zero. JSON data: ")) + QString::fromStdString(json_key));
        STOP_TIMER(create_vector_layer_sample_point);
        return;
    }
    json_key = "data.samplepoint.x";
    std::vector<double> x;
    std::vector<double> y;
    status = prop_tree->get(json_key, x);
    json_key = "data.samplepoint.y";
    status = prop_tree->get(json_key, y);

    QgsLayerTreeGroup * subTreeGroup;
    subTreeGroup = get_subgroup(treeGroup, QString("Area"));
    QString layer_name = QString("Sample points");

    // create the vector layer for observation point
    QgsVectorLayer * vl;
    QgsVectorDataProvider * dp_vl;
    QList <QgsField> lMyAttribField;

    int nr_attrib_fields = 3;

    lMyAttribField << QgsField("Sample point value", QMetaType::Double);
    lMyAttribField << QgsField("Sample point Id (0-based)", QMetaType::QString);
    lMyAttribField << QgsField("Sample point Id (1-based)", QMetaType::QString);

    QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
    vl = new QgsVectorLayer(uri, layer_name, "memory");
    vl->blockSignals(true);
    vl->startEditing();
    dp_vl = vl->dataProvider();
    dp_vl->addAttributes(lMyAttribField);
    vl->updatedFields();
    QgsFeatureList MyFeatures;
    MyFeatures.reserve(z_value.size());
    for (int j = 0; j < z_value.size(); j++)
    {
        QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(x[j], y[j]));
        QgsFeature MyFeature;
        MyFeature.setGeometry(MyPoints);

        MyFeature.initAttributes(nr_attrib_fields);
        int k = -1;
        k++;
        MyFeature.setAttribute(k, z_value[j]);
        k++;
        MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
        k++;
        MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));
        MyFeatures.append(MyFeature);
    }
    dp_vl->addFeatures(MyFeatures);
    vl->commitChanges();
    std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
    vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

    QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
    simple_marker->setSize(2.4);
    simple_marker->setColor(QColor(255, 0, 0));
    simple_marker->setFillColor(QColor(255, 255, 255));
    simple_marker->setShape(Qgis::MarkerShape::Circle);

    QgsSymbol * marker = new QgsMarkerSymbol();
    marker->changeSymbolLayer(0, simple_marker);

    //set up a renderer for the layer
    QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
    vl->setRenderer(mypRenderer);
    vl->blockSignals(false);

    add_layer_to_group(vl, subTreeGroup);
    connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer

    STOP_TIMER(create_vector_layer_sample_point);
}
//------------------------------------------------------------------------------
// Observation cross-section (D-Flow FM) filename given in mdu-file
void HVL::create_vector_layer_observation_cross_section(GRID * grid_file, JSON_READER *prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    START_TIMERN(create_vector_layer_observation_cross_section);

    int status = -1;
    std::vector<std::string> tmp_point_name;  // point object 1D simualtion
    std::vector<std::string> tmp_line_name;  // line object 2D simulation
    status = prop_tree->get("data.observationcrosssection.name", tmp_point_name);
    status = prop_tree->get("data.path.name", tmp_line_name);
    if (tmp_point_name.size() > 0)
    {
        create_vector_layer_1D_observation_cross_section(grid_file, prop_tree, epsg_code, treeGroup);
    }
    if (tmp_line_name.size() > 0)
    {
        create_vector_layer_2D_observation_cross_section(grid_file, prop_tree, epsg_code, treeGroup);
    }
    STOP_TIMER(create_vector_layer_observation_cross_section);
}
//------------------------------------------------------------------------------
// Observation cross-section (D-Flow 1D) filename given in mdu-file, point object when 1D simulation
void HVL::create_vector_layer_1D_observation_cross_section(GRID* grid_file, JSON_READER* prop_tree, long epsg_code, QgsLayerTreeGroup* treeGroup)
{
    if (prop_tree != nullptr)
    {
        START_TIMERN(create_vector_layer_1D_observation_cross_section);

        long status = -1;
        std::string json_key = "data.observationcrosssection.name";
        std::vector<std::string> names;
        status = prop_tree->get(json_key, names);
        json_key = "data.observationcrosssection.branchid";
        std::vector<std::string> branch_name;
        status = prop_tree->get(json_key, branch_name);
        if (branch_name.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_vector_layer_observation_point"), QString(tr("Number of branch names is zero. JSON data: ")) + QString::fromStdString(json_key));
            STOP_TIMER(create_vector_layer_1D_observation_cross_section);
            return;
        }
        json_key = "data.observationcrosssection.chainage";
        std::vector<double> chainage;
        status = prop_tree->get(json_key, chainage);
        if (chainage.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_1D_observation_point_vector_layer"), QString(tr("Number of chainages is zero. JSON data: ")) + QString::fromStdString(json_key));
            STOP_TIMER(create_vector_layer_1D_observation_cross_section);
            return;
        }
        if (names.size() != branch_name.size() || branch_name.size() != chainage.size() || names.size() != chainage.size())
        {
            QMessageBox::warning(0, tr("Message: create_1D_observation_point_vector_layer"), QString(tr("Inconsistent data set. JSON data: ")) + QString::fromStdString(json_key)
                + "\nObservation points: " + (int)names.size()
                + "\nBranches: " + (int)branch_name.size()
                + "\nChainage: " + (int)chainage.size());
            STOP_TIMER(create_vector_layer_1D_observation_cross_section);
            return;
        }

        QgsLayerTreeGroup* subTreeGroup;
        subTreeGroup = get_subgroup(treeGroup, QString("Area"));
        QString layer_name = QString("Observation Cross-sections points");

        // create the vector layer for structures
        QgsVectorLayer* vl;
        QgsVectorDataProvider* dp_vl;
        QList <QgsField> lMyAttribField;

        int nr_attrib_fields = 0;
        lMyAttribField << QgsField("Observation cross-sections name", QMetaType::QString);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation cross-sections point Id (0-based)", QMetaType::QString);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation cross-sections point Id (1-based)", QMetaType::QString);
        nr_attrib_fields++;

        QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
        vl = new QgsVectorLayer(uri, layer_name, "memory");
        vl->blockSignals(true);
        vl->startEditing();
        dp_vl = vl->dataProvider();
        dp_vl->addAttributes(lMyAttribField);
        //dp_vl->createSpatialIndex();
        vl->updatedFields();
        QgsFeatureList MyFeatures;
        MyFeatures.reserve(names.size());

        struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
        struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

        double xp;
        double yp;
        double rotation;
        for (int j = 0; j < names.size(); j++)
        {
            status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
            QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
            QgsFeature MyFeature;
            MyFeature.setGeometry(MyPoints);

            MyFeature.initAttributes(nr_attrib_fields);
            int k = -1;
            k++;
            MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(names[j]).trimmed()));
            k++;
            MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
            k++;
            MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));
            MyFeatures.append(MyFeature);
        }
        dp_vl->addFeatures(MyFeatures);
        vl->commitChanges();
        std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
        vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

        QgsSimpleMarkerSymbolLayer* simple_marker = new QgsSimpleMarkerSymbolLayer();
        simple_marker->setSize(2.5);
        simple_marker->setColor(QColor(1, 0, 0));
        simple_marker->setFillColor(QColor(255, 0, 255));
        simple_marker->setShape(Qgis::MarkerShape::Hexagon);

        QgsSymbol* marker = new QgsMarkerSymbol();
        marker->changeSymbolLayer(0, simple_marker);

        //set up a renderer for the layer
        QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
        vl->setRenderer(mypRenderer);
        vl->blockSignals(false);

        add_layer_to_group(vl, subTreeGroup);
        QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
        for (int i = 0; i < tmp_layers.size(); i++)
        {
            if (tmp_layers[i]->name() == layer_name)
            {
                tmp_layers[i]->setItemVisibilityChecked(false);
            }
        }
        connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        STOP_TIMER(create_vector_layer_1D_observation_cross_section);
    }
}
//------------------------------------------------------------------------------
// Observation cross-section (D-Flow FM) filename given in mdu-file, line object when 2D simulation
void HVL::create_vector_layer_2D_observation_cross_section(GRID* grid_file, JSON_READER* prop_tree, long epsg_code, QgsLayerTreeGroup* treeGroup)
{
    START_TIMERN(create_vector_layer_2D_observation_cross_section);

    int status = -1;
    std::vector<std::string> tmp_line_name;
    status = prop_tree->get("data.path.name", tmp_line_name);
    if (tmp_line_name.size() != 0)
    {
        QgsLayerTreeGroup* subTreeGroup;
        subTreeGroup = get_subgroup(treeGroup, QString("Area"));
        QString layer_name = QString("Observation cross-section");

        // create the vector
        QgsVectorLayer* vl;
        QgsVectorDataProvider* dp_vl;
        QList <QgsField> lMyAttribField;

        std::vector<std::string> line_name;
        std::vector<std::vector<std::vector<double>>> poly_lines;
        status = prop_tree->get("data.path.name", line_name);
        status = prop_tree->get("data.path.multiline", poly_lines);

        int nr_attrib_fields = 0;
        lMyAttribField << QgsField("Observation cross-section name", QMetaType::QString);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation cross-section Id (0-based)", QMetaType::QString);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation cross-section Id (1-based)", QMetaType::QString);
        nr_attrib_fields++;

        QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
        vl = new QgsVectorLayer(uri, layer_name, "memory");
        vl->blockSignals(true);
        vl->startEditing();
        dp_vl = vl->dataProvider();
        dp_vl->addAttributes(lMyAttribField);
        //dp_vl->createSpatialIndex();
        vl->updatedFields();

        QFileInfo g_file = grid_file->get_filename();
        QVector<QgsPointXY> point;
        QgsMultiPolylineXY lines;

        if (poly_lines.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_vector_layer_observation_cross_section"), QString(tr("JSON data: ")) + QString::fromStdString("data.path.name"));
            STOP_TIMER(create_vector_layer_2D_observation_cross_section);
            return;
        }

        QgsFeatureList MyFeatures;
        MyFeatures.reserve(poly_lines.size());
        for (int ii = 0; ii < poly_lines.size(); ii++)
        {
            point.clear();
            lines.clear();
            for (int j = 0; j < poly_lines[ii][0].size(); j++)  // number of x-coordinates
            {
                double x1 = poly_lines[ii][0][j];
                double y1 = poly_lines[ii][1][j];
                point.append(QgsPointXY(x1, y1));
            }
            lines.append(point);

            point.clear();
            int nr_points = poly_lines[ii][0].size();
            std::vector<double> chainage(nr_points);
            chainage[0] = 0.0;
            for (int j = 1; j < nr_points; j++)
            {
                double x1 = poly_lines[ii][0][j - 1];
                double y1 = poly_lines[ii][1][j - 1];
                double x2 = poly_lines[ii][0][j];
                double y2 = poly_lines[ii][1][j];

                chainage[j] = chainage[j - 1] + sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));  // Todo: HACK: this is just the euclidian distance
            }
            double chainage_point = 0.5 * chainage[nr_points - 1];
            double xp;
            double yp;
            for (int j = 1; j < nr_points; j++)
            {
                if (chainage_point <= chainage[j])
                {
                    double alpha = (chainage_point - chainage[j - 1]) / (chainage[j] - chainage[j - 1]);  // alpha is a weight coefficient
                    double x1 = poly_lines[ii][0][j - 1];
                    double y1 = poly_lines[ii][1][j - 1];
                    double x2 = poly_lines[ii][0][j];
                    double y2 = poly_lines[ii][1][j];

                    xp = x1 + alpha * (x2 - x1);
                    yp = y1 + alpha * (y2 - y1);
                    point.append(QgsPointXY(xp, yp));

                    // perpendicular on last found line part of the polyline
                    double dx = x2 - x1;
                    double dy = y2 - y1;

                    double gamma = 0.1 * chainage[nr_points - 1];
                    double vlen = sqrt(dx * dx + dy * dy);  // The "length" of the vector
                    x2 = xp + gamma * dy / vlen;
                    y2 = yp - gamma * dx / vlen;
                    point.append(QgsPointXY(x2, y2));
                    dx = x2 - xp;
                    dy = y2 - yp;
                    vlen = sqrt(dx * dx + dy * dy);  // The "length" of the vector
                    double beta = atan2(-dy, -dx);
                    double dxt = 0.25;
                    double dyt = -0.1;
                    point.append(QgsPointXY(x2 + vlen * (cos(beta) * dxt - sin(beta) * dyt), y2 + vlen * (sin(beta) * dxt + cos(beta) * dyt)));
                    dxt = 0.25;
                    dyt = 0.1;
                    point.append(QgsPointXY(x2 + vlen * (cos(beta) * dxt - sin(beta) * dyt), y2 + vlen * (sin(beta) * dxt + cos(beta) * dyt)));
                    point.append(QgsPointXY(x2, y2));
                    break;
                }
            }
            lines.append(point);

            QgsFeature MyFeature;
            MyFeature.setGeometry(QgsGeometry::fromMultiPolylineXY(lines));
            MyFeature.initAttributes(nr_attrib_fields);

            int k = -1;
            k++;
            MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(line_name[ii]).trimmed()));
            k++;
            MyFeature.setAttribute(k, QString("%1:0").arg(ii));  // arg(j, nsig, 10, QLatin1Char('0')));
            k++;
            MyFeature.setAttribute(k, QString("%1:1").arg(ii + 1));
            MyFeatures.append(MyFeature);
        }
        dp_vl->addFeatures(MyFeatures);
        vl->commitChanges();
        std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
        vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

        QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
        line_marker->setWidth(0.75);
        line_marker->setColor(QColor(255, 0, 255));

        QgsSymbol * symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
        symbol->changeSymbolLayer(0, line_marker);

        //set up a renderer for the layer
        QgsSingleSymbolRenderer * mypRenderer = new QgsSingleSymbolRenderer(symbol);
        vl->setRenderer(mypRenderer);
        vl->blockSignals(false);

        add_layer_to_group(vl, subTreeGroup);
        connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
    }
    STOP_TIMER(create_vector_layer_2D_observation_cross_section);
}
//------------------------------------------------------------------------------
// Structures (D-Flow FM) filename given in mdu-file
void HVL::create_vector_layer_structure(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    START_TIMERN(create_vector_layer_structure);

    int status = -1;
    std::vector<std::string> fname;
    std::string json_key = "data.structure.polylinefile";
    status = prop_tree->get(json_key, fname);
    if (fname.size() == 0)
    {
        json_key = "data.structure.branchid";  // structures are defined on a branch
        status = prop_tree->get(json_key, fname);
        if (fname.size() == 0)
        {
            QString qname = QString::fromStdString(prop_tree->get_filename());
            QString msg = QString(tr("Structure polylines are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
        }
        else
        {
            create_vector_layer_1D_structure(grid_file, prop_tree, epsg_code, treeGroup);
        }
    }
    else
    {
        QgsLayerTreeGroup * subTreeGroup;
        subTreeGroup = get_subgroup(treeGroup, QString("Area"));
        QString layer_name = QString("Structures");

        // create the vector
        QgsVectorLayer * vl;
        QgsVectorDataProvider * dp_vl;
        QList <QgsField> lMyAttribField;

        int nr_attrib_fields = 0;
        lMyAttribField << QgsField("Structure name", QMetaType::QString);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Structure Id (0-based)", QMetaType::QString);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Structure Id (1-based)", QMetaType::QString);
        nr_attrib_fields++;

        QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
        vl = new QgsVectorLayer(uri, layer_name, "memory");
        vl->blockSignals(true);
        vl->startEditing();
        dp_vl = vl->dataProvider();
        dp_vl->addAttributes(lMyAttribField);
        //dp_vl->createSpatialIndex();
        vl->updatedFields();

        QFileInfo ug_file = grid_file->get_filename();
        QVector<QgsPointXY> point;
        QgsMultiPolylineXY lines;
        QgsFeatureList MyFeatures;
        MyFeatures.reserve(fname.size());

        std::vector<std::string> line_name;
        std::vector<std::vector<std::vector<double>>> poly_lines;

        for (int i = 0; i < fname.size(); i++)
        {
            lines.clear();
            point.clear();
            poly_lines.clear();
            line_name.clear();
            QString filename = ug_file.absolutePath() + "/" + QString::fromStdString(fname[i]);
            JSON_READER * json_file = new JSON_READER(filename.toStdString());

            status = json_file->get("data.path.name", line_name);
            status = json_file->get("data.path.multiline", poly_lines);

            if (poly_lines.size() == 0)
            {
                QMessageBox::warning(0, tr("Message: create_vector_layer_1D_external_forcing"), QString(tr("JSON data: ")) + QString::fromStdString(json_key));
                STOP_TIMER(create_vector_layer_structure);
                return;
            }

            for (int ii = 0; ii < poly_lines.size(); ii++)
            {
                for (int j = 0; j < poly_lines[ii][0].size(); j++)  // number of x-coordinates
                {
                    double x1 = poly_lines[ii][0][j];
                    double y1 = poly_lines[ii][1][j];
                    point.append(QgsPointXY(x1, y1));
                }
                lines.append(point);

                point.clear();
                int nr_points = poly_lines[ii][0].size();
                std::vector<double> chainage(nr_points);
                chainage[0] = 0.0;
                for (int j = 1; j < nr_points; j++)
                {
                    double x1 = poly_lines[ii][0][j - 1];
                    double y1 = poly_lines[ii][1][j - 1];
                    double x2 = poly_lines[ii][0][j];
                    double y2 = poly_lines[ii][1][j];

                    chainage[j] = chainage[j - 1] + sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));  // Todo: HACK: this is just the euclidian distance
                }
                double chainage_point = 0.5 * chainage[nr_points - 1];
                double xp;
                double yp;
                for (int j = 1; j < nr_points; j++)
                {
                    if (chainage_point <= chainage[j])
                    {
                        double alpha = (chainage_point - chainage[j - 1]) / (chainage[j] - chainage[j - 1]);  // alpha is a weight coefficient
                        double x1 = poly_lines[ii][0][j - 1];
                        double y1 = poly_lines[ii][1][j - 1];
                        double x2 = poly_lines[ii][0][j];
                        double y2 = poly_lines[ii][1][j];

                        xp = x1 + alpha * (x2 - x1);
                        yp = y1 + alpha * (y2 - y1);
                        point.append(QgsPointXY(xp, yp));

                        // perpendicular on last found line part of the polyline
                        double dx = x2 - x1;
                        double dy = y2 - y1;

                        double gamma = 0.1 * chainage[nr_points - 1];
                        double vlen = sqrt(dx * dx + dy * dy);  // The "length" of the vector
                        x2 = xp + gamma * dy/vlen;
                        y2 = yp - gamma * dx/vlen;
                        point.append(QgsPointXY(x2, y2));
                        dx = x2 - xp;
                        dy = y2 - yp;
                        vlen = sqrt(dx * dx + dy * dy);  // The "length" of the vector
                        double beta = atan2(-dy, -dx);
                        double dxt = 0.25;
                        double dyt = -0.1;
                        point.append(QgsPointXY(x2 + vlen * (cos(beta) * dxt - sin(beta) * dyt), y2 + vlen * (sin(beta) * dxt + cos(beta) * dyt)));
                        dxt = 0.25;
                        dyt = 0.1;
                        point.append(QgsPointXY(x2 + vlen * (cos(beta) * dxt - sin(beta) * dyt), y2 + vlen * (sin(beta) * dxt + cos(beta) * dyt)));
                        point.append(QgsPointXY(x2, y2));
                        break;
                    }
                }
                lines.append(point);

                QgsFeature MyFeature;
                MyFeature.setGeometry(QgsGeometry::fromMultiPolylineXY(lines));
                MyFeature.initAttributes(nr_attrib_fields);

                int k = -1;
                k++;
                MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(line_name[ii]).trimmed()));
                k++;
                MyFeature.setAttribute(k, QString("%1:0").arg(i));  // arg(j, nsig, 10, QLatin1Char('0')));
                k++;
                MyFeature.setAttribute(k, QString("%1:1").arg(i + 1));
                MyFeatures.append(MyFeature);
            }
            dp_vl->addFeatures(MyFeatures);
            vl->commitChanges();
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(fname[i]));
        }

        QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
        line_marker->setWidth(0.75);
        line_marker->setColor(QColor(255, 0, 0));

        QgsSymbol * symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
        symbol->changeSymbolLayer(0, line_marker);

        //set up a renderer for the layer
        QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
        vl->setRenderer(mypRenderer);
        vl->blockSignals(false);

        add_layer_to_group(vl, subTreeGroup);
        connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
    }
    STOP_TIMER(create_vector_layer_structure);
}
//------------------------------------------------------------------------------
// DryPointsFile (Dryareas) (D-Flow FM) filename given in mdu-file
void HVL::create_vector_layer_drypoints(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    Q_UNUSED(grid_file);
    START_TIMERN(create_vector_layer_drypoints);

    int status = -1;
    std::vector<std::string> tmp_line_name;
    std::string json_key = "data.path.name";
    status = prop_tree->get(json_key, tmp_line_name);
    if (tmp_line_name.size() != 0)
    {
        QgsLayerTreeGroup * subTreeGroup;
        subTreeGroup = get_subgroup(treeGroup, QString("Area"));
        QString layer_name = "Dry points";
        // create the vector
        QgsVectorLayer * vl;
        QgsVectorDataProvider * dp_vl;
        QList <QgsField> lMyAttribField;

        std::vector<std::string> line_name;
        std::vector<std::vector<std::vector<double>>> poly_lines;
        status = prop_tree->get("data.path.name", line_name);
        status = prop_tree->get("data.path.multiline", poly_lines);

        int nr_attrib_fields = 0;
        QString attrib_label = layer_name + " name";
        lMyAttribField << QgsField(attrib_label, QMetaType::QString);
        nr_attrib_fields++;

        attrib_label = layer_name + " Id (0-based)";
        lMyAttribField << QgsField(attrib_label, QMetaType::QString);
        nr_attrib_fields++;

        attrib_label = layer_name + " Id (1-based)";
        lMyAttribField << QgsField(attrib_label, QMetaType::QString);
        nr_attrib_fields++;

        QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
        vl = new QgsVectorLayer(uri, layer_name, "memory");
        vl->blockSignals(true);
        vl->startEditing();
        dp_vl = vl->dataProvider();
        dp_vl->addAttributes(lMyAttribField);
        vl->updatedFields();

        if (poly_lines.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_vector_layer_1D_external_forcing"), QString(tr("JSON data: ")) + QString::fromStdString(json_key));
            STOP_TIMER(create_vector_layer_drypoints);
            return;
        }

        QgsFeatureList MyFeatures;
        MyFeatures.reserve(poly_lines.size());
        for (int ii = 0; ii < poly_lines.size(); ii++)
        {
            QVector<QgsPointXY> point(poly_lines[ii][0].size());
            for (int j = 0; j < poly_lines[ii][0].size(); j++)  // number of x-coordinates
            {
                double x1 = poly_lines[ii][0][j];
                double y1 = poly_lines[ii][1][j];
                point[j] = QgsPointXY(x1, y1);
            }

            QgsFeature MyFeature;
            MyFeature.setGeometry(QgsGeometry::fromPolylineXY(point));
            MyFeature.initAttributes(nr_attrib_fields);

            int k = -1;
            k++;
            MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(line_name[ii]).trimmed()));
            k++;
            MyFeature.setAttribute(k, QString("%1:0").arg(ii));  // arg(j, nsig, 10, QLatin1Char('0')));
            k++;
            MyFeature.setAttribute(k, QString("%1:1").arg(ii + 1));
            MyFeatures.append(MyFeature);
        }
        dp_vl->addFeatures(MyFeatures);
        vl->commitChanges();
        std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
        vl->serverProperties()->setTitle(layer_name + ": " +  QString::fromStdString(token[token.size() - 1]));

        QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
        line_marker->setWidth(0.75);
        line_marker->setColor(QColor(1, 1, 1));  // black

        QgsSymbol * symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
        symbol->changeSymbolLayer(0, line_marker);

        //set up a renderer for the layer
        QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
        vl->setRenderer(mypRenderer);
        vl->blockSignals(false);

        add_layer_to_group(vl, subTreeGroup);
        connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
    }
    STOP_TIMER(create_vector_layer_drypoints);
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_1D_external_forcing(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (prop_tree != nullptr)
    {
        START_TIMERN(create_vector_layer_1D_external_forcing);
        long status;
        std::string json_key;
        std::vector<std::string> fname;
        //-------------------------------------------------------------------------------------------
        status = -1;
        json_key = "data.sources_sinks.filename";
        status = prop_tree->get(json_key, fname);
        if (fname.size() == 0)
        {
            QString qname = QString::fromStdString(prop_tree->get_filename());
            QString msg = QString(tr("Sources and sinks are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
        }
        else
        {
            START_TIMER(data.sources_sinks.filename);
            QgsLayerTreeGroup * subTreeGroup;
            subTreeGroup = get_subgroup(treeGroup, QString("Area"));
            QString layer_name = QString("Sources and Sinks");

            // create the vector
            QgsVectorLayer* vl_lines;
            QgsVectorLayer* vl_points;
            QgsVectorDataProvider* dp_vl_lines;
            QgsVectorDataProvider* dp_vl_points;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Source Sink name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Source Sink Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Source Sink Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            //lines
            QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
            vl_lines = new QgsVectorLayer(uri, layer_name, "memory");
            vl_lines->blockSignals(true);
            vl_lines->startEditing();
            dp_vl_lines = vl_lines->dataProvider();
            dp_vl_lines->addAttributes(lMyAttribField);
            vl_lines->updatedFields();

            //points
            uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl_points = new QgsVectorLayer(uri, layer_name, "memory");
            vl_points->blockSignals(true);
            vl_points->startEditing();
            dp_vl_points = vl_points->dataProvider();
            dp_vl_points->addAttributes(lMyAttribField);
            vl_points->updatedFields();

            QFileInfo ug_file = grid_file->get_filename();

            std::vector<std::string> line_name;
            std::vector<std::vector<std::vector<double>>> poly_lines;
            std::vector<std::vector<std::vector<double>>> poly_points;

            for (int i = 0; i < fname.size(); i++)
            {
                bool processed = false;
                for (int j = 0; j < i; ++j)  // check if the fname[i] is already processed
                {
                    if (fname[i] == fname[j])
                    {
                        processed = true;
                    }
                }
                if (processed) { continue; }  // fname is already processed -> next fname

                poly_lines.clear();
                poly_points.clear();
                line_name.clear();
                QString filename = ug_file.absolutePath() + "/" + QString::fromStdString(fname[i]);
                JSON_READER * json_file = new JSON_READER(filename.toStdString());

                status = json_file->get("data.path.name", line_name);
                status = json_file->get("data.path.multiline", poly_lines);

                if (poly_lines.size() == 0)
                {
                    status = json_file->get("data.path.multipoint", poly_points);
                    if (poly_points.size() == 0)
                    {
                        QMessageBox::warning(0, tr("Message: create_vector_layer_1D_external_forcing"),
                            QString(tr("JSON data: ")) + QString::fromStdString(json_key) + "\nFile: " + QString::fromStdString(fname[i]));
                        STOP_TIMER(create_vector_layer_1D_external_forcing);
                        return;
                    }

                    QgsFeature MyFeature;
                    MyFeature.setGeometry(QgsGeometry::fromPointXY(QgsPointXY(poly_points[0][0][0], poly_points[0][1][0])));
                    MyFeature.initAttributes(nr_attrib_fields);

                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(line_name[0]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(i));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(i + 1));

                    dp_vl_points->addFeature(MyFeature);
                    vl_points->commitChanges();
                }
                else
                {
                    for (int ii = 0; ii < poly_lines.size(); ii++)
                    {
                        QVector<QgsPointXY> point(poly_lines[ii][0].size());
                        for (int j = 0; j < poly_lines[ii][0].size(); j++)  // number of x-coordinates
                        {
                            double x1 = poly_lines[ii][0][j];
                            double y1 = poly_lines[ii][1][j];
                            point[j] = QgsPointXY(x1, y1);
                        }

                        QgsFeature MyFeature;
                        MyFeature.setGeometry(QgsGeometry::fromPolylineXY(point));
                        MyFeature.initAttributes(nr_attrib_fields);

                        int k = -1;
                        k++;
                        MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(line_name[ii]).trimmed()));
                        k++;
                        MyFeature.setAttribute(k, QString("%1:0").arg(i));  // arg(j, nsig, 10, QLatin1Char('0')));
                        k++;
                        MyFeature.setAttribute(k, QString("%1:1").arg(i + 1));

                        dp_vl_lines->addFeature(MyFeature);
                        vl_lines->commitChanges();
                    }
                }
            }
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            if (vl_points->featureCount() != 0)
            {
                vl_points->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

                QgsSimpleMarkerSymbolLayer* simple_marker = new QgsSimpleMarkerSymbolLayer();
                simple_marker->setStrokeStyle(Qt::NoPen);
                simple_marker->setColor(QColor(255, 165, 0));  //  orange

                QgsSymbol* marker = QgsSymbol::defaultSymbol(Qgis::GeometryType::Point);
                marker->changeSymbolLayer(0, simple_marker);
                QgsSingleSymbolRenderer* myPointRenderer = new QgsSingleSymbolRenderer(marker);
                vl_points->setRenderer(myPointRenderer);
                vl_points->blockSignals(false);

                add_layer_to_group(vl_points, subTreeGroup);
                connect(vl_points, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
            }
            if (vl_lines->featureCount() != 0)
            {
                vl_lines->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));
                QgsSimpleLineSymbolLayer* line_marker = new QgsSimpleLineSymbolLayer();
                line_marker->setWidth(0.75);
                line_marker->setColor(QColor(255, 165, 0));  //  orange

                QgsSymbol* symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
                symbol->changeSymbolLayer(0, line_marker);
                QgsSingleSymbolRenderer* myLineRenderer = new QgsSingleSymbolRenderer(symbol);
                vl_lines->setRenderer(myLineRenderer);
                vl_lines->blockSignals(false);

                add_layer_to_group(vl_lines, subTreeGroup);
                connect(vl_lines, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
            }
            STOP_TIMER(data.sources_sinks.filename);
        }
        //-------------------------------------------------------------------------------------------
        status = -1;
        fname.clear();
        json_key = "data.boundary.locationfile";
        status = prop_tree->get(json_key, fname);
        if (fname.size() == 0)
        {
            std::string json_key_old = "data.boundary.filename";
            status = prop_tree->get(json_key_old, fname);
            if (fname.size() == 0)
            {
                QString qname = QString::fromStdString(prop_tree->get_filename());
                QString msg = QString(tr("Boundary polylines are skipped.\nTag \"%1\" or \"%2\" does not exist in file \"%3\".").arg(QString::fromStdString(json_key)).arg(QString::fromStdString(json_key_old)).arg(qname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
        }
        if (fname.size() != 0)
        {
            START_TIMER(data.boundary.locationfile);
            QgsLayerTreeGroup * subTreeGroup;
            subTreeGroup = get_subgroup(treeGroup, QString("Area"));
            QString layer_name = QString("Boundary");

            // create the vector
            QgsVectorLayer * vl;
            QgsVectorDataProvider * dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Boundary name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Boundary Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Boundary Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QFileInfo ug_file = grid_file->get_filename();
            QgsMultiPolylineXY lines;

            std::vector<std::string> line_name;
            std::vector<std::vector<std::vector<double>>> poly_lines;

            for (int i = 0; i < fname.size(); i++)
            {
                bool processed = false;
                for (int j = 0; j < i; ++j)  // check if the fname[i] is already processed
                {
                    if (fname[i] == fname[j])
                    {
                        processed = true;
                    }
                }
                if (processed) { continue; }  // fname is already processed -> next fname

                poly_lines.clear();
                line_name.clear();
                QString filename = ug_file.absolutePath() + "/" + QString::fromStdString(fname[i]);
                JSON_READER * json_file = new JSON_READER(filename.toStdString());

                status = json_file->get("data.path.name", line_name);
                status = json_file->get("data.path.multiline", poly_lines);

                if (poly_lines.size() == 0)
                {
                    QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"),
                        QString(tr("JSON data: ")) + QString::fromStdString(json_key) + "\nFile: " + QString::fromStdString(fname[i]));
                    STOP_TIMER(data.boundary.locationfile);
                    return;
                }

                for (int ii = 0; ii < poly_lines.size(); ii++)
                {
                    QVector<QgsPointXY> point(poly_lines[ii][0].size());
                    for (int j = 0; j < poly_lines[ii][0].size(); j++)  // number of x-coordinates
                    {
                        double x1 = poly_lines[ii][0][j];
                        double y1 = poly_lines[ii][1][j];
                        point[j] = QgsPointXY(x1, y1);
                    }

                    QgsFeature MyFeature;
                    MyFeature.setGeometry(QgsGeometry::fromPolylineXY(point));
                    MyFeature.initAttributes(nr_attrib_fields);

                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(line_name[0]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(i));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(i + 1));

                    dp_vl->addFeature(MyFeature);
                    vl->commitChanges();
                }
            }
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
            line_marker->setWidth(0.75);
            line_marker->setColor(QColor(0, 204, 0));

            QgsSymbol * symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
            symbol->changeSymbolLayer(0, line_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
            STOP_TIMER(data.boundary.locationfile);
        }
        //-------------------------------------------------------------------------------------------
        status = -1;
        fname.clear();
        json_key = "data.lateral.locationfile";
        status = prop_tree->get(json_key, fname);
        if (fname.size() == 0)
        {
            QString qname = QString::fromStdString(prop_tree->get_filename());
            QString msg = QString(tr("Lateral areas are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
        }
        else
        {
            START_TIMER(data.lateral.locationfile);
            QgsLayerTreeGroup * subTreeGroup;
            subTreeGroup = get_subgroup(treeGroup, QString("Area"));
            QString layer_name = QString("Lateral area");

            // create the vector
            QgsVectorLayer * vl;
            QgsVectorDataProvider * dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Lateral area name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Lateral area Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Lateral area Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QFileInfo ug_file = grid_file->get_filename();
            QgsMultiLineString * polylines = new QgsMultiLineString();

            std::vector<std::string> line_name;
            std::vector<std::vector<std::vector<double>>> poly_lines;

            for (int i = 0; i < fname.size(); i++)
            {
                bool processed = false;
                for (int j = 0; j < i; ++j)  // check if the fname[i] is already processed
                {
                    if (fname[i] == fname[j])
                    {
                        processed = true;
                    }
                }
                if (processed) { continue; }  // fname is already processed -> next fname

                poly_lines.clear();
                line_name.clear();
                QString filename = ug_file.absolutePath() + "/" + QString::fromStdString(fname[i]);
                JSON_READER * json_file = new JSON_READER(filename.toStdString());

                status = json_file->get("data.path.name", line_name);
                status = json_file->get("data.path.multiline", poly_lines);

                if (poly_lines.size() == 0)
                {
                    QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"),
                        QString(tr("JSON data: ")) + QString::fromStdString(json_key) + "\nFile: " + QString::fromStdString(fname[i]));
                    STOP_TIMER(data.lateral.locationfile);
                    return;
                }

                for (int ii = 0; ii < poly_lines.size(); ii++)
                {
                    QVector<QgsPointXY> point(poly_lines[ii][0].size());
                    for (int j = 0; j < poly_lines[ii][0].size(); j++)  // number of x-coordinates
                    {
                        double x1 = poly_lines[ii][0][j];
                        double y1 = poly_lines[ii][1][j];
                        point[j] = QgsPointXY(x1, y1);
                    }

                    QgsFeature MyFeature;
                    MyFeature.setGeometry(QgsGeometry::fromPolylineXY(point));
                    MyFeature.initAttributes(nr_attrib_fields);

                    int k = -1;
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(line_name[0]).trimmed()));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:0").arg(i));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(k, QString("%1:1").arg(i + 1));

                    dp_vl->addFeature(MyFeature);
                    vl->commitChanges();
                }
            }
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
            line_marker->setWidth(0.75);
            line_marker->setColor(QColor(0, 200, 255));

            QgsSymbol * symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
            symbol->changeSymbolLayer(0, line_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
            STOP_TIMER(data.lateral.locationfile);
        }
 //------------------------------------------------------------------------------
        status = -1;
        json_key = "data.lateral.id";
        std::vector<std::string> lateral_name;
        status = prop_tree->get(json_key, lateral_name);
        if (lateral_name.size() == 0)
        {
            QString qname = QString::fromStdString(prop_tree->get_filename());
            QString msg = QString(tr("Lateral discharges are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
        }
        else
        {
            START_TIMER(data.lateral.id);
            json_key = "data.lateral.branchid";
            std::vector<std::string> branch_name;
            status = prop_tree->get(json_key, branch_name);
            if (branch_name.size() == 0)
            {
                QString name = QString::fromStdString(prop_tree->get_filename());
                QString msg = QString(tr("Lateral discharges are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(name));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
                STOP_TIMER(data.lateral.id);
                return;
            }
            json_key = "data.lateral.chainage";
            std::vector<double> chainage;
            status = prop_tree->get(json_key, chainage);
            if (chainage.size() == 0)
            {
                QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"), QString(tr("Number of chainages is zero. JSON data: ")) + QString::fromStdString(json_key));
                STOP_TIMER(data.lateral.id);
                return;
            }
            if (lateral_name.size() != branch_name.size() || branch_name.size() != chainage.size() || lateral_name.size() != chainage.size())
            {
                QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"), QString(tr("Inconsistent data set. JSON data: ")) + QString::fromStdString(json_key)
                    + "\nLateral points: " + (int)lateral_name.size()
                    + "\nBranches: " + (int)branch_name.size()
                    + "\nChainage: " + (int)chainage.size());
                STOP_TIMER(data.lateral.id);
                return;
            }

            QgsLayerTreeGroup * subTreeGroup;
            subTreeGroup = get_subgroup(treeGroup, QString("Area"));
            QString layer_name = QString("Lateral discharge");

            // create the vector
            QgsVectorLayer * vl;
            QgsVectorDataProvider * dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Lateral point name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Lateral point rotation", QMetaType::Double);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Lateral point Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Lateral point Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            struct _ntw_geom * ntw_geom = grid_file->get_network_geometry();
            struct _ntw_edges * ntw_edges = grid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            for (int j = 0; j < lateral_name.size(); j++)
            {
                status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
                QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                QgsFeature MyFeature;
                MyFeature.setGeometry(MyPoints);

                MyFeature.initAttributes(nr_attrib_fields);
                int k = -1;
                k++;
                MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(lateral_name[j]).trimmed()));
                k++;
                MyFeature.setAttribute(k, QString("%1").arg(rotation));
                k++;
                MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                k++;
                MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                dp_vl->addFeature(MyFeature);
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSvgMarkerSymbolLayer * simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/lateral.svg"));
            simple_marker->setSize(2.7);
            //simple_marker->setColor(QColor(0, 255, 0));
            //simple_marker->setFillColor(QColor(0, 255, 0));
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Lateral point rotation")));

            QgsSymbol * marker = new QgsMarkerSymbol();
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
            for (int i = 0; i < tmp_layers.size(); i++)
            {
                if (tmp_layers[i]->name() == layer_name)
                {
                    tmp_layers[i]->setItemVisibilityChecked(false);
                }
            }
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
            STOP_TIMER(data.lateral.id);
     }
//------------------------------------------------------------------------------
        status = -1;
        json_key = "data.boundary.nodeid";
        std::vector<std::string> bnd_name;
        status = prop_tree->get(json_key, bnd_name);
        if (bnd_name.size() == 0)
        {
            QString qname = QString::fromStdString(prop_tree->get_filename());
            QString msg = QString(tr("Boundary nodes are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
        }
        else
        {
            START_TIMER(data.boundary.nodeid);
            QgsLayerTreeGroup * subTreeGroup;
            subTreeGroup = get_subgroup(treeGroup, QString("Area"));
            QString layer_name = QString("Boundary nodes");

            // create the vector
            QgsVectorLayer * vl;
            QgsVectorDataProvider * dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 0;
            lMyAttribField << QgsField("Boundary name", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Boundary point Id (0-based)", QMetaType::QString);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Boundary point Id (1-based)", QMetaType::QString);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            struct _ntw_nodes * ntw_nodes = grid_file->get_connection_nodes();

            double xp;
            double yp;
            for (int j = 0; j < bnd_name.size(); j++)
            {
                status = find_location_boundary(ntw_nodes, bnd_name[j], &xp, &yp);

                QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                QgsFeature MyFeature;
                MyFeature.setGeometry(MyPoints);

                MyFeature.initAttributes(nr_attrib_fields);
                int k = -1;
                k++;
                MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(bnd_name[j]).trimmed()));
                k++;
                MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                k++;
                MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

                dp_vl->addFeature(MyFeature);
            }
            vl->commitChanges();
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
            //QgsSvgMarkerSymbolLayer * simple_marker = new QgsSvgMarkerSymbolLayer(QString("d:/checkouts/git/qgis_plugins/qgis_umesh/svg/tmp_bridge_tui.svg"));
            simple_marker->setSize(4.0);
            simple_marker->setColor(QColor(0, 204, 0));
            simple_marker->setFillColor(QColor(0, 204, 0));
            simple_marker->setShape(Qgis::MarkerShape::Diamond);

            QgsSymbol * marker = new QgsMarkerSymbol();
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            add_layer_to_group(vl, subTreeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
            STOP_TIMER(data.boundary.nodeid);
        }
        json_key = "data.coefficient.filename";
        status = prop_tree->get(json_key, fname);
        if (fname.size() >= 1)
        {
            QString qname = QString::fromStdString(prop_tree->get_filename());
            QString msg = QString(tr("Coefficients are skipped.\nTag \"%1\" in not supported for file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
        }
        STOP_TIMER(create_vector_layer_1D_external_forcing)
    }
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_thin_dams(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    int status = -1;
    std::vector<std::string> tmp_line_name;
    std::string json_key = "data.path.name";
    status = prop_tree->get(json_key, tmp_line_name);
    if (tmp_line_name.size() != 0)
    {
        START_TIMERN(create_vector_layer_thin_dams);
        QgsLayerTreeGroup * subTreeGroup;
        subTreeGroup = get_subgroup(treeGroup, QString("Area"));
        QString layer_name = "Thin dams";

        // create the vector
        QgsVectorLayer * vl;
        QgsVectorDataProvider * dp_vl;
        QList <QgsField> lMyAttribField;

        std::vector<std::string> line_name;
        std::vector<std::vector<std::vector<double>>> poly_lines;
        status = prop_tree->get("data.path.name", line_name);
        status = prop_tree->get("data.path.multiline", poly_lines);

        int nr_attrib_fields = 0;
        QString attrib_label = layer_name + " name";
        lMyAttribField << QgsField(attrib_label, QMetaType::QString);
        nr_attrib_fields++;

        attrib_label = layer_name + " Id (0-based)";
        lMyAttribField << QgsField(attrib_label, QMetaType::QString);
        nr_attrib_fields++;

        attrib_label = layer_name + " Id (1-based)";
        lMyAttribField << QgsField(attrib_label, QMetaType::QString);
        nr_attrib_fields++;

        QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
        vl = new QgsVectorLayer(uri, layer_name, "memory");
        vl->blockSignals(true);
        vl->startEditing();
        dp_vl = vl->dataProvider();
        dp_vl->addAttributes(lMyAttribField);
        vl->updatedFields();

        QFileInfo ug_file = grid_file->get_filename();
        QgsMultiLineString * polylines = new QgsMultiLineString();

        if (poly_lines.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"), QString(tr("JSON data: ")) + QString::fromStdString(json_key));
            STOP_TIMER(create_vector_layer_thin_dams);
            return;
        }

        for (int ii = 0; ii < poly_lines.size(); ii++)
        {
            QVector<QgsPointXY> point(poly_lines[ii][0].size());
            for (int j = 0; j < poly_lines[ii][0].size(); j++)  // number of x-coordinates
            {
                double x1 = poly_lines[ii][0][j];
                double y1 = poly_lines[ii][1][j];
                point[j] = QgsPointXY(x1, y1);
            }

            QgsFeature MyFeature;
            MyFeature.setGeometry(QgsGeometry::fromPolylineXY(point));
            MyFeature.initAttributes(nr_attrib_fields);

            int k = -1;
            k++;
            MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(line_name[ii]).trimmed()));
            k++;
            MyFeature.setAttribute(k, QString("%1:0").arg(ii));  // arg(j, nsig, 10, QLatin1Char('0')));
            k++;
            MyFeature.setAttribute(k, QString("%1:1").arg(ii + 1));

            dp_vl->addFeature(MyFeature);
            vl->commitChanges();
        }
        std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
        vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

        QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
        line_marker->setWidth(0.75);
        line_marker->setColor(QColor(1, 1, 1));  // black

        QgsSymbol * symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
        symbol->changeSymbolLayer(0, line_marker);

        //set up a renderer for the layer
        QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
        vl->setRenderer(mypRenderer);
        vl->blockSignals(false);

        add_layer_to_group(vl, subTreeGroup);
        connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        STOP_TIMER(create_vector_layer_thin_dams)
    }
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_fixed_weir(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    int status = -1;
    std::vector<std::string> tmp_line_name;
    std::string json_key = "data.path.name";
    status = prop_tree->get(json_key, tmp_line_name);
    if (tmp_line_name.size() != 0)
    {
        START_TIMERN(create_vector_layer_fixed_weir);
        QgsLayerTreeGroup* mainTreeGroup;
        QgsLayerTreeGroup* subTreeGroup;
        mainTreeGroup = get_subgroup(treeGroup, QString("Area"));
        subTreeGroup = get_subgroup(mainTreeGroup, QString("Fixed weirs"));
        std::filesystem::path fname = prop_tree->get_filename();
        std::string stem = fname.stem().generic_string();
        QString layer_name = QString::fromStdString(stem);
        // create the vector
        QgsVectorLayer * vl;
        QgsVectorDataProvider * dp_vl;
        QList <QgsField> lMyAttribField;

        std::vector<std::string> line_name;
        std::vector<std::vector<std::vector<double>>> poly_lines;
        status = prop_tree->get("data.path.name", line_name);
        status = prop_tree->get("data.path.multiline", poly_lines);

        int nr_attrib_fields = 0;
        QString attrib_label = layer_name + " name";
        lMyAttribField << QgsField(attrib_label, QMetaType::QString);
        nr_attrib_fields++;

        attrib_label = layer_name + " Id (0-based)";
        lMyAttribField << QgsField(attrib_label, QMetaType::QString);
        nr_attrib_fields++;

        attrib_label = layer_name + " Id (1-based)";
        lMyAttribField << QgsField(attrib_label, QMetaType::QString);
        nr_attrib_fields++;

        QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
        vl = new QgsVectorLayer(uri, layer_name, "memory");
        vl->blockSignals(true);
        vl->startEditing();
        dp_vl = vl->dataProvider();
        dp_vl->addAttributes(lMyAttribField);
        vl->updatedFields();

        QFileInfo ug_file = grid_file->get_filename();

        if (poly_lines.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"), QString(tr("JSON data: ")) + QString::fromStdString(json_key));
            STOP_TIMER(create_vector_layer_fixed_weir);
            return;
        }

        for (int ii = 0; ii < poly_lines.size(); ii++)
        {
            QVector<QgsPointXY> point(poly_lines[ii][0].size());
            for (int j = 0; j < poly_lines[ii][0].size(); j++)  // number of x-coordinates
            {
                double x1 = poly_lines[ii][0][j];
                double y1 = poly_lines[ii][1][j];
                point[j] = QgsPointXY(x1, y1);
            }

            QgsFeature MyFeature;
            MyFeature.setGeometry(QgsGeometry::fromPolylineXY(point));
            MyFeature.initAttributes(nr_attrib_fields);

            int k = -1;
            k++;
            MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(line_name[ii]).trimmed()));
            k++;
            MyFeature.setAttribute(k, QString("%1:0").arg(ii));  // arg(j, nsig, 10, QLatin1Char('0')));
            k++;
            MyFeature.setAttribute(k, QString("%1:1").arg(ii + 1));

            dp_vl->addFeature(MyFeature);
            vl->commitChanges();
        }
        std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
        vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

        QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
        line_marker->setWidth(0.75);
        line_marker->setColor(QColor(128, 0, 128));

        QgsSymbol * symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
        symbol->changeSymbolLayer(0, line_marker);

        //set up a renderer for the layer
        QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
        vl->setRenderer(mypRenderer);
        vl->blockSignals(false);

        add_layer_to_group(vl, subTreeGroup);
        connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        STOP_TIMER(create_vector_layer_fixed_weir);
    }
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_1D_cross_section(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    long status = -1;

    std::string json_key = "data.crosssection.id";
    std::vector<std::string> crosssection_name;
    status = prop_tree->get(json_key, crosssection_name);
    if (crosssection_name.size() == 0)
    {
        QString fname = QString::fromStdString(prop_tree->get_filename());
        QString msg = QString(tr("Cross-section locations are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(fname));
        QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
    }
    else
    {
        START_TIMERN(create_vector_layer_1D_cross_section);
        json_key = "data.crosssection.branchid";
        std::vector<std::string> branch_name;
        status = prop_tree->get(json_key, branch_name);
        if (branch_name.size() == 0)
        {
            QString fname = QString::fromStdString(prop_tree->get_filename());
            QString msg = QString(tr("Cross-section are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(fname));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            STOP_TIMER(create_vector_layer_1D_cross_section);
            return;
        }
        json_key = "data.crosssection.chainage";
        std::vector<double> chainage;
        status = prop_tree->get(json_key, chainage);
        if (chainage.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"), QString(tr("Number of chainages is zero. JSON data: ")) + QString::fromStdString(json_key));
            STOP_TIMER(create_vector_layer_1D_cross_section);
            return;
        }
        if (crosssection_name.size() != branch_name.size() || branch_name.size() != chainage.size() || crosssection_name.size() != chainage.size())
        {
            QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"), QString(tr("Inconsistent data set. JSON data: ")) + QString::fromStdString(json_key)
                + "\nCross-section names: " + (int)crosssection_name.size()
                + "\nBranches: " + (int)branch_name.size()
                + "\nChainage: " + (int)chainage.size());
            STOP_TIMER(create_vector_layer_1D_cross_section);
            return;
        }

        QgsLayerTreeGroup * subTreeGroup;
        subTreeGroup = get_subgroup(treeGroup, QString("Area"));
        QString layer_name = QString("Cross-section location");

        // create the vector
        QgsVectorLayer * vl;
        QgsVectorDataProvider * dp_vl;
        QList <QgsField> lMyAttribField;

        int nr_attrib_fields = 0;
        lMyAttribField << QgsField("Cross-section name", QMetaType::QString);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Cross-section Id (0-based)", QMetaType::QString);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Cross-section Id (1-based)", QMetaType::QString);
        nr_attrib_fields++;

        QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
        vl = new QgsVectorLayer(uri, layer_name, "memory");
        vl->blockSignals(true);
        vl->startEditing();
        dp_vl = vl->dataProvider();
        dp_vl->addAttributes(lMyAttribField);
        //dp_vl->createSpatialIndex();
        vl->updatedFields();

        struct _ntw_geom * ntw_geom = grid_file->get_network_geometry();
        struct _ntw_edges * ntw_edges = grid_file->get_network_edges();

        double xp;
        double yp;
        double rotation;
        for (int j = 0; j < crosssection_name.size(); j++)
        {
            status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
            QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
            QgsFeature MyFeature;
            MyFeature.setGeometry(MyPoints);

            MyFeature.initAttributes(nr_attrib_fields);
            int k = -1;
            k++;
            MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(crosssection_name[j]).trimmed()));
            k++;
            MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
            k++;
            MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

            dp_vl->addFeature(MyFeature);
        }
        vl->commitChanges();
        std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
        vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

        QgsSvgMarkerSymbolLayer * simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/cross_section_location_1d.svg"));
        simple_marker->setSize(5.0);
        simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Cross-section location")));

        QgsSymbol * marker = new QgsMarkerSymbol();
        marker->changeSymbolLayer(0, simple_marker);

        //set up a renderer for the layer
        QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
        vl->setRenderer(mypRenderer);
        vl->blockSignals(false);

        add_layer_to_group(vl, subTreeGroup);
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        for (int i = 0; i < tmp_layers.size(); i++)
        {
            if (tmp_layers[i]->name() == layer_name)
            {
                tmp_layers[i]->setItemVisibilityChecked(false);
            }
        }

        connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        STOP_TIMER(create_vector_layer_1D_cross_section);
    }
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_1D_retention(GRID* grid_file, JSON_READER* prop_tree, long epsg_code, QgsLayerTreeGroup* treeGroup)
{
    long status = -1;

    std::string json_key = "data.retention.id";
    std::vector<std::string> retention_name;
    status = prop_tree->get(json_key, retention_name);
    if (retention_name.size() == 0)
    {
        QString fname = QString::fromStdString(prop_tree->get_filename());
        QString msg = QString(tr("Retention locations are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(fname));
        QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
    }
    else
    {
        START_TIMERN(create_vector_layer_1D_retention);
        json_key = "data.retention.branchid";
        std::vector<std::string> branch_name;
        status = prop_tree->get(json_key, branch_name);
        if (branch_name.size() == 0)
        {
            QString fname = QString::fromStdString(prop_tree->get_filename());
            QString msg = QString(tr("Retention locations are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(fname));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            STOP_TIMER(create_vector_layer_1D_retention);
            return;
        }
        json_key = "data.retention.chainage";
        std::vector<double> chainage;
        status = prop_tree->get(json_key, chainage);
        if (chainage.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"), QString(tr("Number of chainages is zero. JSON data: ")) + QString::fromStdString(json_key));
            STOP_TIMER(create_vector_layer_1D_retention);
            return;
        }
        if (retention_name.size() != branch_name.size() || branch_name.size() != chainage.size() || retention_name.size() != chainage.size())
        {
            QMessageBox::warning(0, tr("Message: create_vector_layer_1D_retention"), QString(tr("Inconsistent data set. JSON data: ")) + QString::fromStdString(json_key)
                + "\nRetention names: " + (int)retention_name.size()
                + "\nBranches: " + (int)branch_name.size()
                + "\nChainage: " + (int)chainage.size());
            STOP_TIMER(create_vector_layer_1D_retention);
            return;
        }

        QgsLayerTreeGroup* subTreeGroup;
        subTreeGroup = get_subgroup(treeGroup, QString("Area"));
        QString layer_name = QString("Retention area");

        // create the vector
        QgsVectorLayer* vl;
        QgsVectorDataProvider* dp_vl;
        QList <QgsField> lMyAttribField;

        int nr_attrib_fields = 0;
        lMyAttribField << QgsField("Retention name", QMetaType::QString);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Retention Id (0-based)", QMetaType::QString);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Retention Id (1-based)", QMetaType::QString);
        nr_attrib_fields++;

        QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
        vl = new QgsVectorLayer(uri, layer_name, "memory");
        vl->blockSignals(true);
        vl->startEditing();
        dp_vl = vl->dataProvider();
        dp_vl->addAttributes(lMyAttribField);
        //dp_vl->createSpatialIndex();
        vl->updatedFields();

        struct _ntw_geom* ntw_geom = grid_file->get_network_geometry();
        struct _ntw_edges* ntw_edges = grid_file->get_network_edges();

        double xp;
        double yp;
        double rotation;
        for (int j = 0; j < retention_name.size(); j++)
        {
            status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
            QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
            QgsFeature MyFeature;
            MyFeature.setGeometry(MyPoints);

            MyFeature.initAttributes(nr_attrib_fields);
            int k = -1;
            k++;
            MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(retention_name[j]).trimmed()));
            k++;
            MyFeature.setAttribute(k, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
            k++;
            MyFeature.setAttribute(k, QString("%1:1").arg(j + 1));

            dp_vl->addFeature(MyFeature);
        }
        vl->commitChanges();
        std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
        vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

        QgsSvgMarkerSymbolLayer* simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/retention_location_1d.svg"));
        simple_marker->setSize(5.0);
        simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Retention location")));

        QgsSymbol* marker = new QgsMarkerSymbol();
        marker->changeSymbolLayer(0, simple_marker);

        //set up a renderer for the layer
        QgsSingleSymbolRenderer* mypRenderer = new QgsSingleSymbolRenderer(marker);
        vl->setRenderer(mypRenderer);
        vl->blockSignals(false);

        add_layer_to_group(vl, subTreeGroup);
        QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
        for (int i = 0; i < tmp_layers.size(); i++)
        {
            if (tmp_layers[i]->name() == layer_name)
            {
                tmp_layers[i]->setItemVisibilityChecked(false);
            }
        }
        connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        STOP_TIMER(create_vector_layer_1D_retention);
    }
}
//------------------------------------------------------------------------------
void HVL::create_vector_layer_1D2D_link(JSON_READER * prop_tree, long epsg_code)
{
    if (prop_tree != nullptr)
    {
        long status = -1;
        std::string json_key = "data.link1d2d.xy_1d";
        std::vector<std::string> link_1d_point;
        status = prop_tree->get(json_key, link_1d_point);
        if (link_1d_point.size() == 0)
        {
            QString fname = QString::fromStdString(prop_tree->get_filename());
            QString msg = QString(tr("Links between 1D and 2D mesh is skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(fname));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
        }
        else
        {
            START_TIMERN(create_1D2D_link_vector_layer);
            json_key = "data.link1d2d.xy_2d";
            std::vector<std::string> link_2d_point;
            status = prop_tree->get(json_key, link_2d_point);
            if (link_2d_point.size() == 0)
            {
                QString fname = QString::fromStdString(prop_tree->get_filename());
                QString msg = QString(tr("Links between 1D and 2D mesh is skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(fname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
                STOP_TIMER(create_1D2D_link_vector_layer);
                return;
            }
            if (link_1d_point.size() != link_2d_point.size())
            {
                QMessageBox::warning(0, tr("Message: create_1D2D_link_vector_layer"), QString(tr("Inconsistent data set. JSON data: ")) + QString::fromStdString(json_key)
                    + "\nPoints on 1D mesh: " + (int)link_1d_point.size()
                    + "\nPoints on 2D mesh: " + (int)link_2d_point.size());
                STOP_TIMER(create_1D2D_link_vector_layer);
                return;
            }

            QString layer_name = QString("Link 1D2D");

            // create the vector
            QgsVectorLayer * vl;
            QgsVectorDataProvider * dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 2;

            lMyAttribField << QgsField("Link Id (0-based)", QMetaType::QString);
            lMyAttribField << QgsField("Link Id (1-based)", QMetaType::QString);

            QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();

            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsMultiLineString * polylines = new QgsMultiLineString();
            QVector<QgsPointXY> point;
            QgsMultiPolylineXY lines;

            for (int j = 0; j < link_1d_point.size()/2; j++)
            {
                lines.clear();
                point.clear();
                std::string::size_type sz;     // alias of size_t
                double x1 = std::stod(link_1d_point[2*j], &sz);
                double y1 = std::stod(link_1d_point[2*j+1], &sz);
                double x2 = std::stod(link_2d_point[2*j], &sz);
                double y2 = std::stod(link_2d_point[2*j+1], &sz);
                point.append(QgsPointXY(x1, y1));
                point.append(QgsPointXY(x2, y2));
                lines.append(point);

                QgsFeature MyFeature;
                MyFeature.setGeometry(QgsGeometry::fromMultiPolylineXY(lines));

                MyFeature.initAttributes(nr_attrib_fields);
                MyFeature.setAttribute(0, QString("%1:0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                MyFeature.setAttribute(1, QString("%1:1").arg(j + 1));
                MyFeature.setValid(true);

                dp_vl->addFeature(MyFeature);
                vl->commitChanges();
            }
            std::vector<std::string> token = tokenize(prop_tree->get_filename(), '/');
            vl->serverProperties()->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

            QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
            line_marker->setWidth(0.25);
            line_marker->setColor(QColor(0, 0, 255));

            QgsSymbol * symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
            symbol->changeSymbolLayer(0, line_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
            vl->setRenderer(mypRenderer);
            vl->blockSignals(false);

            QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
            treeRoot->insertLayer(0, vl);
            add_layer_to_group(vl, treeRoot);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
            STOP_TIMER(create_1D2D_link_vector_layer);
        }
    }
}
//------------------------------------------------------------------------------
long HVL::compute_location_along_geometry(struct _ntw_geom * ntw_geom, struct _ntw_edges * ntw_edges, std::string branch_name, double chainage_node, double * xp, double * yp, double * rotation)
{
    START_TIMERN(compute_location_along_geometry);

    long status = -1;
    int nr_ntw = 1;  // Todo: HACK just one network supported
    *rotation = 0.0;
    std::vector<double> chainage;
    for (int branch = 0; branch < ntw_geom->geom[nr_ntw - 1]->count; branch++)  // loop over the geometries
    {
        if (status == 0) { break; }
        if (QString::fromStdString(ntw_geom->geom[nr_ntw - 1]->name[branch]).trimmed() == QString::fromStdString(branch_name).trimmed())  // todo Check on preformance
        {
            double branch_length = ntw_edges->edge[nr_ntw - 1]->edge_length[branch];
            size_t geom_nodes_count = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->count;

            chainage.reserve(geom_nodes_count);
            chainage[0] = 0.0;
            for (int i = 1; i < geom_nodes_count; i++)
            {
                double x1 = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->x[i - 1];
                double y1 = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->y[i - 1];
                double x2 = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->x[i];
                double y2 = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->y[i];

                chainage[i] = chainage[i - 1] + sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));  // Todo: HACK: this is just the euclidian distance
            }
            double fraction = -1.0;
            fraction = chainage_node / branch_length;
            if (fraction < 0.0 || fraction > 1.0)
            {
#ifdef NATIVE_C
                fprintf(stderr, "UGRID::determine_computational_node_on_geometry()\n\tBranch(%d). Offset %f is larger then branch length %f.\n", branch + 1, chainage_node, branch_length);
#else
                //QMessageBox::warning(0, "HVL::compute_location_along_geometry", QString("UGRID::determine_computational_node_on_geometry()\nBranch(%3). Offset %1 is larger then branch length %2.\n").arg(offset).arg(branch_length).arg(branch+1));
#endif
            }
            double chainage_point = fraction * chainage[geom_nodes_count - 1];

            for (int i = 1; i < geom_nodes_count; i++)
            {
                if (chainage_point <= chainage[i])
                {
                    double alpha = (chainage_point - chainage[i - 1]) / (chainage[i] - chainage[i - 1]);  // alpha is a weight coefficient
                    double x1 = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->x[i - 1];
                    double y1 = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->y[i - 1];
                    double x2 = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->x[i];
                    double y2 = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->y[i];

                    *xp = x1 + alpha * (x2 - x1);
                    *yp = y1 + alpha * (y2 - y1);

                    if (x2 - x1 > 0)
                    {
                        alpha = 90.0 - asin((y2 - y1) / (chainage[i] - chainage[i - 1])) * 180.0 / M_PI;  // Mathematic convention to nautical convention
                    }
                    else
                    {
                        alpha = 90.0 - 180.0 + asin((y2 - y1) / (chainage[i] - chainage[i - 1])) * 180.0 / M_PI;  // Mathematic convention to nautical convention
                    }
                    *rotation = 90.0 + alpha;
                    status = 0;
                    break;
                }
            }
        }
    }
    STOP_TIMER(compute_location_along_geometry);
    return status;
}
//------------------------------------------------------------------------------
long HVL::find_location_boundary(struct _ntw_nodes * ntw_nodes, std::string bnd_name, double * xp, double * yp)
{
    START_TIMERN(find_location_boundary);

    long status = -1;
    for (int i = 0; i < ntw_nodes->node[0]->count; i++)
    {
        if (QString::fromStdString(ntw_nodes->node[0]->name[i]).trimmed() == QString::fromStdString(bnd_name))
        {
            *xp = ntw_nodes->node[0]->x[i];
            *yp = ntw_nodes->node[0]->y[i];
            status = 0;
            break;
        }
    }
    STOP_TIMER(find_location_boundary);
    return status;
}
//
//-----------------------------------------------------------------------------
//
GRID* HVL::get_active_grid_file(QString layer_id)
{
    // Function to find the filename belonging by the highlighted layer,
    // or to which group belongs the highligthed layer,
    // or the highlighted group
    QgsLayerTree* treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
    QList <QgsLayerTreeGroup*> groups = treeRoot->findGroups();
    QgsMapLayer* active_layer = NULL;
    GRID* active_grid_file = nullptr;

    if (layer_id == "")
    {
        active_layer = m_QGisIface->activeLayer();
        if (active_layer != nullptr)
        {
            layer_id = active_layer->id();
        }
    }
    if (layer_id != "")
    {
        //QMessageBox::information(0, "Information", QString("qgis_umesh::get_active_layer()\nActive layer: %1.").arg(active_layer->name()));
        // if there is an active layer, belongs it to a Mesh-group?
        QList< QgsLayerTreeGroup* > myGroups = treeRoot->findGroups();
        for (int j = 0; j < m_grid_file.size(); j++)
        {
            for (int i = 0; i < myGroups.size(); i++)
            {
                if (myGroups[i] != nullptr)
                {
                    QStringList layerIDs = myGroups[i]->findLayerIds();
                    for (int k = 0; k < layerIDs.size(); k++)
                    {
                        // belong the active layer to this group?
                        if (layerIDs[k] == layer_id)
                        {
                            //QMessageBox::information(0, "Information", QString("qgis_umesh::get_active_layer()\nGroup name: %1\nActive layer: %2.").arg(myGroup->name()).arg(active_layer->name()));
                            // get the full file name
                            if (myGroups[i]->name().contains(m_grid_file[j]->get_filename().fileName()))
                            {
                                active_grid_file = m_grid_file[j];
                            }
                        }
                    }
                }
            }
        }
    }
    //else
    //{
    //    QMessageBox::information(0, "Information", QString("No layer selected, determination of output files is not possible."));
    //}
    return active_grid_file;
}

//------------------------------------------------------------------------------
QgsLayerTreeGroup* HVL::get_subgroup(QgsLayerTreeGroup* treeGroup, QString sub_group_name)
{
    QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
    QgsLayerTreeGroup* subTreeGroup = treeGroup->findGroup(sub_group_name);
    if (subTreeGroup == nullptr)  // Treegroup Area does not exist create it
    {
        treeGroup->addGroup(sub_group_name);
        subTreeGroup = treeGroup->findGroup(sub_group_name);
        subTreeGroup->setExpanded(true);  // true is the default
        subTreeGroup->setItemVisibilityChecked(true);
        subTreeGroup->setItemVisibilityCheckedRecursive(true);
    }
    return subTreeGroup;
}
//------------------------------------------------------------------------------
void HVL::add_layer_to_group(QgsVectorLayer* vl, QgsLayerTreeGroup* treeGroup)
{
    START_TIMERN(add_layer_to_group);

    QgsLayerTree* root = QgsProject::instance()->layerTreeRoot();

    QgsMapLayer* map_layer = QgsProject::instance()->addMapLayer(vl, false);
    treeGroup->addLayer(map_layer);

    QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
    for (int i = 0; i < tmp_layers.size(); i++)
    {
        if (tmp_layers[i]->name() == "Mesh1D Topology edges" ||
            tmp_layers[i]->name() == "Mesh1D Connection nodes" ||
            tmp_layers[i]->name() == "Mesh1D edges" ||
            tmp_layers[i]->name() == "Mesh2D nodes" ||
            tmp_layers[i]->name() == "Mesh2D faces")
        {
            tmp_layers[i]->setItemVisibilityChecked(false);
        }
    }
    root->removeLayer(map_layer);
    STOP_TIMER(add_layer_to_group);
}
std::vector<std::string> HVL::tokenize(const std::string& s, char c) {
    auto end = s.cend();
    auto start = end;

    std::vector<std::string> v;
    for (auto it = s.cbegin(); it != end; ++it) {
        if (*it != c) {
            if (start == end)
                start = it;
            continue;
        }
        if (start != end) {
            v.emplace_back(start, it);
            start = end;
        }
    }
    if (start != end)
        v.emplace_back(start, end);
    return v;
}
//
std::vector<std::string> HVL::tokenize(const std::string& s, std::size_t count)
{
    size_t minsize = s.size() / count;
    std::vector<std::string> tokens;
    for (size_t i = 0, offset = 0; i < count; ++i)
    {
        size_t size = minsize;
        if ((offset + size) < s.size())
            tokens.push_back(s.substr(offset, size));
        else
            tokens.push_back(s.substr(offset, s.size() - offset));
        offset += size;
    }
    return tokens;
}
