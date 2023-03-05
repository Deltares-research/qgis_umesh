#include "qgis_umesh_version.h"
#include "qgis_umesh.h"
#include "perf_timer.h"

#define GUI_EXPORT __declspec(dllimport)

#if defined(WIN64)
#  include <windows.h>
#  define strdup _strdup
#endif

#define EXPERIMENT 1

/* static */ const QString qgis_umesh::s_ident = QObject::tr("@(#)" qgis_umesh_company ", " qgis_umesh_program ", " qgis_umesh_version_number ", " qgis_umesh_arch", " __DATE__", " __TIME__);
/* static */ const QString qgis_umesh::s_name = QObject::tr("" qgis_umesh_company ", " qgis_umesh_program " Development");
/* static */ const QString qgis_umesh::s_description = QObject::tr("Plugin to read 1D and 2D unstructured meshes, UGRID-format (" __DATE__", " __TIME__")");
/* static */ const QString qgis_umesh::s_category = QObject::tr("Plugins");
/* static */ const QString qgis_umesh::s_plugin_version = QObject::tr(qgis_umesh_version_number);

/* static */ const QgisPlugin::PluginType qgis_umesh::s_plugin_type = QgisPlugin::UI;
/* static */ const QString* s_plugin_icon;

//
//-----------------------------------------------------------------------------
//
qgis_umesh::qgis_umesh(QgisInterface* iface):
    QgisPlugin(s_name, s_description, s_category, s_plugin_version, s_plugin_type),
    mQGisIface(iface)
{
#include "vsi.xpm"
#include "vsi_disabled.xpm"
    mQGisIface = iface;
    icon_picture = new QIcon();
    icon_picture->addPixmap(QPixmap(vsi), QIcon::Normal, QIcon::On);
    icon_picture->addPixmap(QPixmap(vsi_disabled), QIcon::Normal, QIcon::Off);

    _fil_index = -1;
    _his_cf_fil_index = -1;
    m_working_dir = QString("");

    this->pgBar = new QProgressBar();
    this->pgBar->setMaximum(1000);
    this->pgBar->setValue(0);
    this->pgBar->hide();

    status_bar = mQGisIface->statusBarIface();
    status_bar->addPermanentWidget(this->pgBar, 0, QgsStatusBar::Anchor::AnchorRight);

    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible}
    //connect(treeRoot, &QgsLayerTree::removedChildren, this, &qgis_umesh::onWillRemoveChildren);
    connect(treeRoot, SIGNAL(removedChildren(QgsLayerTreeNode *, int, int)), this, SLOT(onWillRemoveChildren(QgsLayerTreeNode *, int, int)));
    connect(treeRoot, SIGNAL(layerWillBeRemoved(QString)), this, SLOT(onRemovedChildren(QString)));
}
void qgis_umesh::onWillRemoveChildren(QgsLayerTreeNode * node, int indexFrom, int indexTo)
{
    Q_UNUSED(indexTo);
    QString name = node->name();  // contains the group name
    QList <QgsLayerTreeNode*> children =  node->children();
    if (children.size() == 0 && (name.contains("UGRID - ") || name.contains("History -")))
    {
        if (indexFrom == 0)
        {
            // remove the complete group only when the selected name is the group name
            // Close the map time manager window
            // Remove the file entry belonging to this group

            QString timings_file(m_working_dir + "/timing_qgis_umesh.log");
            PRINT_TIMERN(timings_file.toStdString());
            CLEAR_TIMER();
            m_working_dir = QString("");  // make it empty
            if (MapTimeManagerWindow::get_count() != 0)
            {
                // close the map time manager window
                mtm_widget->closeEvent(nullptr);
                mtm_widget->close();
            }
            for (int i = m_grid_file.size() - 1; i >= 0; --i)
            {
                QString filename = m_grid_file[i]->get_filename().fileName();
                if (name.contains(filename))
                {
                    delete m_grid_file[i];
                    m_grid_file.erase(m_grid_file.begin() + i);
                }
            }
        }
    }
}
void qgis_umesh::onRemovedChildren(QString name)
{
    QString msg = QString("Clean up the memory for this group: \'%1\'").arg(name);
    QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
}

//
//-----------------------------------------------------------------------------
//
qgis_umesh::~qgis_umesh()
{
}
// -----------------------------------------------------------
void qgis_umesh::CrsChanged()
{
    // Set the individual CRS via the group CRS, NOT the individual layers separately
    int checked = 0;
    QgsCoordinateReferenceSystem new_crs;
    QgsCoordinateReferenceSystem s_crs = QgsProject::instance()->crs();
    QgsMapLayer * active_layer = mQGisIface->activeLayer();
    QString layer_name;  //
    if (active_layer == nullptr)
    {
        QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
        QList<QgsLayerTreeNode *> selNodes = mQGisIface->layerTreeView()->selectedNodes(true);
        for (int i = 0; i < selNodes.count(); i++)
        {
            checked = 0;
            QgsLayerTreeNode * selNode = selNodes[i];

            if(selNode->nodeType() == QgsLayerTreeNode::NodeType::NodeGroup)
            {
                checked = 1;
                QgsLayerTreeGroup * myGroup = treeRoot->findGroup(selNode->name());
                QList <QgsLayerTreeLayer *> layer = myGroup->findLayers();
                new_crs = layer[0]->layer()->crs();
                layer_name = layer[0]->layer()->name();
                //QMessageBox::information(0, "qgis_umesh::CrsChanged()", QString("Selected group: %1\nNew CRS: %2\nPrev. CRS: %3\nScreen CRS: %4").arg(selNode->name()).arg(new_crs.authid()).arg(m_crs.authid()).arg(s_crs.authid()));
                //QMessageBox::information(0, "qgis_umesh::CrsChanged()", QString("Layer[0] name: %1.").arg(layer[0]->layer()->name()));
                // Change coordinates to new_crs, by overwriting the mapping->epsg and mapping->epsg_code
                GRID * ugrid_file = get_active_grid_file(layer[0]->layer()->id());
                if (ugrid_file != nullptr)
                {
                    QString epsg_code = new_crs.authid();
                    QStringList parts = epsg_code.split(':');
                    long epsg = parts.at(1).toInt();
                    ugrid_file->set_grid_mapping_epsg(epsg, epsg_code.toUtf8().constData());
                }
            }
        }
        if (checked == 0)
        {
            QMessageBox::information(0, "qgis_umesh::CrsChanged()", QString("Layer name: %1\nChecked: %2.").arg(active_layer->name()).arg(checked));
        }
    }
    else
    {
        // QMessageBox::information(0, "Message", QString("You have to select a group layer."));
    }
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::initGui()
{
#include "vsi.xpm"
    START_TIMERN(initgui)

    std::cout << "qgis_umesh::initGui" << std::endl;

    m_crs = QgsProject::instance()->crs();
    if (m_crs == QgsCoordinateReferenceSystem("EPSG:4326") ||  // wgs84 == espg:4326
        m_crs == QgsCoordinateReferenceSystem("EPSG:28991") ||   // Amersfoort old
        m_crs == QgsCoordinateReferenceSystem("epsg:28992"))  // Amersfoort new
    {
        //QMessageBox::warning(0, tr("Message"), QString(tr("The QGIS project CRS is: ")) + _crs.authid() + "\n" + QString(tr("Please make the Delft3D plugin CRS equal to the QGIS project CRS.")));
    }
    else
    {
        //QMessageBox::warning(0, tr("Message"), QString(tr("The QGIS project CRS is: ")) + _crs.authid() + "\n" + QString(tr("Please adjust the project CRS to the CRS's supported by the Delft3D plugins (i.e. 28992).")));
    }

    connect(this->mQGisIface, SIGNAL(projectRead()), this, SLOT(project_read()));
    connect(this->mQGisIface, SIGNAL(currentLayerChanged(QgsMapLayer *)), this, SLOT(set_enabled()));

    QProcessEnvironment env;
    QString program_files = QProcessEnvironment::systemEnvironment().value("ProgramFiles", "");
    program_files = program_files + QString("/deltares/qgis_umesh");
    QDir program_files_dir = QDir(program_files);
    if (!program_files_dir.exists())
    {
        QMessageBox::warning(0, tr("Message"), QString(tr("Missing installation directory:\n")) + program_files);
    }
    executable_dir = QDir::currentPath();  // get directory where the executable is located
    current_dir = QDir::currentPath();  // get current working directory  (_getcwd(current_dir, _MAX_PATH);)
    //
    // Load the plugins list
    //

    QDir::setCurrent(current_dir.canonicalPath());  // (void)_chdir(current_dir);

    //
    // Set Deltares icon
    //
    QIcon actionIcon = *icon_picture;

    mainAction = new QAction(actionIcon, tr("&Enable/Disable showing meshes"));
    mainAction->setToolTip("Enable/Disable showing meshes");
    mainAction->setCheckable(true);
    mainAction->setChecked(true);
    mainAction->setEnabled(true);

    tbar = mQGisIface->addToolBar("Delft3D FM (1D, 2D and 1D2D)");
    tbar->addAction(mainAction);
    tbar->addSeparator();

    connect(mainAction, SIGNAL(changed()), this, SLOT(set_enabled()));
    connect(mainAction, SIGNAL(changed()), this, SLOT(activate_layers()));
    connect(mainAction, SIGNAL(changed()), this, SLOT(activate_observation_layers()));


    //------------------------------------------------------------------------------
    icon_open = get_icon_file(program_files_dir, "/icons/file_open.png");
    open_action_mdu = new QAction(icon_open, tr("&Open D-Flow FM (json) ..."));
    open_action_mdu->setToolTip(tr("Open D-Flow FM MDU, json file"));
    open_action_mdu->setStatusTip(tr("Open D-Flow FM Master Definition file, json format"));
    open_action_mdu->setEnabled(true);
    connect(open_action_mdu, SIGNAL(triggered()), this, SLOT(open_file_mdu()));

    icon_open = get_icon_file(program_files_dir, "/icons/file_open.png");
    open_action_map = new QAction(icon_open, tr("&Open UGRID ..."));
    open_action_map->setToolTip(tr("Open UGRID 1D2D file"));
    open_action_map->setStatusTip(tr("Open UGRID file containing 1D, 2D and/or 1D2D meshes"));
    open_action_map->setEnabled(true);
    connect(open_action_map, SIGNAL(triggered()), this, SLOT(openFile()));

    icon_open_his_cf = get_icon_file(program_files_dir, "/icons/file_open.png");
    open_action_his_cf = new QAction(icon_open_his_cf, tr("&Open HisCF ..."));
    open_action_his_cf->setToolTip(tr("Open CF compliant time series file"));
    open_action_his_cf->setStatusTip(tr("Open Climate and Forecast compliant time series file"));
    open_action_his_cf->setEnabled(true);
    connect(open_action_his_cf, SIGNAL(triggered()), this, SLOT(open_file_his_cf()));

    icon_open = get_icon_file(program_files_dir, "/icons/file_open.png");
    open_action_link1d2d_json = new QAction(icon_open, tr("&Open Link1D2D (json) ..."));
    open_action_link1d2d_json->setToolTip(tr("Open Link1D2D, json file"));
    open_action_link1d2d_json->setStatusTip(tr("Open for 1D2DCoupler the Link1D2D file, json format"));
    open_action_link1d2d_json->setEnabled(true);
    connect(open_action_link1d2d_json, SIGNAL(triggered()), this, SLOT(open_file_link1d2d_json()));

    icon_open = get_icon_file(program_files_dir, "/icons/file_open.png");
    open_action_obs_point_json = new QAction(icon_open, tr("&Open Observation point (json) ..."));
    open_action_obs_point_json->setToolTip(tr("Open Obs point, json file"));
    open_action_obs_point_json->setStatusTip(tr("Open observation point file, json format"));
    open_action_obs_point_json->setEnabled(true);
    connect(open_action_obs_point_json, SIGNAL(triggered()), this, SLOT(open_file_obs_point_json()));

    icon_edit_1d_obs_points = get_icon_file(program_files_dir, "/icons/edit_observation_points.png");
    edit_action_1d_obs_points = new QAction(icon_edit_1d_obs_points, tr("&1D: Observation points ..."));
    edit_action_1d_obs_points->setToolTip(tr("Add/Remove observation points"));
    edit_action_1d_obs_points->setStatusTip(tr("Add/Remove observation points on 1D geometry network"));
    edit_action_1d_obs_points->setEnabled(true);
    connect(edit_action_1d_obs_points, &QAction::triggered, this, &qgis_umesh::edit_1d_obs_points);

    icon_experiment = get_icon_file(program_files_dir, "/icons/experiment.png");
    trial_experiment = new QAction(icon_experiment, tr("&Experiment"));
    trial_experiment->setToolTip(tr("Experiment"));
    trial_experiment->setStatusTip(tr("Experiment"));
    trial_experiment->setEnabled(true);
    connect(trial_experiment, &QAction::triggered, this, &qgis_umesh::experiment);

    icon_inspect = get_icon_file(program_files_dir, "/icons/remoteolv_icon.png");
    inspectAction = new QAction(icon_inspect, tr("&Show map output ..."));
    inspectAction->setToolTip(tr("Show map output time manager"));
    inspectAction->setStatusTip(tr("Show time dependent map output as animation via time manager"));
    inspectAction->setEnabled(true);
    connect(inspectAction, SIGNAL(triggered()), this, SLOT(set_show_map_output()));

    icon_plotcfts = get_icon_file(program_files_dir, "/icons/plotcfts.png");
    plotcftsAction = new QAction();
    plotcftsAction->setIcon(icon_plotcfts);
    plotcftsAction->setText("PlotCFTS");
    plotcftsAction->setToolTip(tr("Start PlotCFTS to view time series"));
    plotcftsAction->setStatusTip(tr("Start plot program PlotCFTS to view Climate and Forecast compliant time series"));
    connect(plotcftsAction, SIGNAL(triggered()), this, SLOT(start_plotcfts()));

    icon_mapoutput = get_icon_file(program_files_dir, "/icons/map_output.png");
    mapoutputAction = new QAction();
    mapoutputAction->setIcon(icon_mapoutput);
    mapoutputAction->setText("Map Output Settings ...");
    mapoutputAction->setToolTip(tr("Supply the settings for the map output animation"));
    mapoutputAction->setStatusTip(tr("Supply the settings for the map output animation"));
    mapoutputAction->setEnabled(true);
    connect(mapoutputAction, SIGNAL(triggered()), this, SLOT(mapPropertyWindow()));
    //connect(mapoutputAction, &QAction::triggered, MapTimeManagerWindow, &MapTimeManagerWindow::contextMenu);

    icon_open = get_icon_file(program_files_dir, "/icons/file_open.png");
    QAction * saveAction = new QAction(icon_open, tr("&Save"));
    saveAction->setToolTip(tr("Dummmy Item"));
    saveAction->setStatusTip(tr("Dummy item shown in statusbar"));
    saveAction->setEnabled(true);
    connect(saveAction, SIGNAL(triggered()), this, SLOT(openFile()));

    // Help menu
    showUserManualAct = new QAction(tr("&User Manual ..."), this);
    showUserManualAct->setStatusTip(tr("Opens the User Manual (pdf-format)"));
    showUserManualAct->setEnabled(true);
    connect(showUserManualAct, SIGNAL(triggered()), this, SLOT(ShowUserManual()));

    aboutAction = new QAction(tr("&About ..."), this);
    aboutAction->setToolTip(tr("Show the About box ..."));
    aboutAction->setStatusTip(tr("Show the application's About box"));
    aboutAction->setEnabled(true);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

//------------------------------------------------------------------------------
    _menuToolBar = new QToolBar();

    QMenuBar * janm = new QMenuBar();
    janm->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QMenu * janm1 = janm->addMenu("File");
    janm1->addAction(open_action_map);
    janm1->addAction(open_action_his_cf);

    janm1 = janm->addMenu("Output");
    janm1->addAction(inspectAction);
    janm1->addAction(plotcftsAction);

    janm1 = janm->addMenu("Settings");
    janm1->addAction(mapoutputAction);

    janm1 = janm->addMenu("Help");
    janm1->addAction(showUserManualAct);
    janm1->addSeparator();
    janm1->addAction(aboutAction);

#if EXPERIMENT
    janm1 = janm->addMenu("Trials");
    janm1->addAction(open_action_mdu);
    janm1->addAction(open_action_link1d2d_json);
    janm1->addAction(open_action_obs_point_json);
    janm1->addAction(edit_action_1d_obs_points);
    janm1->addAction(trial_experiment);
#endif
    _menuToolBar->addWidget(janm);
    tbar->addWidget(_menuToolBar);
    tbar->addSeparator();
    //tbar->addAction(openAction);
    tbar->addAction(inspectAction);
    tbar->addAction(plotcftsAction);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tbar->addWidget(spacer);

    STOP_TIMER(initgui)

//------------------------------------------------------------------------------
//
// create our map tool
//
    mMyEditTool = new MyEditTool(mQGisIface->mapCanvas());
    mMyCanvas = new MyCanvas(mQGisIface);
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::set_enabled()
{
    if (mainAction->isChecked())
    {
        mainAction->setChecked(true);
        //QMessageBox::warning(0, tr("Message"), QString("Plugin will be enabled.\n"));
        _menuToolBar->setEnabled(true);
        open_action_map->setEnabled(true);
        open_action_his_cf->setEnabled(true);
        open_action_mdu->setEnabled(true);

        inspectAction->setEnabled(false);
        GRID * grid_file = get_active_grid_file("");
        if (grid_file != nullptr)
        {
            inspectAction->setEnabled(true);
        }
    }
    else
    {
        mainAction->setChecked(false);
        //QMessageBox::warning(0, tr("Message"), QString("Plugin will be disabled.\n"));
        _menuToolBar->setEnabled(false);
        open_action_map->setEnabled(false);
        inspectAction->setEnabled(false);
        open_action_his_cf->setEnabled(false);
        open_action_mdu->setEnabled(false);
    }
}
//
//-----------------------------------------------------------------------------
//
QIcon qgis_umesh::get_icon_file(QDir home_dir, QString file)
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
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::show_map_output(GRID * grid_file)
{
    //QMessageBox::information(0, "Information", QString("qgis_umesh::show_map_output()\nTime manager: %1.").arg(MapTimeManagerWindow::get_count()));

    // Check on at least UGRID-1.0 file formaat
    long nr_times = 0;
    struct _global_attributes * globals;
    bool conventions_found = false;

    if (grid_file != nullptr)
    {
        nr_times = grid_file->get_count_times();
    }
    else
    {
        QMessageBox::warning(0, "Fatal error", QString("qgis_umesh::show_map_output().\nVariable grid_file is a nullptr."));
        return;
    }

    if (nr_times > 0)
    {
        if (MapTimeManagerWindow::get_count() == 0)  // create a docked window if it is not already there.
        {
            // CRS layer should be the same as the presentation CRS
            QgsCoordinateReferenceSystem s_crs = QgsProject::instance()->crs();  // CRS of the screen
            QgsMapLayer * active_layer = mQGisIface->activeLayer();
            QgsCoordinateReferenceSystem new_crs = active_layer->crs(); // CRS of the vector layer
            if (s_crs != new_crs)
            {
                QMessageBox::information(0, "Message", QString("Screen CRS \"%1\" not equal to layer CRS: \"%2\"\nPlease set first the screen CRS equal to the layer CRS.").arg(s_crs.authid()).arg(new_crs.authid()));
            }
            //active_layer->setDataSource()
            mtm_widget = new MapTimeManagerWindow(mQGisIface, grid_file, mMyCanvas);
            mtm_widget->setContextMenuPolicy(Qt::CustomContextMenu);
            mQGisIface->addDockWidget(Qt::LeftDockWidgetArea, mtm_widget);
            QObject::connect(mtm_widget, &MapTimeManagerWindow::customContextMenuRequested, mtm_widget, &MapTimeManagerWindow::contextMenu);
        }
    }
    else
    {
        QString fname = grid_file->get_filename().canonicalFilePath();
        QMessageBox::information(0, tr("Message"), QString("No time-series available in file:\n%1.").arg(fname));
    }
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::edit_1d_obs_points()
{
    //QMessageBox::information(0, "Information", QString("qgis_umesh::edit_1d_obs_points()."));
    GRID * grid_file = get_active_grid_file("");
    if (grid_file == nullptr)
    {
        QMessageBox::information(0, "Information", QString("qgis_umesh::edit_1d_obs_points()\nSelect first a layer of a UGRID file."));
        return;
    }

    // get UGRID Mesh group
    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
    QgsLayerTreeGroup * myGroup = treeRoot->findGroup(QString("UGRID Mesh - %1").arg(_fil_index + 1));
    if (myGroup == nullptr)
    {
        QMessageBox::information(0, "Information", QString("Group layer with name \"UGRID Mesh - %1\" not found.").arg(_fil_index + 1));
        return;
    }

    // Get selected geometry space and/or observation point layer
    QList<QgsMapLayer *> geom_layers = QgsProject::instance()->mapLayersByName("Mesh1D geometry");
    QList<QgsMapLayer *> obs_layers = QgsProject::instance()->mapLayersByName("Observation points");
    // Look for the selected layers
    QgsMapLayer * geom_layer;
    QgsMapLayer * obs_layer;
    if (geom_layers.size() > 1 || obs_layers.size() > 1)
    {
        QMessageBox::information(0, "Information", QString("Please load just one UGRID file."));
        return;
    }
    if (geom_layers.size() == 0 && obs_layers.size() == 1)
    {
        // there is a no 1D geometry and one observation point layer available, is it selected
        geom_layer = nullptr;
        obs_layer = obs_layers[0];
    }
    else if (geom_layers.size() == 1 && obs_layers.size() == 0)
    {
        // there is one 1D geometry layer available, is it selected
        geom_layer = geom_layers[0];
        obs_layer = nullptr;
    }
    else if (geom_layers.size() == 1 && obs_layers.size() == 1)
    {
        // there is one 1D geometry and one observation point layer available, are they selected
        geom_layer = geom_layers[0];
        obs_layer = obs_layers[0];
    }
    else
    {
        QMessageBox::information(0, "Information", QString("Select a layer with name: 'Observation points' and/or 'Mesh1D Geometry'."));
        return;
    }
    // Are geom_layer and obs_layer from the same UGRID Mesh group, i.e. the same ugrid_file
    // Are geom_layer and obs_layer selected
    QList<QgsLayerTreeNode *> selNodes = mQGisIface->layerTreeView()->selectedNodes(true);
    QList< QgsLayerTreeLayer * > myLayers = myGroup->findLayers();
    int obs_index = -1;
    int geom_index = -1;
    for (int i = 0; i < myLayers.size(); i++)
    {
        if (obs_layer->name() == myLayers[i]->name())
        {
            obs_index = i;
        }
        if (geom_layer != nullptr && geom_layer->name() == myLayers[i]->name())
        {
            geom_index = i;
        }
    }
    // 1D: obs!=-1 and geom!=-1  (both in same UGRID Mesh group)
    // Names are in myGroup but are they selected?
    bool geom_ok = false;
    bool obs_ok = false;
    for (int i = 0; i < selNodes.size(); i++)
    {
        if (geom_index != -1 && selNodes[i]->name() == myLayers[geom_index]->name())
        {
            geom_ok = true;
        }
        if (selNodes[i]->name() == myLayers[obs_index]->name())
        {
            obs_ok = true;
        }
    }
    // 2D: obs==true
    if (!obs_ok && !geom_ok)  // 1D observation point at branch
    {
        QMessageBox::information(0, "qgis_umesh::edit_1d_obs_points()", QString("Please select 'Observation point' and/or 'Mesh1D geometry' layer."));
        return;
    }
    if (!geom_ok && geom_layers.size() == 1)
    {
        // Selected geometry layer is not in the UGRID Mesh group
        return;
    }
    if (!obs_ok && geom_ok)  //  no observation point layer selected, geometry space is selected
    {
        // add first 1D Observation point, so create first a layer
        QMessageBox::information(0, "qgis_umesh::edit_1d_obs_points()", QString("Development: Add first 1D observation point.\nCreate a new 'Observation point' layer OR\nAdd to exiting 'Observation point' layer."));
    }
    if (obs_ok && geom_ok)  // 1D observation point at branch
    {
        EditObsPoints * editObs_widget;
        if (EditObsPoints::get_count() == 0)  // create a docked window if it is not already there.
        {
            if (!geom_ok) { geom_layer = nullptr; }
            editObs_widget = new EditObsPoints(obs_layer, geom_layer, grid_file, mQGisIface);
            mQGisIface->addDockWidget(Qt::LeftDockWidgetArea, editObs_widget);
        }
    }
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::experiment()
{
    // Experiment: Time dependent data in the cell_area layer, whta is the performance draw back when using layers of QGIS
    QMessageBox::information(0, "Information", QString("qgis_umesh::experiment()\nExperiment."));
    QgsMapLayer * layer = mQGisIface->activeLayer();
    QgsVectorLayer *vlayer = (QgsVectorLayer*)layer;
    //QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>(layer);
    QString layer_name("mesh2d_s1");
    GRID* grid_file = m_grid_file[_fil_index];
    struct _mapping * mapping;
    struct _mesh2d * mesh2d;

    mapping = grid_file->get_mapping();
    mesh2d = grid_file->get_mesh_2d();

    if (layer->name().contains("cell_area"))
    {
        QMessageBox::information(0, "Information", QString("qgis_umesh::experiment()\nExperiment: %1").arg(layer_name));
        DataValuesProvider2D<double>std_data_at_face = grid_file->get_variable_values(layer_name.toStdString());
        double * z_value = std_data_at_face.GetValueAtIndex(0, 0);
        int z_value_size = std_data_at_face.m_numXY;

        QgsVectorDataProvider * dp_vl = vlayer->dataProvider();

        //dp_vl->capabilities();
        QgsFeatureIterator feats = vlayer->getFeatures();
        QgsFeature feat;
        vlayer->startEditing();
        int i = -1;
        //QMap
        while (feats.nextFeature(feat))
        {
            i += 1;
            //z_value[i] *= 5.1;
            //dp_vl->changeAttributeValues(i, 2, *z_value[i]);
            //feat.setAttribute(2, z_value[i]);
        }
        for (int j = 0; j < z_value_size; j++)
        {
            //z_value[j] *= 5.1;
            //geom = QgsGeometry.fromPoint(QgsPoint(111, 222))

            //vlayer->dataProvider()->changeAttributeValues(attributeChanges);
            //dp_vl->deleteFeatures(;
            //feat = vlayer->getFeatures();  // this statement is slow (20200518)
            //feat.setAttribute(2, *z_value[i]);
            //dp_vl->addFeature(feat);
        }
        vlayer->commitChanges();
    }
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::mapPropertyWindow()
{
    if (MapPropertyWindow::get_count() == 0)  // create a window if it is not already there.
    {
        MapPropertyWindow * map_property = new MapPropertyWindow(mMyCanvas);
    }
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::ShowUserManual()
{
    char pdf_reader[1024];
    int spawn_err = 0;

    QString program_files = QProcessEnvironment::systemEnvironment().value("ProgramFiles", "");
    program_files = program_files + QString("/deltares/qgis_umesh");
    QDir program_files_dir = QDir(program_files);
    QString manual_path = program_files_dir.absolutePath();
    QString user_manual = manual_path + "/doc/" + QString("qgis_umesh_um.pdf");
    QByteArray manual = user_manual.toUtf8();
    char * pdf_document = manual.data();

    FILE *fp = fopen(pdf_document, "r");
    if (fp != NULL)
    {
        fclose(fp);
        long res = (long)FindExecutableA((LPCSTR)pdf_document, NULL, (LPSTR)pdf_reader);
        if (res >= 32)
        {
            spawn_err = QT_SpawnProcess(NO_WAIT_MODE, pdf_reader, &pdf_document);
        }
    }
    else
    {
        QMessageBox::warning(NULL, QObject::tr("Warning"), QObject::tr("Cannot open file: %1").arg(user_manual));
    }
}
/* @@-------------------------------------------------
Function:   QT_SpawnProcess
Author:     Jan Mooiman
Function:   Start an process, main program will wait or continue
depending on argument waiting
Context:    -
-------------------------------------------------------*/
int qgis_umesh::QT_SpawnProcess(int waiting, char * prgm, char ** args)
{
    int i;
    int status;
    QStringList argList;

    status = -1;  /* set default status as not started*/

    QProcess * proc = new QProcess();

    i = 0;
    while (args[i] != NULL && i < 1)  // just one argument allowed
    {
        argList << args[i];
        i++;
    }
    proc->start(prgm, argList);

    if (proc->state() == QProcess::NotRunning)
    {
        delete proc;
        return status;
        // error handling
    }
    //
    // Process sstarted succesfully
    //
    status = 0;
    if (waiting == WAIT_MODE)
    {
        proc->waitForFinished();
        delete proc;
    }
    return status;
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::about()
{
    char * text;
    char * source_url;
    QString * qtext;
    QString * qsource_url;
    QString * msg_text;
    text = getversionstring_qgis_umesh();
    qtext = new QString(text);
    source_url = getsourceurlstring_qgis_umesh();
    qsource_url = new QString(source_url);

    msg_text = new QString("Deltares\n");
    msg_text->append("Plot results on UGRID compliant meshes. 1D mesh with its geometry, 1D2D, 2D and 3D meshes.\n");
    msg_text->append(qtext);
    msg_text->append("\nSource: ");
    msg_text->append(qsource_url);
    QMessageBox::about(NULL, tr("About"), *msg_text);
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::openFile()
{
    QString * str = new QString();
    QFileDialog * fd = new QFileDialog();
    QStringList * list = new QStringList();

    str->clear();

    str->append("netCDF files");
    str->append(" (*.nc)");
    list->append(*str);
    str->clear();

    str->append("UGRID map");
    str->append(" (*_map.nc)");
    list->append(*str);
    str->clear();

    str->append("UGRID mesh");
    str->append(" (*_net.nc)");
    list->append(*str);
    str->clear();

//    str->append("UGRID");
//    str->append(" (*_clm.nc)");
//    list->append(*str);
//    str->clear();

    str->append("All files");
    str->append(" (*.*)");
    list->append(*str);

    fd->setWindowTitle("Open Unstructured grid file");
    fd->setNameFilters(*list);
    fd->selectNameFilter(list->at(0));
    fd->setFileMode(QFileDialog::ExistingFiles);  // Enable multiple file selection at once

    QDir path("d:/mooiman/home/models/delft3d/GIS/grids/test_qgis");
    if (path.exists())
    {
        fd->setDirectory(path);
    }

    bool canceled = fd->exec() != QDialog::Accepted;
    if (!canceled)
    {
        QStringList * QFilenames = new QStringList();
        QFilenames->append(fd->selectedFiles());

        this->pgBar->show();
        this->pgBar->setValue(0);

        for (QStringList::Iterator it = QFilenames->begin(); it != QFilenames->end(); ++it) {
            openFile(QFileInfo(*it));
        }

        this->pgBar->hide();

    }
    delete fd;
    delete str;
}
void qgis_umesh::openFile(QFileInfo ncfile)
{
    START_TIMERN(open_file);
    int ncid;
    if (m_working_dir.isEmpty())
    {
        m_working_dir = ncfile.absolutePath().toUtf8();
    }

    long stat;
    GRID* grid_file = new GRID();
    stat = grid_file->open(ncfile, this->pgBar);
    if (stat != 0)
    {
        QMessageBox::warning(0, tr("Warning"), tr("Cannot open netCDF file:\n%1\nThis file is not supported by this QGIS plugin.").arg(ncfile.absoluteFilePath()));
        STOP_TIMER(open_file);
        return;
    }
    m_grid_file.push_back(grid_file);
    grid_file = m_grid_file.back();
    _fil_index = m_grid_file.size() - 1;

    stat = grid_file->read();
    STOP_TIMER(open_file);
    START_TIMER(activate_layers);
    activate_layers();
    STOP_TIMER(activate_layers);
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::open_file_his_cf()
{
    QString fname;
    QString * str = new QString();
    QFileDialog * fd = new QFileDialog();
    QStringList * list = new QStringList();

    str->clear();
    str->append("CF Time Series");
    str->append(" (*_his.nc)");
    list->append(*str);
    str->clear();

    str->append("netCDF files");
    str->append(" (*.nc)");
    list->append(*str);
    str->clear();

    str->append("All files");
    str->append(" (*.*)");
    list->append(*str);

    fd->setWindowTitle("Open Climate and Forecast compliant time series file");
    fd->setNameFilters(*list);
    fd->selectNameFilter(list->at(0));
    fd->setFileMode(QFileDialog::ExistingFiles);  // Enable multiple file selection at once

    QDir path("d:/mooiman/home/models/delft3d/GIS/grids/test_qgis");
    if (path.exists())
    {
        fd->setDirectory(path);
    }

    bool canceled = fd->exec() != QDialog::Accepted;
    if (!canceled)
    {
        QStringList * QFilenames = new QStringList();
        QFilenames->append(fd->selectedFiles());

        this->pgBar->show();
        this->pgBar->setMaximum(1000);
        this->pgBar->setValue(0);

        for (QStringList::Iterator it = QFilenames->begin(); it != QFilenames->end(); ++it) {
            fname = *it;
            open_file_his_cf(QFileInfo(fname));
        }

        this->pgBar->setValue(1000);
        if (this->pgBar->value() == this->pgBar->maximum())
        {
            this->pgBar->hide();
        }
    }
    delete fd;
    delete str;
}
void qgis_umesh::open_file_his_cf(QFileInfo ncfile)
{
    int ncid;
    auto start = std::chrono::steady_clock::now();
    char * fname = strdup(ncfile.absoluteFilePath().toUtf8());
    int status = nc_open(fname, NC_NOWRITE, &ncid);
    (void)nc_close(ncid);
    if (status != NC_NOERR)
    {
        QMessageBox::warning(0, tr("Warning"), tr("Cannot open netCDF file:\n%1\nThis file is not supported by this QGIS plugin.").arg(ncfile.absoluteFilePath()));
        return;
    }
    free(fname);
    fname = nullptr;
    _his_cf_fil_index++;
    m_his_cf_file.push_back(new HISCF(ncfile, this->pgBar));
    HISCF * _his_cf_file = m_his_cf_file.back();
    //QMessageBox::warning(0, tr("Warning"), tr("netCDF file opened:\n%1.").arg(ncfile.absoluteFilePath()));
    _his_cf_file->read();
    activate_observation_layers();

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapse_time = end - start;
    QString msg = QString(tr("Timing reading meta data from netCDF file \"%1\": %2 [sec]").arg(ncfile.fileName()).arg(elapse_time.count()));
    QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::open_file_mdu()
{
    QString * str = new QString();
    QFileDialog * fd = new QFileDialog();
    QStringList * list = new QStringList();

    str->clear();
    str->append("D-Flow FM json");
    str->append(" (*_mdu.json)");
    list->append(*str);
    str->clear();
    str->append("All files");
    str->append(" (*.*)");
    list->append(*str);

    fd->setWindowTitle("Open D-Flow FM MDU json file");
    fd->setNameFilters(*list);
    fd->selectNameFilter(list->at(0));
    fd->setFileMode(QFileDialog::ExistingFiles);  // Enable multiple file selection at once

    QDir path("d:/mooiman/home/models/delft3d/GIS/grids/test_qgis");
    if (path.exists())
    {
        fd->setDirectory(path);
    }

    bool canceled = fd->exec() != QDialog::Accepted;
    if (!canceled)
    {
        QStringList * QFilenames = new QStringList();
        QFilenames->append(fd->selectedFiles());

        this->pgBar->show();
        this->pgBar->setMaximum(1000);
        this->pgBar->setValue(0);

        for (QStringList::Iterator it = QFilenames->begin(); it != QFilenames->end(); ++it) {
            open_file_mdu(QFileInfo(*it));
        }

        this->pgBar->setValue(1000);
        if (this->pgBar->value() == this->pgBar->maximum())
        {
            this->pgBar->hide();
        }

    }
    delete fd;
    delete str;
}
void qgis_umesh::open_file_mdu(QFileInfo jsonfile)
{
    std::string fname = jsonfile.absoluteFilePath().toStdString();
    JSON_READER * pt_mdu = new JSON_READER(fname);
    if (pt_mdu == nullptr)
    {
        QMessageBox::warning(0, tr("Warning"), tr("Cannot open JSON file:\n%1.").arg(jsonfile.absoluteFilePath()));
        return;
    }
    m_mdu_files.push_back(pt_mdu);
    //MDU * _mdu_file = new MDU(jsonfile, this->pgBar);
    std::string json_key = "data.geometry.netfile";
    std::vector<std::string> ncfile;
    long status = pt_mdu->get(json_key, ncfile);
    QString mesh;
    if (ncfile.size() == 1)
    {
        mesh = jsonfile.absolutePath() + "/" + QString::fromStdString(ncfile[0]);
        //QMessageBox::warning(0, tr("Warning"), tr("JSON file opened: %1\nMesh file opened: %2.").arg(jsonfile.absoluteFilePath()).arg(mesh));
        openFile(QFileInfo(mesh));
    }
    else
    {
        QString qname = QString::fromStdString(pt_mdu->get_filename());
        QString msg = QString(tr("No UGRID mesh file given.\nTag \"%2\" does not exist in file \"%1\".").arg(QString::fromStdString(json_key)).arg(qname));
        QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Warning, true);
        return;
    }
    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
    QgsLayerTreeGroup * myGroup = treeRoot->findGroup(QString("UGRID - %1").arg(jsonfile.fileName()));
    if (myGroup == nullptr)
    {
        QString name = QString("JSON - %1").arg(jsonfile.fileName());
        myGroup = treeRoot->insertGroup(0, name);
        myGroup->setExpanded(true);  // true is the default
        myGroup->setItemVisibilityChecked(true);
        //QMessageBox::warning(0, "Message", QString("Create group: %1.").arg(name));
        myGroup->setItemVisibilityCheckedRecursive(true);
    }
//------------------------------------------------------------------------------
    int i_json_key = 0;
    int n_json_key = 11;
    this->pgBar->setMaximum(1000);
    this->pgBar->setValue(i_json_key*1000/n_json_key);
    this->pgBar->show();

    json_key = "data.output.obsfile";
    std::vector<std::string> obs_file;
    status = pt_mdu->get(json_key, obs_file);
    if (obs_file.size() != 0 && obs_file[0] != "null")
    {
        for (int i = 0; i < obs_file.size(); ++i)
        {
            QString obser_file = jsonfile.absolutePath() + "/" + QString::fromStdString(obs_file[i]);
            if (!QFileInfo(obser_file).exists())
            {
                QString qname = QString::fromStdString(pt_mdu->get_filename());
                QString msg = QString(tr("Observation points are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
            else
            {
                fname = obser_file.toStdString();
                JSON_READER * pt_obs = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping * mapping;
                mapping = grid_file->get_mapping();
                create_vector_layer_observation_point(grid_file, pt_obs, mapping->epsg, myGroup);  // i.e. a JSON file
            }
        }
    }
//------------------------------------------------------------------------------
    i_json_key += 1;
    this->pgBar->setValue(i_json_key * 1000 / n_json_key);
    json_key = "data.external_forcing.extforcefile";
    std::vector<std::string> extold_file_name;  // There is just one name, so size should be 1
    status = pt_mdu->get(json_key, extold_file_name);

    if (extold_file_name.size() != 0 && extold_file_name[0] != "null" )
    {
        for (int i = 0; i < extold_file_name.size(); ++i)
        {
            QString extold_file = jsonfile.absolutePath() + "/" + QString::fromStdString(extold_file_name[i]);
            if (!QFileInfo(extold_file).exists())
            {
                QString qname = QString::fromStdString(pt_mdu->get_filename());
                QString msg = QString(tr("External forcing file (old format) is skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
            else
            {
                fname = extold_file.toStdString();
                JSON_READER * pt_extold_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping * mapping;
                mapping = grid_file->get_mapping();
                create_vector_layer_1D_external_forcing(grid_file, pt_extold_file, mapping->epsg, myGroup);  // i.e. a JSON file
            }
        }
    }
//------------------------------------------------------------------------------
    i_json_key += 1;
    this->pgBar->setValue(i_json_key * 1000 / n_json_key);
    json_key = "data.external_forcing.extforcefilenew";
    std::vector<std::string> ext_file_name;  // There is just one name, so size should be 1
    status = pt_mdu->get(json_key, ext_file_name);

    if (ext_file_name.size() != 0 && ext_file_name[0] != "null")
    {
        for (int i = 0; i < ext_file_name.size(); ++i)
        {
            QString ext_file = jsonfile.absolutePath() + "/" + QString::fromStdString(ext_file_name[i]);
            if (!QFileInfo(ext_file).exists())
            {
                QString qname = QString::fromStdString(pt_mdu->get_filename());
                QString msg = QString(tr("External forcings are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
            else
            {
                fname = ext_file.toStdString();
                JSON_READER * pt_ext_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_mapping();
                create_vector_layer_1D_external_forcing(grid_file, pt_ext_file, mapping->epsg, myGroup);  // i.e. a JSON file
            }
        }
    }
    //------------------------------------------------------------------------------
    i_json_key += 1;
    this->pgBar->setValue(i_json_key * 1000 / n_json_key);
    json_key = "data.geometry.structurefile";
    std::vector<std::string> structure_file_name;  // There is just one name, so size should be 1
    status = pt_mdu->get(json_key, structure_file_name);
    if (structure_file_name.size() != 0 && structure_file_name[0] != "null")
    {
        for (int i = 0; i < structure_file_name.size(); ++i)
        {
            QString struct_file = jsonfile.absolutePath() + "/" + QString::fromStdString(structure_file_name[i]);
            if (!QFileInfo(struct_file).exists())
            {
                QString qname = QString::fromStdString(pt_mdu->get_filename());
                QString msg = QString(tr("Structures are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
            else
            {
                fname = struct_file.toStdString();
                JSON_READER * pt_struct_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_mapping();
                create_vector_layer_structure(grid_file, pt_struct_file, mapping->epsg, myGroup);  // i.e. a JSON file
            }
        }
    }
    //------------------------------------------------------------------------------
    i_json_key += 1;
    this->pgBar->setValue(i_json_key * 1000 / n_json_key);
    json_key = "data.geometry.drypointsfile";
    std::vector<std::string> drypoints_file_name;  // There is just one name, so size should be 1
    status = pt_mdu->get(json_key, drypoints_file_name);
    if (drypoints_file_name.size() != 0 && drypoints_file_name[0] != "null")
    {
        for (int i = 0; i < drypoints_file_name.size(); ++i)
        {
            QString drypoints_file = jsonfile.absolutePath() + "/" + QString::fromStdString(drypoints_file_name[i]);
            if (!QFileInfo(drypoints_file).exists())
            {
                QString qname = QString::fromStdString(pt_mdu->get_filename());
                QString msg = QString(tr("Structures are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
            else
            {
                fname = drypoints_file.toStdString();
                JSON_READER * pt_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_mapping();
                create_vector_layer_drypoints(grid_file, pt_file, mapping->epsg, myGroup);  // i.e. a JSON file
            }
        }
    }
    //------------------------------------------------------------------------------
    i_json_key += 1;
    this->pgBar->setValue(i_json_key * 1000 / n_json_key);
    json_key = "data.geometry.proflocfile";
    std::vector<std::string> prof_loc_file_name;  // There is just one name, so size should be 1
    status = pt_mdu->get(json_key, prof_loc_file_name);
    if (prof_loc_file_name.size() != 0 && prof_loc_file_name[0] != "null")
    {
        for (int i = 0; i < prof_loc_file_name.size(); ++i)
        {
            QString abs_fname = jsonfile.absolutePath() + "/" + QString::fromStdString(prof_loc_file_name[i]);
            if (!QFileInfo(abs_fname).exists())
            {
                QString qname = QString::fromStdString(pt_mdu->get_filename());
                QString msg = QString(tr("Structures are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
            else
            {
                fname = abs_fname.toStdString();
                JSON_READER * pt_profloc = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_mapping();
                create_vector_layer_sample_point(grid_file, pt_profloc, mapping->epsg, myGroup);  // i.e. a JSON file
            }
        }
    }
    //------------------------------------------------------------------------------
    i_json_key += 1;
    this->pgBar->setValue(i_json_key * 1000 / n_json_key);
    json_key = "data.output.crsfile";
    std::vector<std::string> cross_section_file_name;  // There is just one name, so size should be 1
    status = pt_mdu->get(json_key, cross_section_file_name);
    if (cross_section_file_name.size() != 0 && cross_section_file_name[0] != "null")
    {
        for (int i = 0; i < cross_section_file_name.size(); ++i)
        {
            QString obs_cross_file = jsonfile.absolutePath() + "/" + QString::fromStdString(cross_section_file_name[i]);
            if (!QFileInfo(obs_cross_file).exists())
            {
                QString qname = QString::fromStdString(pt_mdu->get_filename());
                QString msg = QString(tr("Observation cross-sections are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
            else
            {
                fname = obs_cross_file.toStdString();
                JSON_READER * pt_obs_cross_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_mapping();
                create_vector_layer_observation_cross_section(grid_file, pt_obs_cross_file, mapping->epsg, myGroup);  // i.e. a JSON file
            }
        }
    }
//------------------------------------------------------------------------------
    i_json_key += 1;
    this->pgBar->setValue(i_json_key * 1000 / n_json_key);
    json_key = "data.geometry.thindamfile";
    std::vector<std::string> thin_dam_file_name;  // There is just one name, so size should be 1
    status = pt_mdu->get(json_key, thin_dam_file_name);
    if (thin_dam_file_name.size() != 0 && thin_dam_file_name[0] != "null")
    {
        for (int i = 0; i < thin_dam_file_name.size(); ++i)
        {
            QString thin_dam_file = jsonfile.absolutePath() + "/" + QString::fromStdString(thin_dam_file_name[i]);
            if (!QFileInfo(thin_dam_file).exists())
            {
                QString qname = QString::fromStdString(pt_mdu->get_filename());
                QString msg = QString(tr("Thin dams are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
            else
            {
                fname = thin_dam_file.toStdString();
                JSON_READER * pt_thin_dam_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_mapping();
                create_vector_layer_thin_dams(grid_file, pt_thin_dam_file, mapping->epsg, myGroup);  // i.e. a JSON file
            }
        }
    }
//------------------------------------------------------------------------------
    i_json_key += 1;
    this->pgBar->setValue(i_json_key * 1000 / n_json_key);
    json_key = "data.geometry.fixedweirfile";
    std::vector<std::string> fixed_weir_file_name;  // There is just one name, so size should be 1
    status = pt_mdu->get(json_key, fixed_weir_file_name);
    if (fixed_weir_file_name.size() != 0 && fixed_weir_file_name[0] != "null")
    {
        for (int i = 0; i < fixed_weir_file_name.size(); ++i)
        {
            QString fix_weir_file = jsonfile.absolutePath() + "/" + QString::fromStdString(fixed_weir_file_name[i]);
            if (!QFileInfo(fix_weir_file).exists())
            {
                QString qname = QString::fromStdString(pt_mdu->get_filename());
                QString msg = QString(tr("Thin dams are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
            else
            {
                fname = fix_weir_file.toStdString();
                JSON_READER * pt_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_mapping();
                create_vector_layer_fixed_weir(grid_file, pt_file, mapping->epsg, myGroup);  // i.e. a JSON file
            }
        }
    }
    //------------------------------------------------------------------------------
    i_json_key += 1;
    this->pgBar->setValue(i_json_key * 1000 / n_json_key);
    json_key = "data.geometry.crosslocfile";
    std::vector<std::string> cross_section_location_file_name;  // There is just one name, so size should be 1
    status = pt_mdu->get(json_key, cross_section_location_file_name);
    if (cross_section_location_file_name.size() != 0 && cross_section_location_file_name[0] != "null")
    {
        for (int i = 0; i < cross_section_location_file_name.size(); ++i)
        {
            QString full_file_name = jsonfile.absolutePath() + "/" + QString::fromStdString(cross_section_location_file_name[i]);
            if (!QFileInfo(full_file_name).exists())
            {
                QString qname = QString::fromStdString(pt_mdu->get_filename());
                QString msg = QString(tr("Cross-section locations are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
            else
            {
                fname = full_file_name.toStdString();
                JSON_READER * pt_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_mapping();
                create_vector_layer_1D_cross_section(grid_file, pt_file, mapping->epsg, myGroup);  // i.e. a JSON file
            }
        }
    }
    //------------------------------------------------------------------------------
    i_json_key += 1;
    this->pgBar->setValue(i_json_key * 1000 / n_json_key);
    json_key = "data.geometry.retentionfile";
    std::vector<std::string> retention_location_file_name;  // There is just one name, so size should be 1
    status = pt_mdu->get(json_key, retention_location_file_name);
    if (retention_location_file_name.size() != 0 && retention_location_file_name[0] != "null")
    {
        for (int i = 0; i < retention_location_file_name.size(); ++i)
        {
            QString full_file_name = jsonfile.absolutePath() + "/" + QString::fromStdString(retention_location_file_name[i]);
            if (!QFileInfo(full_file_name).exists())
            {
                QString qname = QString::fromStdString(pt_mdu->get_filename());
                QString msg = QString(tr("Retention locations are skipped.\nTag \"%1\" does not exist in file \"%2\".").arg(QString::fromStdString(json_key)).arg(qname));
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
            }
            else
            {
                fname = full_file_name.toStdString();
                JSON_READER* pt_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_mapping();
                create_vector_layer_1D_retention(grid_file, pt_file, mapping->epsg, myGroup);  // i.e. a JSON file
            }
        }
    }
    this->pgBar->setValue(1000);
    this->pgBar->hide();
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::open_file_link1d2d_json()
{
    QString fname;
    QString * str = new QString();
    QFileDialog * fd = new QFileDialog();
    QStringList * list = new QStringList();

    str->clear();
    str->append("Link1D2D json");
    str->append(" (*_ini.json)");
    list->append(*str);
    str->clear();
    str->append("All files");
    str->append(" (*.*)");
    list->append(*str);

    fd->setWindowTitle("Open Link1D2D json file");
    fd->setNameFilters(*list);
    fd->selectNameFilter(list->at(0));
    fd->setFileMode(QFileDialog::ExistingFiles);  // Enable multiple file selection at once

    QDir path("d:/mooiman/home/models/delft3d/GIS/grids/test_qgis");
    if (path.exists())
    {
        fd->setDirectory(path);
    }

    bool canceled = fd->exec() != QDialog::Accepted;
    if (!canceled)
    {
        QStringList * QFilenames = new QStringList();
        QFilenames->append(fd->selectedFiles());

        this->pgBar->show();
        this->pgBar->setMaximum(1000);
        this->pgBar->setValue(0);

        for (QStringList::Iterator it = QFilenames->begin(); it != QFilenames->end(); ++it) {
            fname = *it;
            open_file_link1d2d_json(fname);
        }

        this->pgBar->setValue(1000);
        if (this->pgBar->value() == this->pgBar->maximum())
        {
            this->pgBar->hide();
        }

    }
    delete fd;
    delete str;
}
void qgis_umesh::open_file_link1d2d_json(QFileInfo jsonfile)
{
    std::string fname = jsonfile.absoluteFilePath().toStdString();
    JSON_READER * pt_link1d2d = new JSON_READER(fname);
    if (pt_link1d2d == nullptr)
    {
        QMessageBox::warning(0, tr("Warning"), tr("Cannot open JSON file:\n%1.").arg(jsonfile.absoluteFilePath()));
        return;
    }

    long epsg = 0;
    create_vector_layer_1D2D_link(pt_link1d2d, epsg);  // i.e. a JSON file
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::open_file_obs_point_json()
{
    QString fname;
    QString * str = new QString();
    QFileDialog * fd = new QFileDialog();
    QStringList * list = new QStringList();

    str->clear();
    str->append("Observation point json");
    str->append(" (*_xyn.json)");
    list->append(*str);
    str->clear();
    str->append("All files");
    str->append(" (*.*)");
    list->append(*str);

    fd->setWindowTitle("Open Observation point json file");
    fd->setNameFilters(*list);
    fd->selectNameFilter(list->at(0));
    fd->setFileMode(QFileDialog::ExistingFiles);  // Enable multiple file selection at once

    QDir path("d:/mooiman/home/models/delft3d/GIS/grids/test_qgis");
    if (path.exists())
    {
        fd->setDirectory(path);
    }

    bool canceled = fd->exec() != QDialog::Accepted;
    if (!canceled)
    {
        QStringList * QFilenames = new QStringList();
        QFilenames->append(fd->selectedFiles());

        this->pgBar->show();
        this->pgBar->setMaximum(1000);
        this->pgBar->setValue(0);

        for (QStringList::Iterator it = QFilenames->begin(); it != QFilenames->end(); ++it) {
            fname = *it;
            open_file_obs_point_json(fname);
        }

        this->pgBar->setValue(1000);
        if (this->pgBar->value() == this->pgBar->maximum())
        {
            this->pgBar->hide();
        }

    }
    delete fd;
    delete str;
}
void qgis_umesh::open_file_obs_point_json(QFileInfo jsonfile)
{
    std::string fname = jsonfile.absoluteFilePath().toStdString();
    JSON_READER * pt_file = new JSON_READER(fname);
    if (pt_file == nullptr)
    {
        QMessageBox::warning(0, tr("Warning"), tr("Cannot open JSON file:\n%1.").arg(jsonfile.absoluteFilePath()));
        return;
    }

    long epsg = 0;
    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();
    QgsLayerTreeGroup * treeGroup;
    treeGroup = get_subgroup(treeRoot, QString("Location point"));

    std::vector<std::string> token = tokenize(pt_file->get_filename(), '/');
    GRID* grid_file = new GRID();
    long status = grid_file->open(QFileInfo(QString::fromStdString(token[token.size()-1])), nullptr);
    if (status == 0) { create_vector_layer_observation_point(grid_file, pt_file, epsg, treeGroup); }  // i.e. a JSON file
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::set_show_map_output()
{
    GRID * grid_file = get_active_grid_file("");
    if (grid_file != nullptr) { show_map_output(grid_file); }
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::start_plotcfts()
{
    QString program_files = QProcessEnvironment::systemEnvironment().value("ProgramFiles", "");
    QString prgm_env = program_files + QString("/deltares/plotcfts");
    QString prgm = program_files + QString("/deltares/plotcfts/plotcfts.exe");
    QDir program_files_dir = QDir(prgm_env);
    if (!program_files_dir.exists())
    {
        QMessageBox::warning(0, tr("Message"), QString(tr("Missing installation directory:\n")) + prgm_env + QString(".\nProgram PlotCFTS will not start."));
        return;
    }
    //QMessageBox::information(0, tr("qgis_umesh::start_plotcfts()"), tr("start_plotcfts."));
    HISCF * _his_cf_file = get_active_his_cf_file("");
    if (_his_cf_file == nullptr)
    {
        QMessageBox::warning(0, tr("Warning"), tr("There is no layer selected which contains a Climate and Forecast compliant time series set.\nTime series plot program PlotCFTS will start without preselected history file."));
    }
    //QgsVectorLayer* theVectorLayer = dynamic_cast<QgsVectorLayer*>(active_layer);
    //QgsFeature feature = theVectorLayer->getFeature();
    QStringList argList;
    QProcess * proc = new QProcess();
    if (_his_cf_file != nullptr)
    {
        argList.append("--ncfile");
        QString fname = _his_cf_file->get_filename().canonicalFilePath();
        argList.append(fname);
    }

    QProcessEnvironment env;
    env.insert("PATH", prgm_env); // Add an environment variable
    proc->setProcessEnvironment(env);
    proc->start(prgm, argList);
    if (proc->state() == QProcess::NotRunning)
    {
        delete proc;
    }
    return;
}
//
//-----------------------------------------------------------------------------
//
GRID * qgis_umesh::get_active_grid_file(QString layer_id)
{
    // Function to find the filename belonging by the highlighted layer,
    // or to which group belongs the highligthed layer,
    // or the highlighted group
    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
    QList <QgsLayerTreeGroup *> groups = treeRoot->findGroups();
    QgsMapLayer * active_layer = NULL;
    GRID * grid_file = nullptr;

    if (layer_id == "")
    {
        active_layer = mQGisIface->activeLayer();
        if (active_layer != nullptr)
        {
            layer_id = active_layer->id();
        }
    }
    if (layer_id != "")
    {
        //QMessageBox::information(0, "Information", QString("qgis_umesh::get_active_layer()\nActive layer: %1.").arg(active_layer->name()));
        // if there is an active layer, belongs it to a Mesh-group?
        QList< QgsLayerTreeGroup * > myGroups = treeRoot->findGroups();
        for (int j = 0; j < _fil_index + 1; j++)
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
                                grid_file = m_grid_file[j];
                            }
                        }
                    }
                }
            }
        }
    }
    return grid_file;
}
//-----------------------------------------------------------------------------
//
HISCF * qgis_umesh::get_active_his_cf_file(QString layer_id)
{
    // Function to find the filename belonging by the highlighted layer,
    // or to which group belongs the highligthed layer,
    // or the highlighted group
    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
    QList <QgsLayerTreeGroup *> groups = treeRoot->findGroups();
    QgsMapLayer * active_layer = NULL;
    HISCF * _his_cf_file = nullptr;

    if (layer_id == "")
    {
        active_layer = mQGisIface->activeLayer();
        if (active_layer != nullptr)
        {
            layer_id = active_layer->id();
        }
        else
        {
            // Probably a group is selected, is it the correct one
            // for (int j = 0; j < _his_cf_fil_index + 1; j++)
            for (int j = 0; j < 0; j++)
            {
                QgsLayerTreeGroup * myGroup = treeRoot->findGroup(QString("History - %1").arg(m_his_cf_file[_his_cf_fil_index]->get_filename().fileName()));
                if (myGroup != nullptr)
                {
                   // _his_cf_file = _his_cf_files[j];  ToDo : Needed, what group is selected
                }
            }
        }
    }
    if (layer_id != "")
    {
        //QMessageBox::information(0, "Information", QString("qgis_umesh::get_active_layer()\nActive layer: %1.").arg(active_layer->name()));
        // if there is an active layer, belongs it to a Mesh-group?
        for (int j = 0; j < _his_cf_fil_index + 1; j++)
        {
            QgsLayerTreeGroup * myGroup = treeRoot->findGroup(QString("History - %1").arg(m_his_cf_file[_his_cf_fil_index]->get_filename().fileName()));
            if (myGroup != nullptr)
            {
                QList <QgsLayerTreeLayer *> layers = myGroup->findLayers();
                for (int k = 0; k < layers.length(); k++)
                {
                    // belong the active layer to this group?
                    if (layers[k]->layerId() == layer_id)
                    {
                        //QMessageBox::information(0, "Information", QString("qgis_umesh::get_active_layer()\nGroup name: %1\nActive layer: %2.").arg(myGroup->name()).arg(active_layer->name()));
                        // get the full file name
                        _his_cf_file = m_his_cf_file[j];
                    }
                }
            }
        }
    }
    //else
    //{
    //    QMessageBox::information(0, "Information", QString("No layer selected, determination of output files is not possible."));
    //}

    return _his_cf_file;
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::activate_layers()
{
    struct _ntw_nodes * ntw_nodes;
    struct _ntw_edges * ntw_edges;
    struct _ntw_geom * ntw_geom;
    struct _mesh1d * mesh_1d;
    struct _mesh2d * mesh_2d;
    struct _mesh_contact * mesh_contact;

    if (_fil_index == -1)
    {
        return;
    }

    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible

    if (mainAction->isChecked())
    {
        QList <QgsLayerTreeGroup *> groups = treeRoot->findGroups();
        for (int i = 0; i< groups.length(); i++)
        {
            //QMessageBox::warning(0, "Message", QString("_fil_index: %1+1.").arg(_fil_index+1));
            for (int j = 0; j < _fil_index+1; j++)
            {
                QgsLayerTreeGroup * myGroup = treeRoot->findGroup(QString("UGRID Mesh - %1").arg(j + 1));
                if (myGroup != nullptr)
                {
                    myGroup->setItemVisibilityChecked(true);
                }
            }
        }

        QgsLayerTreeGroup * treeGroup = treeRoot->findGroup(QString("UGRID - %1").arg(m_grid_file[_fil_index]->get_filename().fileName()));
        if (treeGroup == nullptr)
        {
            QString fname = m_grid_file[_fil_index]->get_filename().fileName();
            QString name = QString("UGRID - %1").arg(fname);
            treeGroup = treeRoot->insertGroup(_fil_index, name);  // set this group on top if _fil_index == 0
            treeGroup->setExpanded(true);  // true is the default
            treeGroup->setItemVisibilityChecked(true);
            treeGroup->setItemVisibilityCheckedRecursive(true);
        }

        if (_fil_index != -1)
        {
            GRID* grid_file = m_grid_file[_fil_index];
            struct _mapping* mapping;
            mapping = grid_file->get_mapping();
            QString fname = grid_file->get_filename().canonicalFilePath();
            if (mapping->epsg == 0)
            {
                QString msg = QString("%1\nThe CRS code on file \'%2\' is not known.\nThe CRS code is set to the same CRS code as the presentation map (see lower right corner).\nPlease change the CRS code after loading the file, if necessary.").arg(grid_file->get_filename().fileName()).arg(fname);
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Warning, true );

                // get the crs of the presentation (lower right corner of the qgis main-window)
                QgsCoordinateReferenceSystem _crs = QgsProject::instance()->crs();
                QString epsg_code = _crs.authid();
                QStringList parts = epsg_code.split(':');
                long epsg = parts.at(1).toInt();
                grid_file->set_grid_mapping_epsg(epsg, epsg_code.toUtf8().constData());
            }
            fname = grid_file->get_filename().fileName();  // just the filename

            int pgbar_value = 500;  // start of pgbar counter

                                    // Mesh 1D edges and mesh 1D nodes

            mesh_1d = grid_file->get_mesh_1d();
            //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D nodes."));
            if (mesh_1d != nullptr)
            {
                create_vector_layer_nodes(fname, QString("Mesh1D nodes"), mesh_1d->node[0], mapping->epsg, treeGroup);
                QList <QgsLayerTreeLayer*> tmp_layers = treeGroup->findLayers();
                for (int i = 0; i < tmp_layers.size(); i++)
                {
                    if (tmp_layers[i]->name() == "Mesh1D nodes")
                    {
                        tmp_layers[i]->setItemVisibilityChecked(false);
                    }
                }
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D edges."));
                create_vector_layer_edges(fname, QString("Mesh1D edges"), mesh_1d->node[0], mesh_1d->edge[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
            }

            // Mesh1D Topology Edges between connection nodes, Network Connection node, Network geometry
            ntw_nodes = grid_file->get_connection_nodes();
            ntw_edges = grid_file->get_network_edges();
            ntw_geom = grid_file->get_network_geometry();

            if (ntw_nodes != nullptr)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D Connection nodes."));
                QString layer_name = QString("Mesh1D Connection nodes");
                create_vector_layer_nodes(fname, layer_name, ntw_nodes->node[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);

                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D geometry."));
                create_vector_layer_geometry(fname, QString("Mesh1D geometry"), ntw_geom, mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);

                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D Topology edges."));
                create_vector_layer_edges(fname, QString("Mesh1D Topology edges"), ntw_nodes->node[0], ntw_edges->edge[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
            }

            // Mesh1D Topology Edges between connection nodes, Network Connection node, Network geometry

            mesh_contact = grid_file->get_mesh_contact();

            if (mesh_contact != nullptr)
            {
                create_vector_layer_edges(fname, QString("Mesh contact edges"), mesh_contact->node[0], mesh_contact->edge[0], mapping->epsg, treeGroup);
            }

            // Mesh 2D edges and Mesh 2D nodes

            mesh_2d = grid_file->get_mesh_2d();
            if (mesh_2d != nullptr)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Mesh2D nodes."));
                if (mesh_2d->face[0]->count > 0)
                {
                    create_vector_layer_nodes(fname, QString("Mesh2D faces"), mesh_2d->face[0], mapping->epsg, treeGroup);
                    pgbar_value += 10;
                    this->pgBar->setValue(pgbar_value);
                }

                //QMessageBox::warning(0, tr("Warning"), tr("Mesh2D nodes."));
                create_vector_layer_nodes(fname, QString("Mesh2D nodes"), mesh_2d->node[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);

                if (mesh_2d->edge[0]->count > 0)
                {
                    create_vector_layer_edges(fname, QString("Mesh2D edges"), mesh_2d->node[0], mesh_2d->edge[0], mapping->epsg, treeGroup);
                    pgbar_value += 10;
                    this->pgBar->setValue(pgbar_value);
                }
            }
            pgbar_value = 600;  // start of pgbar counter

            this->pgBar->setValue(pgbar_value);

            //
            // get the time independent variables and list them in the layer-panel as treegroup
            //
            struct _mesh_variable * var = grid_file->get_variables();

            bool time_independent_data = false;
            if (mesh_2d != nullptr && var != nullptr)
            {
                for (int i = 0; i < var->nr_vars; i++)
                {
                    if (!var->variable[i]->time_series)
                    {
                        time_independent_data = true;
                        break;
                    }
                }
                if (time_independent_data)
                {
                    QString name = QString("Time independent data");
                    treeGroup->insertGroup(0, name);  // insert at top of group
                    QgsLayerTreeGroup * subTreeGroup = treeGroup->findGroup(name);

                    subTreeGroup->setExpanded(false);  // true is the default
                    subTreeGroup->setItemVisibilityChecked(true);
                    //QMessageBox::warning(0, "Message", QString("Create group: %1.").arg(name));
                    subTreeGroup->setItemVisibilityCheckedRecursive(true);

                    //cb->blockSignals(true);
                    for (int i = 0; i < var->nr_vars; i++)
                    {
                        if (!var->variable[i]->time_series && var->variable[i]->coordinates != "")
                        {
                            QString qname = QString::fromStdString(var->variable[i]->long_name).trimmed();
                            QString var_name = QString::fromStdString(var->variable[i]->var_name).trimmed();
                            if (var->variable[i]->location == "edge" && var->variable[i]->topology_dimension == 2 && var->variable[i]->nc_type == NC_DOUBLE)
                            {
                                DataValuesProvider2D<double>std_data_at_edge = grid_file->get_variable_values(var->variable[i]->var_name);
                                double * z_value = std_data_at_edge.GetValueAtIndex(0, 0);
                                create_vector_layer_data_on_edges(fname, var->variable[i], mesh_2d->node[0], mesh_2d->edge[0], z_value, mapping->epsg, subTreeGroup);
                            }
                            if (var->variable[i]->location == "edge" && var->variable[i]->topology_dimension == 2 && var->variable[i]->nc_type == NC_INT)
                            {
                                QString name1 = QString("Edge type");
                                subTreeGroup->addGroup(name1);
                                QgsLayerTreeGroup * sGroup = treeGroup->findGroup(name1);

                                DataValuesProvider2D<double>std_data_at_edge = grid_file->get_variable_values(var->variable[i]->var_name);
                                double * z_value = std_data_at_edge.GetValueAtIndex(0, 0);
                                create_vector_layer_edge_type(fname, var->variable[i], mesh_2d->node[0], mesh_2d->edge[0], z_value, mapping->epsg, sGroup);
                            }
                            if (var->variable[i]->location == "face" && var->variable[i]->topology_dimension == 2 && var->variable[i]->nc_type == NC_DOUBLE)
                            {
                                if (var->variable[i]->sediment_index == -1)
                                {
                                    DataValuesProvider2D<double>std_data_at_face = grid_file->get_variable_values(var->variable[i]->var_name, var->variable[i]->sediment_index);
                                    double * z_value = std_data_at_face.GetValueAtIndex(0, 0);
                                    create_vector_layer_data_on_nodes(fname, var->variable[i], mesh_2d->face[0], z_value, mapping->epsg, subTreeGroup);
                                }
                                else
                                {
                                    QgsLayerTreeGroup * Group = get_subgroup(subTreeGroup, QString("Sediment"));

                                    DataValuesProvider2D<double>std_data_at_face = grid_file->get_variable_values(var->variable[i]->var_name, var->variable[i]->sediment_index);
                                    double * z_value = std_data_at_face.GetValueAtIndex(var->variable[i]->sediment_index, 0);
                                    create_vector_layer_data_on_nodes(fname, var->variable[i], mesh_2d->face[0], z_value, mapping->epsg, Group);
                                    Group->setExpanded(false);
                                }
                            }
                            if (var->variable[i]->location == "node" && var->variable[i]->topology_dimension == 2 && var->variable[i]->nc_type == NC_DOUBLE)
                            {
                                DataValuesProvider2D<double>std_data_at_node = grid_file->get_variable_values(var->variable[i]->var_name);
                                double * z_value = std_data_at_node.GetValueAtIndex(0, 0);
                                create_vector_layer_data_on_nodes(fname, var->variable[i], mesh_2d->node[0], z_value, mapping->epsg, subTreeGroup);
                            }
                        }
                    }
                    //cb->blockSignals(false);
                }
            }  // end mesh_2d != nullptr
        }

    }
    else
    {
        QList <QgsLayerTreeGroup *> groups = treeRoot->findGroups();
        for (int i = 0; i< groups.length(); i++)
        {
            QString name = groups[i]->name();
            QgsLayerTreeGroup * myGroup = treeRoot->findGroup(name);
            myGroup->setItemVisibilityChecked(false);
        }
    }
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::activate_observation_layers()
{
    std::vector<_location_type *> obs_type;

    if (_his_cf_fil_index < 0) { return; }  // No history file opened

    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible

    if (mainAction->isChecked())
    {
        QList <QgsLayerTreeGroup *> groups = treeRoot->findGroups();
        for (int i = 0; i< groups.length(); i++)
        {
            //QMessageBox::warning(0, "Message", QString("_fil_index: %1_b1.").arg(_fil_index+1));
            for (int j = 0; j < _his_cf_fil_index + 1; j++)
            {
                QgsLayerTreeGroup * myGroup = treeRoot->findGroup(QString("History - %1").arg(m_his_cf_file[_his_cf_fil_index]->get_filename().fileName()));
                if (myGroup != nullptr)
                {
                    myGroup->setItemVisibilityChecked(true);
                }
            }
        }

        QgsLayerTreeGroup * treeGroup = treeRoot->findGroup(QString("History - %1").arg(m_his_cf_file[_his_cf_fil_index]->get_filename().fileName()));
        if (treeGroup == nullptr)
        {
            QString name = QString("History - %1").arg(m_his_cf_file[_his_cf_fil_index]->get_filename().fileName());
            treeGroup = treeRoot->insertGroup(_his_cf_fil_index, name);
            treeGroup->setExpanded(true);  // true is the default
            treeGroup->setItemVisibilityChecked(true);
            //QMessageBox::warning(0, "Message", QString("Create group: %1.").arg(name));
            treeGroup->setItemVisibilityCheckedRecursive(true);
        }
        if (_his_cf_fil_index != -1)
        {
            HISCF * _his_cf_file = m_his_cf_file[_his_cf_fil_index];
            struct _mapping* mapping;
            mapping = _his_cf_file->get_grid_mapping();
            if (mapping->epsg == 0)
            {
                QString fname = _his_cf_file->get_filename().canonicalFilePath();
                QString msg = QString("%1\nThe CRS code on file \'%2\' is not known.\nThe CRS code is set to the same CRS code as the presentation map (see lower right corner).\nPlease change the CRS code after loading the file, if necessary.").arg(_his_cf_file->get_filename().fileName()).arg(fname);
                QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Warning, true);

                // get the crs of the presentation (lower right corner of the qgis main-window)
                QgsCoordinateReferenceSystem _crs = QgsProject::instance()->crs();
                QString epsg_code = _crs.authid();
                QStringList parts = epsg_code.split(':');
                long epsg = parts.at(1).toInt();
                long status = _his_cf_file->set_grid_mapping_epsg(epsg, epsg_code.toUtf8().constData());
            }
            int pgbar_value = 500;  // start of pgbar counter

                                    // Mesh 1D edges and mesh 1D nodes

            obs_type = _his_cf_file->get_observation_location();
            QString fname = _his_cf_file->get_filename().fileName();
            //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D nodes."));
            if (obs_type.size() > 0)
            {
                for (int i = 0; i < obs_type.size(); i++)
                {
                    if (obs_type[i]->type == OBSERVATION_TYPE::OBS_POINT)
                    {
                        create_vector_layer_observation_point(fname, QString(obs_type[i]->location_long_name), obs_type[i], mapping->epsg, treeGroup);
                    }
                    else if (obs_type[i]->type == OBSERVATION_TYPE::OBS_POLYLINE)
                    {
                        create_vector_layer_observation_polyline(fname, QString(obs_type[i]->location_long_name), obs_type[i], mapping->epsg, treeGroup);
                    }
                    else
                    {
                        QMessageBox::information(0, QString("Information"), QString("Only \'point\' and \'line\' are supported as observation location."));
                    }
                }
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
            }
        }

    }
    else
    {
        QList <QgsLayerTreeGroup *> groups = treeRoot->findGroups();
        for (int i = 0; i< groups.length(); i++)
        {
            QString name = groups[i]->name();
            QgsLayerTreeGroup * myGroup = treeRoot->findGroup(name);
            myGroup->setItemVisibilityChecked(false);
        }
    }
}
//
//-----------------------------------------------------------------------------
//
char* qgis_umesh::stripSpaces(char* string)
{
    //Remove first trailing whitespaces and than leading
    char* s;

    for (s = string + strlen(string) - 1;
        (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n');
        s--);
    *(s + 1) = '\0';

    for (s = string; *s == ' ' || *s == '\t'; s++);

    strcpy(string, s);
    return string;
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::unload()
{
    delete this->mainAction;  // delete main Icon on the Delft3D toolbar tbar
    delete this->tbar;  // delete main icon and combobox with plugins
    unload_vector_layers();

    // clean the Mesh (unstructured) group
    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
    QList< QgsLayerTreeGroup * > groups = treeRoot->findGroups();
    for (int i = 0; i < groups.size(); i++)
    {
        QgsLayerTreeGroup * janm = groups.at(i);
        QString str = janm->name();
        if (str.contains("UGRID - ") ||
            str.contains("History -"))
        {
            janm->setExpanded(false);  // true is the default
            janm->setItemVisibilityChecked(false);
            janm->removeAllChildren();
            treeRoot->removeChildNode(janm);
        }
    }
    for (int i = 0; i < _fil_index; i++)
    {
        //delete[] m_ugrid_file[i];
    }
    if (m_grid_file.size() != 0)
    {
        //delete m_ugrid_file;
    }

    if (mtm_widget != NULL)
    {
        delete mtm_widget;
        mtm_widget = NULL;
    }
    DELETE_TIMERN()
    //QMessageBox::warning(0, tr("Message"), QString("qgis_umesh::unload()."));
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::unload_vector_layers()
{
    // remove scratch layer
    QList< QgsMapLayer * > layers = mQGisIface->mapCanvas()->layers();

    for (int i = 0; i < layers.count(); i++)
    {
        //QMessageBox::warning(0, tr("Message"), QString(tr("Layer name: ")) + layers[i]->name());
        QString jan = layers[i]->name();
        if (jan.contains("Points (Mooiman) vl_1") ||
            jan == QString("Splines(Mooiman) vl_2") ||
            jan == QString("Polylines(Mooiman) vl_3") ||
            jan == QString("Polygons (Mooiman) vl_4"))
        {
            QgsProject::instance()->removeMapLayer(layers[i]);
        }
    }
}

void qgis_umesh::create_vector_layer_nodes(QString fname, QString layer_name, struct _feature * nodes, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
                lMyAttribField << QgsField("Node name", QVariant::String);
                nr_attrib_fields++;
            }
            if (nodes->long_name.size() != 0)
            {
                lMyAttribField << QgsField("Node long name", QVariant::String);
                nr_attrib_fields++;
            }
            lMyAttribField << QgsField("Node Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Node Id (1-based)", QVariant::String);
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
            for (int j = 0; j < nodes->x.size(); j++)
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

            vl->setTitle(layer_name + ": " + fname);

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
void qgis_umesh::create_vector_layer_data_on_edges(QString fname, _variable * var, struct _feature * nodes, struct _edge * edges, double * z_value, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
            lMyAttribField << QgsField("Value", QVariant::Double);

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
            vl->setTitle(layer_name + ": " + fname);

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
void qgis_umesh::create_vector_layer_edge_type(QString fname, _variable * var, struct _feature * nodes, struct _edge * edges, double * z_value, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
                lMyAttribField << QgsField("Value", QVariant::Double);

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
                    vl->setTitle(layer_name + ": " + fname);

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
void qgis_umesh::create_vector_layer_data_on_nodes(QString fname, _variable * var, struct _feature * nodes, double * z_value, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
            lMyAttribField << QgsField("Value", QVariant::Double);

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
            vl->setTitle(layer_name + ": " + fname);

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
void qgis_umesh::create_vector_layer_geometry(QString fname, QString layer_name, struct _ntw_geom * ntw_geom, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
                lMyAttribField << QgsField("Geometry edge name", QVariant::String);
                nr_attrib_fields++;
            }
            if (ntw_geom->geom[0]->long_name.size() != 0)
            {
                lMyAttribField << QgsField("Geometry edge long name", QVariant::String);
                nr_attrib_fields++;
            }
            lMyAttribField << QgsField("Geometry Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Geometry Id (1-based)", QVariant::String);
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
            vl->setTitle(layer_name + ": " + fname);

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
void qgis_umesh::create_vector_layer_edges(QString fname, QString layer_name, struct _feature * nodes, struct _edge * edges, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
                lMyAttribField << QgsField("Edge length", QVariant::Double);
                nr_attrib_fields++;
            }
            if (edges->name.size() > 0)
            {
                lMyAttribField << QgsField("Edge name", QVariant::String);
                nr_attrib_fields++;
            }
            if (edges->long_name.size() > 0)
            {
                lMyAttribField << QgsField("Edge long name", QVariant::String);
                nr_attrib_fields++;
            }
            lMyAttribField << QgsField("Edge Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Edge Id (1-based)", QVariant::String);
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
            vl->setTitle(layer_name + ": " + fname);

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
void qgis_umesh::create_vector_layer_observation_point(QString fname, QString layer_name, _location_type * obs_points, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
                lMyAttribField << QgsField(obs_points->location_long_name, QVariant::String);
                nr_attrib_fields++;
            }
            lMyAttribField << QgsField("Observation point Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Observation point Id (1-based)", QVariant::String);
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
            vl->setTitle(layer_name + ": " + fname);

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
void qgis_umesh::create_vector_layer_observation_polyline(QString fname, QString layer_name, _location_type * obs_points, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
                lMyAttribField << QgsField(obs_points->location_long_name, QVariant::String);
                nr_attrib_fields++;
            }
            lMyAttribField << QgsField("Cross section Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Cross section Id (1-based)", QVariant::String);
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
            vl->setTitle(layer_name + ": " + fname);

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
// Create vector layer for the structures defined by the chainage on a branch, so it is a point
void qgis_umesh::create_vector_layer_1D_structure(GRID * ugrid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
        int sum =
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

        if (id.size() != sum)
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
            lMyAttribField << QgsField("Bridge name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Bridge Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Bridge Id (1-based)", QVariant::String);
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
            struct _ntw_geom* ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = ugrid_file->get_network_edges();

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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("Bridge pillars name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Bridge pillars Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Bridge pillars Id (1-based)", QVariant::String);
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
            struct _ntw_geom* ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = ugrid_file->get_network_edges();

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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("Compound structures name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Compound structures Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Compound structures Id (1-based)", QVariant::String);
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
            struct _ntw_geom* ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = ugrid_file->get_network_edges();

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
            int cnt;
            for (int j = 0; j < id.size(); j++)
            {
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
                            branch_name[j] = branch_name[k];
                            chainage[j] = chainage[k];
                        }
                    }

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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("Culvert name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Culvert Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Culvert Id (1-based)", QVariant::String);
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
            struct _ntw_geom* ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = ugrid_file->get_network_edges();

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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("Extra resistance name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Extra resistance Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Extra resistance Id (1-based)", QVariant::String);
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
            struct _ntw_geom* ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = ugrid_file->get_network_edges();

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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("General structure name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("General structure Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("General structure Id (1-based)", QVariant::String);
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
            struct _ntw_geom* ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = ugrid_file->get_network_edges();

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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("Inverted siphon name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Inverted siphon Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Inverted siphon Id (1-based)", QVariant::String);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->blockSignals(true);
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            struct _ntw_geom* ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = ugrid_file->get_network_edges();

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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("Orifice name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Orifice Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Orifice Id (1-based)", QVariant::String);
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
            struct _ntw_geom* ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = ugrid_file->get_network_edges();

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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("Pump name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Pump Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Pump Id (1-based)", QVariant::String);
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
            struct _ntw_geom* ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = ugrid_file->get_network_edges();

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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("River weir name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("River weir Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("River weir Id (1-based)", QVariant::String);
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
            struct _ntw_geom* ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = ugrid_file->get_network_edges();

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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("Weir name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Weir Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Weir Id (1-based)", QVariant::String);
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
            struct _ntw_geom* ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges* ntw_edges = ugrid_file->get_network_edges();

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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
void qgis_umesh::create_vector_layer_observation_point(GRID * grid_file, JSON_READER * pt_structures, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
// observation point defined by coordinate reference systeem (crs)
void qgis_umesh::create_vector_layer_crs_observation_point(GRID * ugrid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    Q_UNUSED(ugrid_file);
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

    lMyAttribField << QgsField("Observation point name", QVariant::String);
    lMyAttribField << QgsField("Observation point Id (0-based)", QVariant::String);
    lMyAttribField << QgsField("Observation point Id (1-based)", QVariant::String);

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
    vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
// observation point defined by chainage
void qgis_umesh::create_vector_layer_chainage_observation_point(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
        lMyAttribField << QgsField("Observation point name", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation point Id (0-based)", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation point Id (1-based)", QVariant::String);
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
        vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
// sample point (x, y,z) defined by coordinate reference systeem (crs)
void qgis_umesh::create_vector_layer_sample_point(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
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

    lMyAttribField << QgsField("Sample point value", QVariant::Double);
    lMyAttribField << QgsField("Sample point Id (0-based)", QVariant::String);
    lMyAttribField << QgsField("Sample point Id (1-based)", QVariant::String);

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
    vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
// Observation cross-section (D-Flow FM) filename given in mdu-file
void qgis_umesh::create_vector_layer_observation_cross_section(GRID * grid_file, JSON_READER *prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
//
// Observation cross-section (D-Flow 1D) filename given in mdu-file, point object when 1D simulation
void qgis_umesh::create_vector_layer_1D_observation_cross_section(GRID* ugrid_file, JSON_READER* prop_tree, long epsg_code, QgsLayerTreeGroup* treeGroup)
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
        lMyAttribField << QgsField("Observation cross-sections name", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation cross-sections point Id (0-based)", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation cross-sections point Id (1-based)", QVariant::String);
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

        struct _ntw_geom* ntw_geom = ugrid_file->get_network_geometry();
        struct _ntw_edges* ntw_edges = ugrid_file->get_network_edges();

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
        vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
//
// Observation cross-section (D-Flow FM) filename given in mdu-file, line object when 2D simulation
void qgis_umesh::create_vector_layer_2D_observation_cross_section(GRID* grid_file, JSON_READER* prop_tree, long epsg_code, QgsLayerTreeGroup* treeGroup)
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
        lMyAttribField << QgsField("Observation cross-section name", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation cross-section Id (0-based)", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation cross-section Id (1-based)", QVariant::String);
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
        vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
void qgis_umesh::create_vector_layer_structure(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
        lMyAttribField << QgsField("Structure name", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Structure Id (0-based)", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Structure Id (1-based)", QVariant::String);
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
            vl->setTitle(layer_name + ": " + QString::fromStdString(fname[i]));
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
void qgis_umesh::create_vector_layer_drypoints(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
        lMyAttribField << QgsField(attrib_label, QVariant::String);
        nr_attrib_fields++;

        attrib_label = layer_name + " Id (0-based)";
        lMyAttribField << QgsField(attrib_label, QVariant::String);
        nr_attrib_fields++;

        attrib_label = layer_name + " Id (1-based)";
        lMyAttribField << QgsField(attrib_label, QVariant::String);
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
        vl->setTitle(layer_name + ": " +  QString::fromStdString(token[token.size() - 1]));

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
void qgis_umesh::create_vector_layer_1D_external_forcing(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
            lMyAttribField << QgsField("Source Sink name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Source Sink Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Source Sink Id (1-based)", QVariant::String);
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
                vl_points->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
                vl_lines->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));
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
            lMyAttribField << QgsField("Boundary name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Boundary Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Boundary Id (1-based)", QVariant::String);
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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("Lateral area name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Lateral area Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Lateral area Id (1-based)", QVariant::String);
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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("Lateral point name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Lateral point rotation", QVariant::Double);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Lateral point Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Lateral point Id (1-based)", QVariant::String);
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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
            lMyAttribField << QgsField("Boundary name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Boundary point Id (0-based)", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Boundary point Id (1-based)", QVariant::String);
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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
void qgis_umesh::create_vector_layer_thin_dams(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
        lMyAttribField << QgsField(attrib_label, QVariant::String);
        nr_attrib_fields++;

        attrib_label = layer_name + " Id (0-based)";
        lMyAttribField << QgsField(attrib_label, QVariant::String);
        nr_attrib_fields++;

        attrib_label = layer_name + " Id (1-based)";
        lMyAttribField << QgsField(attrib_label, QVariant::String);
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
        vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
void qgis_umesh::create_vector_layer_fixed_weir(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    int status = -1;
    std::vector<std::string> tmp_line_name;
    std::string json_key = "data.path.name";
    status = prop_tree->get(json_key, tmp_line_name);
    if (tmp_line_name.size() != 0)
    {
        START_TIMERN(create_vector_layer_fixed_weir);
        QgsLayerTreeGroup * subTreeGroup;
        subTreeGroup = get_subgroup(treeGroup, QString("Area"));
        QString layer_name = "Fix weir";
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
        lMyAttribField << QgsField(attrib_label, QVariant::String);
        nr_attrib_fields++;

        attrib_label = layer_name + " Id (0-based)";
        lMyAttribField << QgsField(attrib_label, QVariant::String);
        nr_attrib_fields++;

        attrib_label = layer_name + " Id (1-based)";
        lMyAttribField << QgsField(attrib_label, QVariant::String);
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
        vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
void qgis_umesh::create_vector_layer_1D_cross_section(GRID * grid_file, JSON_READER * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
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
        lMyAttribField << QgsField("Cross-section name", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Cross-section Id (0-based)", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Cross-section Id (1-based)", QVariant::String);
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
        vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
void qgis_umesh::create_vector_layer_1D_retention(GRID * grid_file, JSON_READER* prop_tree, long epsg_code, QgsLayerTreeGroup* treeGroup)
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
        lMyAttribField << QgsField("Retention name", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Retention Id (0-based)", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Retention Id (1-based)", QVariant::String);
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
        vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
void qgis_umesh::create_vector_layer_1D2D_link(JSON_READER * prop_tree, long epsg_code)
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

            lMyAttribField << QgsField("Link Id (0-based)", QVariant::String);
            lMyAttribField << QgsField("Link Id (1-based)", QVariant::String);

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
            vl->setTitle(layer_name + ": " + QString::fromStdString(token[token.size() - 1]));

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
long qgis_umesh::compute_location_along_geometry(struct _ntw_geom * ntw_geom, struct _ntw_edges * ntw_edges, std::string branch_name, double chainage_node, double * xp, double * yp, double * rotation)
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
                //QMessageBox::warning(0, "qgis_umesh::compute_location_along_geometry", QString("UGRID::determine_computational_node_on_geometry()\nBranch(%3). Offset %1 is larger then branch length %2.\n").arg(offset).arg(branch_length).arg(branch+1));
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
long qgis_umesh::find_location_boundary(struct _ntw_nodes * ntw_nodes, std::string bnd_name, double * xp, double * yp)
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


void qgis_umesh::add_layer_to_group(QgsVectorLayer * vl, QgsLayerTreeGroup * treeGroup)
{
    START_TIMERN(add_layer_to_group);

    QgsLayerTree * root = QgsProject::instance()->layerTreeRoot();

    QgsMapLayer * map_layer = QgsProject::instance()->addMapLayer(vl, false);
    treeGroup->addLayer(map_layer);

    QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
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
//
QgsLayerTreeGroup * qgis_umesh::get_subgroup(QgsLayerTreeGroup * treeGroup, QString sub_group_name)
{
    QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
    QgsLayerTreeGroup * subTreeGroup = treeGroup->findGroup(sub_group_name);
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
std::vector<std::string> qgis_umesh::tokenize(const std::string& s, char c) {
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
std::vector<std::string> qgis_umesh::tokenize(const std::string& s, std::size_t count)
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

//
void qgis_umesh::dummy_slot()
{
    QMessageBox::information(0, tr("Message"), QString("Dummy slot called."));
}

///////////////////////////////////////////////////////////////////////////////
/**
 * Required extern functions needed  for every plugin
 * These functions can be called prior to creating an instance
 * of the plugin class
 */

// Return the type (either UI or MapLayer plugin)
QGISEXTERN int type()
{
    //QgsMessageLog::logMessage("::type()", "Hello World", Qgis::Info, true);
    return qgis_umesh::s_plugin_type; // eerste, na selectie in plugin manager
}

// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin* classFactory(QgisInterface* iface)
{
    //QgsMessageLog::logMessage("::classFactory()", "QGIS umesh", Qgis::Info, true);
    qgis_umesh * p = new qgis_umesh(iface);
    return (QgisPlugin*) p; // tweede na selectie in plugin manager
}

QGISEXTERN const QString* name()
{
    //QgsMessageLog::logMessage("::name()", "QGIS umesh", Qgis::Info, true);
    return &qgis_umesh::s_name; // derde vanuit QGIS
}

QGISEXTERN const QString* category()
{
    //QgsMessageLog::logMessage("::category()", "QGIS umesh", Qgis::Info, true);
    return &qgis_umesh::s_category; //
}

QGISEXTERN const QString* description()
{
    //QgsMessageLog::logMessage("::description()", "QGIS umesh", Qgis::Info, true);
    return &qgis_umesh::s_description; // tweede vanuit QGIS
}

QGISEXTERN const QString* version()
{
    //QgsMessageLog::logMessage("::version()", "QGIS umesh", Qgis::Info, true);
    return &qgis_umesh::s_plugin_version;
}

QGISEXTERN const QString* icon()  // derde vanuit QGIS
{
    //QgsMessageLog::logMessage("::icon()", "QGIS umesh", Qgis::Info, true);
    //QString program_files = QProcessEnvironment::systemEnvironment().value("ProgramFiles", "");
    //QString q_icon_file = program_files + QString("/deltares/qgis_umesh/icons/qgis_umesh.png");
    QString q_icon_file = QStringLiteral("C:/Program Files/deltares/qgis_umesh/icons/qgis_umesh.png");
    return &q_icon_file;
}
// Delete ourself
QGISEXTERN void unload(QgisPlugin* the_qgis_umesh_pointer)
{
    //QgsMessageLog::logMessage("::unload()", "QGIS umesh", Qgis::Info, true);
    the_qgis_umesh_pointer->unload();
    delete the_qgis_umesh_pointer;
}
