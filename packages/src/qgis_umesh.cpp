#include "qgis_umesh.h"
#include "qgis_umesh_version.h"

#define GUI_EXPORT __declspec(dllimport)
#include "qgisinterface.h"
#include "qgsapplication.h"
#include "qgscoordinatereferencesystem.h"

#if defined(WIN64)
#  include <windows.h>
#  define strdup _strdup 
#endif
    
#define PROGRAM "QGIS_UMESH"
#define QGIS_UMESH_VERSION "0.00.01"
#define COMPANY "Deltares"
#define ARCH "Win64"

static const QString ident = QObject::tr( "@(#)" COMPANY ", " PROGRAM ", " QGIS_UMESH_VERSION ", " ARCH", " __DATE__", " __TIME__ );
static const QString sName = QObject::tr( "" COMPANY ", " PROGRAM " Development");
static const QString sDescription = QObject::tr("Plugin to read 1D and 2D unstructured meshes (" __DATE__", " __TIME__")");
static const QString sCategory = QObject::tr("Plugins");
static const QString sPluginVersion = QObject::tr(QGIS_UMESH_VERSION);
static const QgisPlugin::PluginType sPluginType = QgisPlugin::UI;
static const QString * sPluginIcon;

//
//-----------------------------------------------------------------------------
//
qgis_umesh::qgis_umesh(QgisInterface* iface):
    QgisPlugin(sName, sDescription, sCategory, sPluginVersion, sPluginType),
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

    this->pgBar = new QProgressBar();
    this->pgBar->setMaximum(1000);
    this->pgBar->setValue(0);
    this->pgBar->hide();

    status_bar = mQGisIface->statusBarIface();
    status_bar->addPermanentWidget(this->pgBar, 0, QgsStatusBar::Anchor::AnchorRight);

    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible}
    //connect(treeRoot, &QgsLayerTree::removedChildren, this, &qgis_umesh::onWillRemoveChildren);
    connect(treeRoot, SIGNAL(removedChildren(QgsLayerTreeNode *, int, int)), this, SLOT(onWillRemoveChildren(QgsLayerTreeNode *, int, int)));
    //root.willRemoveChildren.connect(onWillRemoveChildren)
    //    root.removedChildren.connect(onRemovedChildren)
}
void qgis_umesh::onWillRemoveChildren(QgsLayerTreeNode * node, int indexFrom, int indexTo)
{
    //QMessageBox::information(0, "qgis_umesh::onWillRemoveChildren()", QString("Node: %1.").arg(node->name()));
    // Remove the file entry belonging to this group
    // if available: deactivate the map_time_manager_window
}

//
//-----------------------------------------------------------------------------
//
qgis_umesh::~qgis_umesh()
{
    int i;
    i = 5;
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
                UGRID * ugrid_file = get_active_ugrid_file(layer[0]->layer()->id());
                QString epsg_code = new_crs.authid();
                QStringList parts = epsg_code.split(':');
                long epsg = parts.at(1).toInt();
                ugrid_file->set_grid_mapping_epsg(epsg, epsg_code.toUtf8().constData());
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
    QIcon * icon_file_open = new QIcon(QPixmap(vsi));
    std::cout << "HelloWorldPlugin::initGui" << std::endl;

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
    open_action_map = new QAction(icon_open, tr("&Open Map ..."));
    open_action_map->setToolTip(tr("Open UGRID 1D2D file"));
    open_action_map->setStatusTip(tr("Open UGRID file containing 1D, 2D and/or 1D2D meshes"));
    open_action_map->setEnabled(true);
    connect(open_action_map, SIGNAL(triggered()), this, SLOT(openFile()));

    icon_open_his_cf = get_icon_file(program_files_dir, "/icons/file_open.png");
    open_action_his_cf = new QAction(icon_open_his_cf, tr("&Open HIS ..."));
    open_action_his_cf->setToolTip(tr("Open CF compliant time series file"));
    open_action_his_cf->setStatusTip(tr("Open Climate and Forecast compliant time series file"));
    open_action_his_cf->setEnabled(true);
    connect(open_action_his_cf, SIGNAL(triggered()), this, SLOT(open_file_his_cf()));

    icon_edit_1d_obs_points = get_icon_file(program_files_dir, "/icons/edit_observation_points.png");
    edit_action_1d_obs_points = new QAction(icon_edit_1d_obs_points, tr("&1D: Observation points ..."));
    edit_action_1d_obs_points->setToolTip(tr("Add/Remove observation points"));
    edit_action_1d_obs_points->setStatusTip(tr("Add/Remove observation points on 1D geometry network"));
    edit_action_1d_obs_points->setEnabled(true);
    connect(edit_action_1d_obs_points, &QAction::triggered, this, &qgis_umesh::edit_1d_obs_points);

    icon_inspect = get_icon_file(program_files_dir, "/icons/remoteolv_icon.png");
    inspectAction = new QAction(icon_inspect, tr("&Show map output"));
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
    mapoutputAction->setText("Map settings ...");
    mapoutputAction->setToolTip(tr("Supply the settings for the map animation"));
    mapoutputAction->setStatusTip(tr("Supply the settings for the map animation"));
    mapoutputAction->setEnabled(true);
    connect(mapoutputAction, SIGNAL(triggered()), this, SLOT(mapPropertyWindow()));
    //connect(mapoutputAction, &QAction::triggered, MapTimeManagerWindow, &MapTimeManagerWindow::contextMenu);

    icon_open = get_icon_file(program_files_dir, "/icons/file_open.png");
    QAction * saveAction = new QAction(icon_open, tr("&Save"));
    saveAction->setToolTip(tr("Dummmy Item"));
    saveAction->setStatusTip(tr("Dummy item shown in statusbar"));
    saveAction->setEnabled(true);
    connect(saveAction, SIGNAL(triggered()), this, SLOT(openFile()));

    aboutAction = new QAction(tr("&About ..."), this);
    aboutAction->setToolTip(tr("Show the About box"));
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
    janm1->addAction(aboutAction);

    janm1 = janm->addMenu("Trials");
    janm1->addAction(open_action_mdu);
    janm1->addAction(edit_action_1d_obs_points);

    _menuToolBar->addWidget(janm);
    tbar->addWidget(_menuToolBar);
    tbar->addSeparator();
    //tbar->addAction(openAction);
    tbar->addAction(inspectAction);
    tbar->addAction(plotcftsAction);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tbar->addWidget(spacer);

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
        UGRID * ugrid_file = get_active_ugrid_file("");
        if (ugrid_file != nullptr)
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
void qgis_umesh::show_map_output(UGRID * ugrid_file)
{
    //QMessageBox::information(0, "Information", QString("qgis_umesh::show_map_output()\nTime manager: %1.").arg(MapTimeManagerWindow::get_count()));

    // Check on at least UGRID-1.0 file formaat
    long nr_times = 0;
    struct _global_attributes * globals;
    bool conventions_found = false;

    if (ugrid_file != nullptr)
    {
        globals = ugrid_file->get_global_attributes();
        nr_times = ugrid_file->get_count_times();
    }
    else
    {
        QMessageBox::warning(0, "Fatal error", QString("qgis_umesh::show_map_output().\nVariable ugrid_file is a nullptr."));
        return;
    }
    for (int i = 0; i < globals->count; i++)
    {
        if (globals->attribute[i]->name == "Conventions")
        {
            conventions_found = true;
            if (globals->attribute[i]->cvalue.find("UGRID-1.") == string::npos)
            {
                QMessageBox::information(0, "Information", QString("Time manager will not start because this file is not UGRID-1.* compliant.\nThis file has file format: %1.").arg(globals->attribute[i]->cvalue.c_str()));
                return;
            }
        }
    }
    if (!conventions_found)
    {
        QMessageBox::information(0, "Information", QString("Time manager will not start because there is no Conventions attribute in the file."));
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

            mtm_widget = new MapTimeManagerWindow(ugrid_file, mMyCanvas);
            mtm_widget->setContextMenuPolicy(Qt::CustomContextMenu);
            mQGisIface->addDockWidget(Qt::LeftDockWidgetArea, mtm_widget);
            QObject::connect(mtm_widget, &MapTimeManagerWindow::customContextMenuRequested, mtm_widget, &MapTimeManagerWindow::contextMenu);
        }
    }
    else
    {
        QString fname = ugrid_file->get_filename().canonicalFilePath();
        QMessageBox::information(0, tr("Message"), QString("No time-series available in file:\n%1.").arg(fname));
    }
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::edit_1d_obs_points()
{
    //QMessageBox::information(0, "Information", QString("qgis_umesh::edit_1d_obs_points()."));
    UGRID * ugrid_file = get_active_ugrid_file("");
    if (ugrid_file == nullptr)
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
            editObs_widget = new EditObsPoints(obs_layer, geom_layer, ugrid_file, mQGisIface);
            mQGisIface->addDockWidget(Qt::LeftDockWidgetArea, editObs_widget);
        }
    }
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::mapPropertyWindow()
{
    MapPropertyWindow * map_property = new MapPropertyWindow(mMyCanvas);
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
    msg_text->append("Plot UGRID compliant 1D mesh with its geometry, and 2D meshes\n");
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
    QString fname;
    QString * str = new QString();
    QFileDialog * fd = new QFileDialog();
    QStringList * list = new QStringList();

    str->clear();
    str->append("UGRID");
    str->append(" (*_map.nc *_net.nc)");
    list->append(*str);
    str->clear();
    str->append("UGRID");
    str->append(" (*_map.nc)");
    list->append(*str);
    str->clear();
    str->append("UGRID");
    str->append(" (*_net.nc)");
    list->append(*str);
    str->clear();
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
        this->pgBar->setMaximum(1000);
        this->pgBar->setValue(0);

        for (QStringList::Iterator it = QFilenames->begin(); it != QFilenames->end(); ++it) {
            fname = *it;
            openFile(fname);
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
void qgis_umesh::openFile(QFileInfo ncfile)
{
    int ncid;
    char * fname = strdup(ncfile.absoluteFilePath().toUtf8());
    int status = nc_open(fname, NC_NOWRITE, &ncid);
    (void)nc_close(ncid);
    if (status != NC_NOERR)
    {
        QMessageBox::warning(0, tr("Warning"), tr("Cannot open netCDF file:\n%1\nThis filename is not supported QGIS unstructured mesh program.").arg(ncfile.absoluteFilePath()));
        return;
    }
    _fil_index++;
    _UgridFiles.push_back(new UGRID(ncfile, this->pgBar));
    UGRID * ugrid_file = new UGRID(ncfile, this->pgBar);
    //QMessageBox::warning(0, tr("Warning"), tr("netCDF file opened:\n%1.").arg(ncfile.absoluteFilePath()));
    ugrid_file->read();
    _UgridFiles[_fil_index] = ugrid_file;
    activate_layers();
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
            open_file_his_cf(fname);
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
    char * fname = strdup(ncfile.absoluteFilePath().toUtf8());
    int status = nc_open(fname, NC_NOWRITE, &ncid);
    (void)nc_close(ncid);
    if (status != NC_NOERR)
    {
        QMessageBox::warning(0, tr("Warning"), tr("Cannot open netCDF file:\n%1\nThis filename is not supported QGIS unstructured mesh program.").arg(ncfile.absoluteFilePath()));
        return;
    }
    _his_cf_fil_index++;
    _his_cf_files.push_back(new HISCF(ncfile, this->pgBar));
    HISCF * _his_cf_file = new HISCF(ncfile, this->pgBar);
    //QMessageBox::warning(0, tr("Warning"), tr("netCDF file opened:\n%1.").arg(ncfile.absoluteFilePath()));
    _his_cf_file->read();
    _his_cf_files[_his_cf_fil_index] = _his_cf_file;
    activate_observation_layers();
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::open_file_mdu()
{
    QString fname;
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
            fname = *it;
            open_file_mdu(fname);
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
    string fname = jsonfile.absoluteFilePath().toStdString();
    READ_JSON * pt_mdu = new READ_JSON(fname);
    if (pt_mdu == nullptr)
    {
        QMessageBox::warning(0, tr("Warning"), tr("Cannot open JSON file:\n%1.").arg(jsonfile.absoluteFilePath()));
        return;
    }
    _mdu_files.push_back(pt_mdu);
    //MDU * _mdu_file = new MDU(jsonfile, this->pgBar);
    string values = "data.geometry.NetFile";
    vector<string> ncfile;
    long status = pt_mdu->get(values, ncfile);
    QString mesh;
    if (ncfile.size() == 1)
    {
        mesh = jsonfile.absolutePath() + "/" + QString::fromStdString(ncfile[0]);
        //QMessageBox::warning(0, tr("Warning"), tr("JSON file opened: %1\nMesh file opened: %2.").arg(jsonfile.absoluteFilePath()).arg(mesh));
        openFile(mesh);
    }
    else
    {
        QMessageBox::warning(0, tr("Warning"), tr("JSON file opened: %1\nNo keyword \"%2\" in this file.").arg(jsonfile.absoluteFilePath()).arg(QString::fromStdString(values)));
        return;
    }
    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
    QgsLayerTreeGroup * myGroup = treeRoot->findGroup(QString("UGRID Mesh - %1").arg(_fil_index + 1));

    values = "data.geometry.StructureFile";
    vector<string> struct_file;
    status = pt_mdu->get(values, struct_file);
    if (struct_file.size() == 1)
    {
        QString struc_file = jsonfile.absolutePath() + "/" + QString::fromStdString(struct_file[0]);
        if (!QFileInfo(struc_file).exists())
        {
            QMessageBox::information(0, tr("Structure file"), tr("File:\n\"%1\",\nReferenced by tag: \"%2\" does not exist. Structures are skipped.").arg(struc_file).arg(QString::fromStdString(values)));
        }
        else
        {
            fname = struc_file.toStdString();
            READ_JSON * pt_structures = new READ_JSON(fname);
            UGRID * ugrid_file = _UgridFiles[_fil_index];
            if (ugrid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
            {
                QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(ugrid_file->get_filename().fileName()));
                return;
            }
            struct _mapping * mapping;
            mapping = ugrid_file->get_grid_mapping();
            create_1D_structure_vector_layer(ugrid_file, pt_structures, mapping->epsg, myGroup);  // i.e. a JSON file
        }
    }
    else
    {
        QMessageBox::warning(0, tr("Warning"), QString(tr("JSON file opened: %1\nNo keyword \"%2\" in this file.").arg(jsonfile.absoluteFilePath()).arg(QString::fromStdString(values))));
    }

    values = "data.output.ObsFile";
    vector<string> obs_file;
    status = pt_mdu->get(values, obs_file);
    if (obs_file.size() == 1)
    {
        QString obser_file = jsonfile.absolutePath() + "/" + QString::fromStdString(obs_file[0]);
        if (!QFileInfo(obser_file).exists())
        {
            QMessageBox::information(0, tr("Observation point file"), tr("File:\n\"%1\",\nReferenced by tag: \"%2\" does not exist. Observation points are skipped.").arg(obser_file).arg(QString::fromStdString(values)));
        }
        else
        {
            fname = obser_file.toStdString();
            READ_JSON * pt_obs = new READ_JSON(fname);
            UGRID * ugrid_file = _UgridFiles[_fil_index];
            if (ugrid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
            {
                QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(ugrid_file->get_filename().fileName()));
                return;
            }
            struct _mapping * mapping;
            mapping = ugrid_file->get_grid_mapping();
            create_1D_observation_point_vector_layer(ugrid_file, pt_obs, mapping->epsg, myGroup);  // i.e. a JSON file
        }
    }
    else
    {
        QMessageBox::warning(0, tr("Warning"), QString(tr("JSON file opened: %1\nNo keyword \"%2\" in this file.").arg(jsonfile.absoluteFilePath()).arg(QString::fromStdString(values))));
    }

    values = "data.external_forcing.ExtForceFileNew";
    vector<string> ext_file_name;  // There is just one name, so size should be 1
    status = pt_mdu->get(values, ext_file_name);

    if (ext_file_name.size() == 1)
    {
        QString ext_file = jsonfile.absolutePath() + "/" + QString::fromStdString(ext_file_name[0]);
        if (!QFileInfo(ext_file).exists())
        {
            QMessageBox::information(0, tr("External forcings file"), tr("File:\n\"%1\",\nReferenced by tag: \"%2\" does not exist. External forcings are skipped.").arg(ext_file).arg(QString::fromStdString(values)));
        }
        else
        {
            fname = ext_file.toStdString();
            READ_JSON * pt_ext_file = new READ_JSON(fname);
            UGRID * ugrid_file = _UgridFiles[_fil_index];
            if (ugrid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
            {
                QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(ugrid_file->get_filename().fileName()));
                return;
            }
            struct _mapping * mapping;
            mapping = ugrid_file->get_grid_mapping();
            create_1D_external_forcing_vector_layer(ugrid_file, pt_ext_file, mapping->epsg, myGroup);  // i.e. a JSON file
        }
    }
    else
    {
        QMessageBox::warning(0, tr("Warning"), QString(tr("JSON file opened: %1\nNo keyword \"%2\" in this file.").arg(jsonfile.absoluteFilePath()).arg(QString::fromStdString(values))));
    }
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::set_show_map_output()
{
    UGRID * ugrid_file = get_active_ugrid_file("");
    if (ugrid_file != nullptr) show_map_output(ugrid_file);
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
UGRID * qgis_umesh::get_active_ugrid_file(QString layer_id)
{
    // Function to find the filename belonging by the highlighted layer, 
    // or to which group belongs the highligthed layer,
    // or the highlighted group
    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
    QList <QgsLayerTreeGroup *> groups = treeRoot->findGroups();
    QgsMapLayer * active_layer = NULL;
    UGRID * ugrid_file = nullptr;

    if (layer_id == "")
    {
        active_layer = mQGisIface->activeLayer();
        if (active_layer != nullptr)
        {
            layer_id = active_layer->id();
        }
        // no else because the group entry does not have a CRS.
    }
    if (layer_id != "")
    {
        //QMessageBox::information(0, "Information", QString("qgis_umesh::get_active_layer()\nActive layer: %1.").arg(active_layer->name()));
        // if there is an active layer, belongs it to a Mesh-group?
        for (int j = 0; j < _fil_index + 1; j++)
        {
            QgsLayerTreeGroup * myGroup = treeRoot->findGroup(QString("UGRID Mesh - %1").arg(j + 1));
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
                        ugrid_file = _UgridFiles[j];
                    }
                }
            }
        }
    }
    //else
    //{
    //    QMessageBox::information(0, "Information", QString("No layer selected, determination of output files is not possible."));
    //}

    return ugrid_file;
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
                QgsLayerTreeGroup * myGroup = treeRoot->findGroup(QString("History - %1").arg(j + 1));
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
            QgsLayerTreeGroup * myGroup = treeRoot->findGroup(QString("History - %1").arg(j + 1));
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
                        _his_cf_file = _his_cf_files[j];
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
    struct _mesh1d * mesh1d;
    struct _mesh2d * mesh2d;
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

        QgsLayerTreeGroup * treeGroup = treeRoot->findGroup(QString("UGRID Mesh - %1").arg(_fil_index + 1));
        if (treeGroup == nullptr)
        {
            QString name = QString("UGRID Mesh - %1").arg(_fil_index + 1);
            treeGroup = treeRoot->insertGroup(_fil_index, name);
            treeGroup->setExpanded(true);  // true is the default 
            treeGroup->setItemVisibilityChecked(true);
            //QMessageBox::warning(0, "Message", QString("Create group: %1.").arg(name));
            treeGroup->setItemVisibilityCheckedRecursive(true);
        }
        //treeGroup->setItemVisibilityCheckedParentRecursive(true);
        //connect(lstCoordinateSystems, &QTreeWidget::currentItemChanged, this, &qgis_umesh::msg);  // changing coordinate system of a layer
        // signal when changing CRS: QgsProject::instance()->crsChanged();
        //connect(mCanvas, &QgsMapCanvas::destinationCrsChanged, this, &qgis_umesh::msg);  // changing coordinate system of a layer
        //for (int i = 0; i < mQGisIface->mapCanvas()->layerCount(); i++)
        //{
        //    QgsMapLayer * mapLayer = mQGisIface->mapCanvas()->layer(i);
        //}
        //QgsProject::instance()->crs
        // The treeGroup exists
        if (_fil_index != -1)
        {
            UGRID * ugrid_file = _UgridFiles[_fil_index];
            struct _mapping * mapping;
            mapping = ugrid_file->get_grid_mapping();
            if (mapping->epsg == 0)
            {
                QString fname = ugrid_file->get_filename().canonicalFilePath();
                QMessageBox::information(0, "Message", QString("The CRS code on file\n\'%1\'\nis not known.\nThe CRS code is set to the same CRS code as the presentation map (see lower right corner).\nPlease change the CRS code after loading the file, if necessary.").arg(fname));
                // get the crs of the presentation (lower right corner of the qgis main-window)
                QgsCoordinateReferenceSystem _crs = QgsProject::instance()->crs();
                QString epsg_code = _crs.authid();
                QStringList parts = epsg_code.split(':');
                long epsg = parts.at(1).toInt();
                ugrid_file->set_grid_mapping_epsg(epsg, epsg_code.toUtf8().constData());
            }
            int pgbar_value = 500;  // start of pgbar counter

                                    // Mesh 1D edges and mesh 1D nodes

            mesh1d = ugrid_file->get_mesh1d();
            //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D nodes."));
            if (mesh1d != nullptr)
            {
                create_nodes_vector_layer(QString("Mesh1D nodes"), mesh1d->node[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D edges."));
                create_edges_vector_layer(QString("Mesh1D edges"), mesh1d->node[0], mesh1d->edge[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
            }

            // Mesh1D Topology Edges between connection nodes, Network Connection node, Network geometry
            ntw_nodes = ugrid_file->get_connection_nodes();
            ntw_edges = ugrid_file->get_network_edges();
            ntw_geom = ugrid_file->get_network_geometry();

            if (ntw_nodes != nullptr)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D Connection nodes."));
                QString layer_name = QString("Mesh1D Connection nodes");
                create_nodes_vector_layer(layer_name, ntw_nodes->node[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
 
                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D geometry."));
                create_geometry_vector_layer(QString("Mesh1D geometry"), ntw_geom, mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);

                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D Topology edges."));
                create_edges_vector_layer(QString("Mesh1D Topology edges"), ntw_nodes->node[0], ntw_edges->edge[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
            }

            // Mesh1D Topology Edges between connection nodes, Network Connection node, Network geometry

            mesh_contact = ugrid_file->get_mesh_contact();

            if (mesh_contact != nullptr)
            {
                create_edges_vector_layer(QString("Mesh contact edges"), mesh_contact->node[0], mesh_contact->edge[0], mapping->epsg, treeGroup);
            }

            // Mesh 2D edges and Mesh 2D nodes

            mesh2d = ugrid_file->get_mesh2d();
            if (mesh2d != nullptr)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Mesh2D nodes."));
                if (mesh2d->face[0]->count > 0)
                {
                    create_nodes_vector_layer(QString("Mesh2D faces"), mesh2d->face[0], mapping->epsg, treeGroup);
                    pgbar_value += 10;
                    this->pgBar->setValue(pgbar_value);
                }

                //QMessageBox::warning(0, tr("Warning"), tr("Mesh2D nodes."));
                create_nodes_vector_layer(QString("Mesh2D nodes"), mesh2d->node[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);

                //QMessageBox::warning(0, tr("Warning"), tr("Mesh2D edges."));
                if (mesh2d->edge[0]->count > 0)
                {
                    create_edges_vector_layer(QString("Mesh2D edges"), mesh2d->node[0], mesh2d->edge[0], mapping->epsg, treeGroup);
                    pgbar_value += 10;
                    this->pgBar->setValue(pgbar_value);
                }
            }
            pgbar_value = 600;  // start of pgbar counter

            this->pgBar->setValue(pgbar_value);

            //
            // get the time independent variables and list them in the layer-panel as treegroup
            //
            struct _mesh_variable * var = ugrid_file->get_variables();

            bool time_independent_data = false;
            if (mesh2d != nullptr)
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
                    treeGroup->addGroup(name);
                    QgsLayerTreeGroup * subTreeGroup = treeGroup->findGroup(name);

                    subTreeGroup->setExpanded(true);  // true is the default 
                    subTreeGroup->setItemVisibilityChecked(true);
                    //QMessageBox::warning(0, "Message", QString("Create group: %1.").arg(name));
                    subTreeGroup->setItemVisibilityCheckedRecursive(true);

                    //cb->blockSignals(true);
                    for (int i = 0; i < var->nr_vars; i++)
                    {
                        if (!var->variable[i]->time_series && var->variable[i]->coordinates != "")
                        {
                            QString name = QString::fromStdString(var->variable[i]->long_name).trimmed();
                            QString var_name = QString::fromStdString(var->variable[i]->var_name).trimmed();
                            if (var->variable[i]->location == "edge" && var->variable[i]->topology_dimension == 2 && var->variable[i]->nc_type == NC_DOUBLE)
                            {
                                vector<vector <double *>>  std_data_at_edge = ugrid_file->get_variable_values(var->variable[i]->var_name);
                                create_data_on_edges_vector_layer(var->variable[i], mesh2d->node[0], mesh2d->edge[0], std_data_at_edge[0], mapping->epsg, subTreeGroup);
                            }
                            if (var->variable[i]->location == "face" && var->variable[i]->topology_dimension == 2 && var->variable[i]->nc_type == NC_DOUBLE)
                            {
                                vector<vector <double *>>  std_data_at_face = ugrid_file->get_variable_values(var->variable[i]->var_name);
                                create_data_on_nodes_vector_layer(var->variable[i], mesh2d->face[0], std_data_at_face[0], mapping->epsg, subTreeGroup);
                            }
                            if (var->variable[i]->location == "node" && var->variable[i]->topology_dimension == 2 && var->variable[i]->nc_type == NC_DOUBLE)
                            {
                                vector<vector <double *>>  std_data_at_node = ugrid_file->get_variable_values(var->variable[i]->var_name);
                                create_data_on_nodes_vector_layer(var->variable[i], mesh2d->node[0], std_data_at_node[0], mapping->epsg, subTreeGroup);
                            }
                        }
                    }
                    //cb->blockSignals(false);
                }
            }  // end mesh2d != nullptr
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
    vector<_location_type *> obs_type;

    QgsLayerTree * treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible

    if (mainAction->isChecked())
    {
        QList <QgsLayerTreeGroup *> groups = treeRoot->findGroups();
        for (int i = 0; i< groups.length(); i++)
        {
            //QMessageBox::warning(0, "Message", QString("_fil_index: %1+1.").arg(_fil_index+1));
            for (int j = 0; j < _his_cf_fil_index + 1; j++)
            {
                QgsLayerTreeGroup * myGroup = treeRoot->findGroup(QString("History - %1").arg(j + 1));
                if (myGroup != nullptr)
                {
                    myGroup->setItemVisibilityChecked(true);
                }
            }
        }

        QgsLayerTreeGroup * treeGroup = treeRoot->findGroup(QString("History - %1").arg(_his_cf_fil_index + 1));
        if (treeGroup == nullptr)
        {
            QString name = QString("History - %1").arg(_his_cf_fil_index + 1);
            treeGroup = treeRoot->insertGroup(_his_cf_fil_index, name);
            treeGroup->setExpanded(true);  // true is the default 
            treeGroup->setItemVisibilityChecked(true);
            //QMessageBox::warning(0, "Message", QString("Create group: %1.").arg(name));
            treeGroup->setItemVisibilityCheckedRecursive(true);
        }
        if (_his_cf_fil_index != -1)
        {
            HISCF * _his_cf_file = _his_cf_files[_his_cf_fil_index];
            struct _mapping * mapping;
            mapping = _his_cf_file->get_grid_mapping();
            if (mapping->epsg == 0)
            {
                QString fname = _his_cf_file->get_filename().canonicalFilePath();
                QMessageBox::information(0, "Message", QString("The CRS code on file\n\'%1\'\nis not known.\nThe CRS code is set to the same CRS code as the presentation map (see lower right corner).\nPlease change the CRS code after loading the file, if necessary.").arg(fname));
                // get the crs of the presentation (lower right corner of the qgis main-window)
                QgsCoordinateReferenceSystem _crs = QgsProject::instance()->crs();
                QString epsg_code = _crs.authid();
                QStringList parts = epsg_code.split(':');
                long epsg = parts.at(1).toInt();
                _his_cf_file->set_grid_mapping_epsg(epsg, epsg_code.toUtf8().constData());
            }
            int pgbar_value = 500;  // start of pgbar counter

                                    // Mesh 1D edges and mesh 1D nodes

            obs_type = _his_cf_file->get_observation_location();
            //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D nodes."));
            if (obs_type.size() > 0)
            {
                for (int i = 0; i < obs_type.size(); i++)
                {
                    if (obs_type[i]->type == OBS_POINT)
                    {
                        create_observation_point_vector_layer(QString("Observation point"), obs_type[i], mapping->epsg, treeGroup);
                    }
                    if (obs_type[i]->type == OBS_POLYLINE)
                    {
                        create_observation_polyline_vector_layer(QString("Cross section"), obs_type[i], mapping->epsg, treeGroup);
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
        if (str.contains("UGRID Mesh - ") ||
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
        //delete[] _UgridFiles[i];
    }
    if (_UgridFiles.size() != 0)
    {
        //delete _UgridFiles;
    }

    if (mtm_widget != NULL)
    {
        delete mtm_widget;
        mtm_widget = NULL; 

    }
    //QMessageBox::warning(0, tr("Message"), QString("qgis_umesh::unload()."));
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::unload_vector_layers()
{
    // remove scratch layer
    QList< QgsMapLayer * > layers = mQGisIface->mapCanvas()->layers();
    int cnt = layers.count();
    //QMessageBox::warning(0, tr("Message"), QString("qgis_umesh::unload_vector_layers()\nActive layers: %1.").arg(cnt));

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

void qgis_umesh::create_nodes_vector_layer(QString layer_name, struct _feature * nodes, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (nodes != NULL)
    {
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        QList <QgsField> lMyAttribField;

        bool layer_found = false;
        for (int i = 0; i < tmp_layers.length(); i++)
        {
            //QMessageBox::warning(0, tr("Message: create_nodes_vector_layer"), QString(tr("Layers in group by name: ")) + tmp_layers[i]->name() + QString(" Look for: ") + layer_name);
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

            int nr_attrib_fields = 2;
            lMyAttribField << QgsField("Node Id (0-based)", QVariant::String)
                           << QgsField("Node Id (1-based)", QVariant::String);
            if (nodes->name.size() != 0)
            {
                lMyAttribField << QgsField("Node name", QVariant::String);
                nr_attrib_fields++;
            }
            if (nodes->long_name.size() != 0)
            {
                lMyAttribField  << QgsField("Node long name", QVariant::String);
                nr_attrib_fields++;
            }

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            int nsig = long( log10(nodes->count) ) + 1;
            for (int j = 0; j < nodes->count; j++)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Count: %1, %5\nCoord: (%2,%3)\nName : %4\nLong name: %5").arg(ntw_nodes->nodes[i]->count).arg(ntw_nodes->nodes[i]->x[j]).arg(ntw_nodes->nodes[i]->y[j]).arg(ntw_nodes->nodes[i]->id[j]).arg(ntw_nodes->nodes[i]->name[j]));

                QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(nodes->x[j], nodes->y[j]));
                QgsFeature MyFeature;
                MyFeature.setGeometry(MyPoints);

                MyFeature.initAttributes(nr_attrib_fields);
                int k = 0;
                MyFeature.setAttribute(0, QString("%1_b0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                k++;
                MyFeature.setAttribute(1, QString("%1_b1").arg(j + 1));
                if (nodes->name.size() != 0)
                {
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(nodes->name[j]).trimmed()));
                }
                if (nodes->long_name.size() != 0)
                {
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(nodes->long_name[j]).trimmed()));
                    //MyFeature.setAttribute(k, QString::fromStdString(nodes->long_name[j]).trimmed());
                }

                dp_vl->addFeature(MyFeature);
            }
            vl->commitChanges();
            //QgsCoordinateReferenceSystem crs = vl->crs();
            //QMessageBox::information(0, tr("Message: create_geometry_vector_layer"), QString("CRS layer: %1.").arg(crs.authid()));

            // todo: Probeersel symbology adjustements

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

                QgsSymbol * marker = QgsSymbol::defaultSymbol(QgsWkbTypes::PointGeometry);
                marker->changeSymbolLayer(0, simple_marker);

                //set up a renderer for the layer
                QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
                vl->setRenderer(mypRenderer);
            }
            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
    }
}
void qgis_umesh::create_data_on_edges_vector_layer(_variable * var, struct _feature * nodes, struct _edge * edges, vector<double *> z_value, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (nodes != nullptr)
    {
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        QList <QgsField> lMyAttribField;
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

            QString uri = QString("MultiPoint?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsMultiLineString * polylines = new QgsMultiLineString();
            QVector<QgsPointXY> point;
            QgsMultiPolylineXY lines;

            for (int j = 0; j < edges->count; j++)
            {
                lines.clear();
                point.clear();
                int p1 = edges->edge_nodes[j][0];
                int p2 = edges->edge_nodes[j][1];
                double x1 = nodes->x[p1];
                double y1 = nodes->y[p1];
                double x2 = nodes->x[p2];
                double y2 = nodes->y[p2];
                point.append(QgsPointXY(x1, y1));
                point.append(QgsPointXY(x2, y2));
                //QMessageBox::warning(0, tr("Warning"), tr("Edge: %1 (%2, %3)->(%4, %5).").arg(j).arg(x1).arg(y1).arg(x2).arg(y2));
                lines.append(point);

                QgsGeometry MyEdge = QgsGeometry::fromMultiPolylineXY(lines);
                QgsFeature MyFeature;
                MyFeature.setGeometry(MyEdge);

                MyFeature.initAttributes(nr_attrib_fields);
                MyFeature.setAttribute(0, *z_value[j]);
                MyFeature.setValid(true);

                dp_vl->addFeature(MyFeature);
                vl->commitChanges();
            }
            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer

            QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
            int ind = tmp_layers.size() - 1;
            tmp_layers[ind]->setItemVisibilityChecked(false);

        }
    }
}
void qgis_umesh::create_data_on_nodes_vector_layer(_variable * var, struct _feature * nodes, vector<double *> z_value, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (nodes != nullptr)
    {
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        QList <QgsField> lMyAttribField;
        QString layer_name = QString::fromStdString(var->long_name).trimmed();
        bool layer_found = false;
        for (int i = 0; i < tmp_layers.length(); i++)
        {
            //QMessageBox::warning(0, tr("Message: create_nodes_vector_layer"), QString(tr("Layers in group by name: ")) + tmp_layers[i]->name() + QString(" Look for: ") + layer_name);
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
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            for (int j = 0; j < nodes->count; j++)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Count: %1, %5\nCoord: (%2,%3)\nName : %4\nLong name: %5").arg(ntw_nodes->nodes[i]->count).arg(ntw_nodes->nodes[i]->x[j]).arg(ntw_nodes->nodes[i]->y[j]).arg(ntw_nodes->nodes[i]->id[j]).arg(ntw_nodes->nodes[i]->name[j]));

                QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(nodes->x[j], nodes->y[j]));
                QgsFeature MyFeature;
                MyFeature.setGeometry(MyPoints);

                MyFeature.initAttributes(nr_attrib_fields);
                MyFeature.setAttribute(0, *z_value[j]);

                dp_vl->addFeature(MyFeature);
            }
            vl->commitChanges();

            QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
            simple_marker->setStrokeStyle(Qt::NoPen) ;

            QgsSymbol * marker = QgsSymbol::defaultSymbol(QgsWkbTypes::PointGeometry);
            marker->changeSymbolLayer(0, simple_marker);
            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);

            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer

            QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
            int ind = tmp_layers.size()-1;
            tmp_layers[ind]->setItemVisibilityChecked(false);


        }
    }
}
void qgis_umesh::create_geometry_vector_layer(QString layer_name, struct _ntw_geom * ntw_geom, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (ntw_geom != NULL)
    {
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();

        bool layer_found = false;
        for (int i = 0; i < tmp_layers.length(); i++)
        {
            //QMessageBox::warning(0, tr("Message: create_geometry_vector_layer"), QString(tr("Layers in group by name: ")) + tmp_layers[i]->name() + QString(" Look for: ") + layer_name);
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

            int nr_attrib_fields = 2;

            lMyAttribField << QgsField("Geometry Id (0-based)", QVariant::String)
                           << QgsField("Geometry Id (1-based)", QVariant::String);
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

            QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsMultiLineString * polylines = new QgsMultiLineString();
            QVector<QgsPointXY> point;
            QgsMultiPolylineXY lines;

            for (int i = 0; i < ntw_geom->nr_ntw; i++)
            {
                for (int j = 0; j < ntw_geom->geom[i]->count; j++)
                {
                    int nsig = long(log10(ntw_geom->geom[i]->nodes[j]->count)) + 1;
                    lines.clear();
                    point.clear();
                    for (int k = 0; k < ntw_geom->geom[i]->nodes[j]->count; k++)
                    {
                        double x1 = ntw_geom->geom[i]->nodes[j]->x[k];
                        double y1 = ntw_geom->geom[i]->nodes[j]->y[k];
                        point.append(QgsPointXY(x1, y1));
                        //QMessageBox::warning(0, tr("Warning"), tr("Edge: %1 (%2, %3)->(%4, %5).").arg(j).arg(x1).arg(y1).arg(x2).arg(y2));
                    }
                    lines.append(point);
                    QgsGeometry MyEdge = QgsGeometry::fromMultiPolylineXY(lines);
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyEdge);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = 0;
                    MyFeature.setAttribute(0, QString("%1_b0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(1, QString("%1_b1").arg(j + 1));
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

                    dp_vl->addFeature(MyFeature);
                    vl->commitChanges();
                }
            }
            if (layer_name == QString("Mesh1D geometry"))
            {
                QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
                line_marker->setWidth(0.5);
                line_marker->setColor(QColor(0, 0, 255));

                QgsSymbol * symbol = QgsSymbol::defaultSymbol(QgsWkbTypes::GeometryType::LineGeometry);
                symbol->changeSymbolLayer(0, line_marker);

                //set up a renderer for the layer
                QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
                vl->setRenderer(mypRenderer);
            }
            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
    }
}

void qgis_umesh::create_edges_vector_layer(QString layer_name, struct _feature * nodes, struct _edge * edges, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (nodes != NULL && edges!= NULL)
    {
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();

        bool layer_found = false;
        for (int i = 0; i < tmp_layers.length(); i++)
        {
            //QMessageBox::warning(0, tr("Message: create_edges_vector_layer"), QString(tr("Layers in group by name: ")) + tmp_layers[i]->name() + QString(" Look for: ") + layer_name);
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

            int nr_attrib_fields = 2;

            lMyAttribField << QgsField("Edge Id (0-based)", QVariant::String)
                           << QgsField("Edge Id (1-based)", QVariant::String);
            if (edges->name.size() != 0)
            {
                lMyAttribField << QgsField("Edge name", QVariant::String);
                nr_attrib_fields++;
            }
            if (edges->long_name.size() != 0)
            {
                lMyAttribField << QgsField("Edge long name", QVariant::String);
                nr_attrib_fields++;
            }

            QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsMultiLineString * polylines = new QgsMultiLineString();
            QVector<QgsPointXY> point;
            QgsMultiPolylineXY lines;

            int nsig = long(log10(edges->count)) + 1;
            for (int j = 0; j < edges->count; j++)
            {
                lines.clear();
                point.clear();
                int p1 = edges->edge_nodes[j][0];
                int p2 = edges->edge_nodes[j][1];
                double x1 = nodes->x[p1];
                double y1 = nodes->y[p1];
                double x2 = nodes->x[p2];
                double y2 = nodes->y[p2];
                point.append(QgsPointXY(x1, y1));
                point.append(QgsPointXY(x2, y2));
                //QMessageBox::warning(0, tr("Warning"), tr("Edge: %1 (%2, %3)->(%4, %5).").arg(j).arg(x1).arg(y1).arg(x2).arg(y2));
                lines.append(point);

                QgsGeometry MyEdge = QgsGeometry::fromMultiPolylineXY(lines);
                QgsFeature MyFeature;
                MyFeature.setGeometry(MyEdge);

                MyFeature.initAttributes(nr_attrib_fields);
                int k = 0;
                MyFeature.setAttribute(0, QString("%1_b0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                k++;
                MyFeature.setAttribute(1, QString("%1_b1").arg(j + 1));
                if (edges->name.size() != 0)
                {
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(edges->name[j]).trimmed()));
                }
                if (edges->long_name.size() != 0)
                {
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(edges->long_name[j]).trimmed()));
                }
                MyFeature.setValid(true);

                dp_vl->addFeature(MyFeature);
                vl->commitChanges();
            }
            if (layer_name == QString("Mesh2D edges"))
            {
                QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
                line_marker->setWidth(0.25);
                line_marker->setColor(QColor(0, 192, 255));

                QgsSymbol * symbol = QgsSymbol::defaultSymbol(QgsWkbTypes::GeometryType::LineGeometry);
                symbol->changeSymbolLayer(0, line_marker);

                //set up a renderer for the layer
                QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
                vl->setRenderer(mypRenderer);
            }
            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
    }
}
void qgis_umesh::create_observation_point_vector_layer(QString layer_name, _location_type * obs_points, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (obs_points != NULL && obs_points->type == OBS_POINT  || obs_points->type == OBS_POLYLINE)
    {
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        QList <QgsField> lMyAttribField;

        bool layer_found = false;
        for (int i = 0; i < tmp_layers.length(); i++)
        {
            //QMessageBox::warning(0, tr("Message: create_nodes_vector_layer"), QString(tr("Layers in group by name: ")) + tmp_layers[i]->name() + QString(" Look for: ") + layer_name);
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

            int nr_attrib_fields = 2;
            lMyAttribField << QgsField("Observation point Id (0-based)", QVariant::String)
                           << QgsField("Observation point Id (1-based)", QVariant::String);

            if (obs_points->location_long_name != nullptr)
            {
                lMyAttribField << QgsField(obs_points->location_long_name, QVariant::String);
                nr_attrib_fields++;
            }

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            int nsig = long(log10(obs_points->location.size())) + 1;
            for (int j = 0; j < obs_points->location.size(); j++)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Count: %1, %5\nCoord: (%2,%3)\nName : %4\nLong name: %5").arg(ntw_nodes->nodes[i]->count).arg(ntw_nodes->nodes[i]->x[j]).arg(ntw_nodes->nodes[i]->y[j]).arg(ntw_nodes->nodes[i]->id[j]).arg(ntw_nodes->nodes[i]->name[j]));

                QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(obs_points->location[j].x[0], obs_points->location[j].y[0]));
                QgsFeature MyFeature;
                MyFeature.setGeometry(MyPoints);

                MyFeature.initAttributes(nr_attrib_fields);
                int k = 0;
                MyFeature.setAttribute(0, QString("%1_b0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                k++;
                MyFeature.setAttribute(1, QString("%1_b1").arg(j + 1));
                if (obs_points->location[j].name != nullptr)
                {
                    k++;
                    MyFeature.setAttribute(k, QString("%1").arg(QString(obs_points->location[j].name).trimmed()));
                }

                dp_vl->addFeature(MyFeature);
            }
            vl->commitChanges();

            QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
            simple_marker->setSize(4.0);
            simple_marker->setColor(QColor(0, 0, 0));
            simple_marker->setFillColor(QColor(255, 255, 255));
            simple_marker->setShape(QgsSimpleMarkerSymbolLayerBase::Star);

            QgsSymbol * marker = QgsSymbol::defaultSymbol(QgsWkbTypes::PointGeometry);
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);

            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
                                                                          //QgsCoordinateReferenceSystem crs = vl->crs();
                                                                          //QMessageBox::information(0, tr("Message: create_geometry_vector_layer"), QString("CRS layer: %1").arg(crs.authid()));

                                                                          // todo: Probeersel symbology adjustements
        }
    }
}
void qgis_umesh::create_observation_polyline_vector_layer(QString layer_name, _location_type * obs_points, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (obs_points != NULL && obs_points->type == OBS_POLYLINE)
    {
        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        QList <QgsField> lMyAttribField;

        bool layer_found = false;
        for (int i = 0; i < tmp_layers.length(); i++)
        {
            //QMessageBox::warning(0, tr("Message: create_nodes_vector_layer"), QString(tr("Layers in group by name: ")) + tmp_layers[i]->name() + QString(" Look for: ") + layer_name);
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

            int nr_attrib_fields = 2;

            lMyAttribField << QgsField("Cross section Id (0-based)", QVariant::String)
                           << QgsField("Cross section Id (1-based)", QVariant::String);
                    
            if (obs_points->location_long_name != nullptr)
            {
                lMyAttribField << QgsField(obs_points->location_long_name, QVariant::String);
                nr_attrib_fields++;
            }

            QString uri = QString("MultiLineString?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");

            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QgsMultiLineString * polylines = new QgsMultiLineString();
            QVector<QgsPointXY> point;
            QgsMultiPolylineXY lines;

            for (int i = 0; i < obs_points->location.size(); i++)
            {
                //for (int j = 0; j < obs_points->location[i]; j++)
                {
                    lines.clear();
                    point.clear();
                    for (int k = 0; k < obs_points->location[i].x.size(); k++)
                    {
                        if (obs_points->location[i].x[k] != NC_FILL_DOUBLE)
                        {
                            double x1 = obs_points->location[i].x[k];
                            double y1 = obs_points->location[i].y[k];
                            point.append(QgsPointXY(x1, y1));
                        }
                    }

                    lines.append(point);
                    QgsGeometry MyEdge = QgsGeometry::fromMultiPolylineXY(lines);
                    QgsFeature MyFeature;
                    MyFeature.setGeometry(MyEdge);

                    MyFeature.initAttributes(nr_attrib_fields);
                    int k = 0;
                    MyFeature.setAttribute(0, QString("%1_b0").arg(i));  // arg(j, nsig, 10, QLatin1Char('0')));
                    k++;
                    MyFeature.setAttribute(1, QString("%1_b1").arg(i + 1));
                    if (obs_points->location[i].name != nullptr)
                    {
                        k++;
                        MyFeature.setAttribute(k, QString("%1").arg(QString(obs_points->location[i].name).trimmed()));
                    }

                    dp_vl->addFeature(MyFeature);
                    vl->commitChanges();
                }
            }


            QgsSimpleLineSymbolLayer * line_marker = new QgsSimpleLineSymbolLayer();
            line_marker->setWidth(0.75);
            line_marker->setColor(QColor(0, 0, 255));

            QgsSymbol * symbol = QgsSymbol::defaultSymbol(QgsWkbTypes::GeometryType::LineGeometry);
            symbol->changeSymbolLayer(0, line_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(symbol);
            vl->setRenderer(mypRenderer);

            add_layer_to_group(vl, treeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
    }
}
void qgis_umesh::create_1D_structure_vector_layer(UGRID * ugrid_file, READ_JSON * pt_structures, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (pt_structures != nullptr)
    {
        long status = -1;
        string values = "data.Structure.id";
        vector<string> id;
        status = pt_structures->get(values, id);
        values = "data.Structure.branchId";
        vector<string> branch_name;
        status = pt_structures->get(values, branch_name);
        values = "data.Structure.chainage";
        vector<double> chainage;
        status = pt_structures->get(values, chainage);
        if (id.size() == 0) { return; }

        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        QString layer_name = QString("Structures");
        QString group_name = QString("Area");

        QgsLayerTreeGroup * subTreeGroup = treeGroup->findGroup(QString(group_name));
        if (subTreeGroup == nullptr)  // Treegroup Area does not exist create it
        {
            treeGroup->addGroup(group_name);
            subTreeGroup = treeGroup->findGroup(group_name);
            subTreeGroup->setExpanded(true);  // true is the default 
            subTreeGroup->setItemVisibilityChecked(true);
            //QMessageBox::warning(0, "Message", QString("Create group: %1.").arg(name));
            subTreeGroup->setItemVisibilityCheckedRecursive(true);
        }
        // Now there is a tree group with name "Area"

        // create the vector layer for structures
        QgsVectorLayer * vl;
        QgsVectorDataProvider * dp_vl;
        QList <QgsField> lMyAttribField;

        int nr_attrib_fields = 2;

        lMyAttribField << QgsField("Structure Id (0-based)", QVariant::String)
                       << QgsField("Structure Id (1-based)", QVariant::String);
        lMyAttribField << QgsField("Structure name", QVariant::String);
        nr_attrib_fields++;

        QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
        vl = new QgsVectorLayer(uri, layer_name, "memory");
        vl->startEditing();
        dp_vl = vl->dataProvider();
        dp_vl->addAttributes(lMyAttribField);
        //dp_vl->createSpatialIndex();
        vl->updatedFields();

        QgsPointXY janm;
        struct _ntw_geom * ntw_geom = ugrid_file->get_network_geometry();
        struct _ntw_edges * ntw_edges = ugrid_file->get_network_edges();

        double xp;
        double yp;
        double rotation;
        for (int j = 0; j < id.size(); j++)
        {
            long status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
            QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
            QgsFeature MyFeature;
            MyFeature.setGeometry(MyPoints);

            MyFeature.initAttributes(nr_attrib_fields);
            int k = 0;
            MyFeature.setAttribute(0, QString("%1_b0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
            k++;
            MyFeature.setAttribute(1, QString("%1_b1").arg(j + 1));
            k++;
            MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(id[j]).trimmed()));

            dp_vl->addFeature(MyFeature);
        }
        vl->commitChanges();

        QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
        simple_marker->setSize(3.5);
        simple_marker->setColor(QColor(0, 0, 0));
        simple_marker->setFillColor(QColor(255, 0, 0));
        simple_marker->setShape(QgsSimpleMarkerSymbolLayerBase::Triangle);

        QgsSymbol * marker = QgsSymbol::defaultSymbol(QgsWkbTypes::PointGeometry);
        marker->changeSymbolLayer(0, simple_marker);

        //set up a renderer for the layer
        QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
        vl->setRenderer(mypRenderer);

        add_layer_to_group(vl, subTreeGroup);
        connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
    }
}
void qgis_umesh::create_1D_observation_point_vector_layer(UGRID * ugrid_file, READ_JSON * pt_structures, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (pt_structures != nullptr)
    {
        long status = -1;
        string values = "data.ObservationPoint.id";
        vector<string> obs_name;
        status = pt_structures->get(values, obs_name);
        if (obs_name.size() == 0)
        {
            string values = "data.ObservationPoint.name";
            status = pt_structures->get(values, obs_name);
            if (obs_name.size() == 0)
            {
                QMessageBox::warning(0, tr("Message: create_1D_observation_point_vector_layer"), QString(tr("Number of observation points is zero. JSON data: ")) + QString::fromStdString(values));
                return;
            }
        }
        values = "data.ObservationPoint.branchId";
        vector<string> branch_name;
        status = pt_structures->get(values, branch_name);
        if (branch_name.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_1D_observation_point_vector_layer"), QString(tr("Number of branch names is zero. JSON data: ")) + QString::fromStdString(values));
            return;
        }
        values = "data.ObservationPoint.chainage";
        vector<double> chainage;
        status = pt_structures->get(values, chainage);
        if (chainage.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_1D_observation_point_vector_layer"), QString(tr("Number of chainages is zero. JSON data: ")) + QString::fromStdString(values));
            return;
        }
        if (obs_name.size() != branch_name.size() || branch_name.size() != chainage.size() || obs_name.size() != chainage.size())
        {
            QMessageBox::warning(0, tr("Message: create_1D_observation_point_vector_layer"), QString(tr("Inconsistent data set. JSON data: ")) + QString::fromStdString(values) 
                + "\nObservation points: " + (int)obs_name.size()
                + "\nBranches: " + (int)branch_name.size()
                + "\nChainage: " + (int)chainage.size());
            return;
        }

        QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
        QString layer_name = QString("Observation points");
        QString group_name = QString("Area");

        QgsLayerTreeGroup * subTreeGroup = treeGroup->findGroup(QString(group_name));
        if (subTreeGroup == nullptr)  // Treegroup Area does not exist create it
        {
            treeGroup->addGroup(group_name);
            subTreeGroup = treeGroup->findGroup(group_name);
            subTreeGroup->setExpanded(true);  // true is the default 
            subTreeGroup->setItemVisibilityChecked(true);
            //QMessageBox::warning(0, "Message", QString("Create group: %1.").arg(name));
            subTreeGroup->setItemVisibilityCheckedRecursive(true);
        }
        // Now there is a tree group with name "Area"

        // create the vector layer for structures
        QgsVectorLayer * vl;
        QgsVectorDataProvider * dp_vl;
        QList <QgsField> lMyAttribField;

        int nr_attrib_fields = 2;

        lMyAttribField << QgsField("Observation point Id (0-based)", QVariant::String)
                       << QgsField("Observation point Id (1-based)", QVariant::String);
        lMyAttribField << QgsField("Observation point name", QVariant::String);
        nr_attrib_fields++;
        lMyAttribField << QgsField("Observation point rotation", QVariant::Double);
        nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
        vl = new QgsVectorLayer(uri, layer_name, "memory");
        vl->startEditing();
        dp_vl = vl->dataProvider();
        dp_vl->addAttributes(lMyAttribField);
        //dp_vl->createSpatialIndex();
        vl->updatedFields();

        struct _ntw_geom * ntw_geom = ugrid_file->get_network_geometry();
        struct _ntw_edges * ntw_edges = ugrid_file->get_network_edges();

        double xp;
        double yp;
        double rotation;
        for (int j = 0; j < obs_name.size(); j++)
        {
            long status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
            QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
            QgsFeature MyFeature;
            MyFeature.setGeometry(MyPoints);

            MyFeature.initAttributes(nr_attrib_fields);
            int k = 0;
            MyFeature.setAttribute(0, QString("%1_b0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
            k++;
            MyFeature.setAttribute(1, QString("%1_b1").arg(j + 1));
            k++;
            MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(obs_name[j]).trimmed()));
            k++;
            MyFeature.setAttribute(k, QString("%1").arg(double(j)/double(obs_name.size())*360.));

            dp_vl->addFeature(MyFeature);
        }
        vl->commitChanges();

        QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
        simple_marker->setSize(4.0);
        simple_marker->setColor(QColor(255, 0, 0));
        simple_marker->setFillColor(QColor(255, 255, 255));
        simple_marker->setShape(QgsSimpleMarkerSymbolLayerBase::Star);
        simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Observation point rotation")));

        QgsSymbol * marker = new QgsMarkerSymbol();
        marker->changeSymbolLayer(0, simple_marker);

        //set up a renderer for the layer
        QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
        vl->setRenderer(mypRenderer);

        add_layer_to_group(vl, subTreeGroup);
        connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
    }
}
void qgis_umesh::create_1D_external_forcing_vector_layer(UGRID * ugrid_file, READ_JSON * prop_tree, long epsg_code, QgsLayerTreeGroup * treeGroup)
{
    if (prop_tree != nullptr)
    {
        long status = -1;
        string values = "data.Lateral.id";
        vector<string> lateral_name;
        status = prop_tree->get(values, lateral_name);
        if (lateral_name.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"), QString(tr("Number of lateral discharges is zero. JSON data: ")) + QString::fromStdString(values));
        }
        else
        {
            values = "data.Lateral.branchId";
            vector<string> branch_name;
            status = prop_tree->get(values, branch_name);
            if (branch_name.size() == 0)
            {
                QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"), QString(tr("Number of branch names is zero. JSON data: ")) + QString::fromStdString(values));
                return;
            }
            values = "data.Lateral.chainage";
            vector<double> chainage;
            status = prop_tree->get(values, chainage);
            if (chainage.size() == 0)
            {
                QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"), QString(tr("Number of chainages is zero. JSON data: ")) + QString::fromStdString(values));
                return;
            }
            if (lateral_name.size() != branch_name.size() || branch_name.size() != chainage.size() || lateral_name.size() != chainage.size())
            {
                QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"), QString(tr("Inconsistent data set. JSON data: ")) + QString::fromStdString(values)
                    + "\nLateral points: " + (int)lateral_name.size()
                    + "\nBranches: " + (int)branch_name.size()
                    + "\nChainage: " + (int)chainage.size());
                return;
            }

            QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
            QString layer_name = QString("Lateral discharge");
            QString group_name = QString("Area");

            QgsLayerTreeGroup * subTreeGroup = treeGroup->findGroup(QString(group_name));
            if (subTreeGroup == nullptr)  // Treegroup Area does not exist create it
            {
                treeGroup->addGroup(group_name);
                subTreeGroup = treeGroup->findGroup(group_name);
                subTreeGroup->setExpanded(true);  // true is the default 
                subTreeGroup->setItemVisibilityChecked(true);
                //QMessageBox::warning(0, "Message", QString("Create group: %1.").arg(name));
                subTreeGroup->setItemVisibilityCheckedRecursive(true);
            }
            // Now there is a tree group with name "Area"

            // create the vector 
            QgsVectorLayer * vl;
            QgsVectorDataProvider * dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 2;

            lMyAttribField << QgsField("Lateral point Id (0-based)", QVariant::String)
                << QgsField("Lateral point Id (1-based)", QVariant::String);
            lMyAttribField << QgsField("Lateral point name", QVariant::String);
            nr_attrib_fields++;
            lMyAttribField << QgsField("Lateral point rotation", QVariant::Double);
            nr_attrib_fields++;

            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            struct _ntw_geom * ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges * ntw_edges = ugrid_file->get_network_edges();

            double xp;
            double yp;
            double rotation;
            for (int j = 0; j < lateral_name.size(); j++)
            {
                long status = compute_location_along_geometry(ntw_geom, ntw_edges, branch_name[j], chainage[j], &xp, &yp, &rotation);
                QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                QgsFeature MyFeature;
                MyFeature.setGeometry(MyPoints);

                MyFeature.initAttributes(nr_attrib_fields);
                int k = 0;
                MyFeature.setAttribute(0, QString("%1_b0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                k++;
                MyFeature.setAttribute(1, QString("%1_b1").arg(j + 1));
                k++;
                MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(lateral_name[j]).trimmed()));
                k++;
                MyFeature.setAttribute(k, QString("%1").arg(rotation));

                dp_vl->addFeature(MyFeature);
            }
            vl->commitChanges();

            //QgsSvgMarkerSymbolLayer * simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/OSGeo4W64/apps/qgis/svg/arrows/NorthArrow_11.svg"));
            //QgsSvgMarkerSymbolLayer * simple_marker = new QgsSvgMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/lateral.svg"));
            QgsRasterMarkerSymbolLayer * simple_marker = new QgsRasterMarkerSymbolLayer(QString("c:/Program Files/Deltares/qgis_umesh/icons/lateral.png"));
            simple_marker->setSize(4.0);
            simple_marker->setColor(QColor(0, 255, 0));
            simple_marker->setFillColor(QColor(0, 255, 0));
            simple_marker->setDataDefinedProperties(QgsPropertyCollection(QString("Lateral point rotation")));

            QgsSymbol * marker = new QgsMarkerSymbol();
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);

            add_layer_to_group(vl, subTreeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }


        status = -1;
        values = "data.boundary.nodeId";
        vector<string> bnd_name;
        status = prop_tree->get(values, bnd_name);
        if (bnd_name.size() == 0)
        {
            QMessageBox::warning(0, tr("Message: create_1D_external_forcing_vector_layer"), QString(tr("Number of boundary node is zero. JSON data: ")) + QString::fromStdString(values));
        }
        else
        {
            QList <QgsLayerTreeLayer *> tmp_layers = treeGroup->findLayers();
            QString layer_name = QString("Boundary nodes");
            QString group_name = QString("Area");

            QgsLayerTreeGroup * subTreeGroup = treeGroup->findGroup(QString(group_name));
            if (subTreeGroup == nullptr)  // Treegroup Area does not exist create it
            {
                treeGroup->addGroup(group_name);
                subTreeGroup = treeGroup->findGroup(group_name);
                subTreeGroup->setExpanded(true);  // true is the default 
                subTreeGroup->setItemVisibilityChecked(true);
                //QMessageBox::warning(0, "Message", QString("Create group: %1.").arg(name));
                subTreeGroup->setItemVisibilityCheckedRecursive(true);
            }
            // Now there is a tree group with name "Area"

            // create the vector 
            QgsVectorLayer * vl;
            QgsVectorDataProvider * dp_vl;
            QList <QgsField> lMyAttribField;

            int nr_attrib_fields = 2;

            lMyAttribField << QgsField("Boundary point Id (0-based)", QVariant::String)
                           << QgsField("Boundary point Id (1-based)", QVariant::String);
            lMyAttribField << QgsField("Boundary name", QVariant::String);
            nr_attrib_fields++;
            QString uri = QString("Point?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            struct _ntw_geom * ntw_geom = ugrid_file->get_network_geometry();
            struct _ntw_edges * ntw_edges = ugrid_file->get_network_edges();
            struct _ntw_nodes * ntw_nodes = ugrid_file->get_connection_nodes();

            double xp;
            double yp;
            for (int j = 0; j < bnd_name.size(); j++)
            {
                long status = find_location_boundary(ntw_nodes, bnd_name[j], &xp, &yp);

                QgsGeometry MyPoints = QgsGeometry::fromPointXY(QgsPointXY(xp, yp));
                QgsFeature MyFeature;
                MyFeature.setGeometry(MyPoints);

                MyFeature.initAttributes(nr_attrib_fields);
                int k = 0;
                MyFeature.setAttribute(0, QString("%1_b0").arg(j));  // arg(j, nsig, 10, QLatin1Char('0')));
                k++;
                MyFeature.setAttribute(1, QString("%1_b1").arg(j + 1));
                k++;
                MyFeature.setAttribute(k, QString("%1").arg(QString::fromStdString(bnd_name[j]).trimmed()));

                dp_vl->addFeature(MyFeature);
            }
            vl->commitChanges();

            QgsSimpleMarkerSymbolLayer * simple_marker = new QgsSimpleMarkerSymbolLayer();
            //QgsSvgMarkerSymbolLayer * simple_marker = new QgsSvgMarkerSymbolLayer(QString("d:/checkouts/git/qgis_plugins/qgis_umesh/svg/tmp_bridge_tui.svg"));
            simple_marker->setSize(4.0);
            simple_marker->setColor(QColor(0, 255, 0));
            simple_marker->setFillColor(QColor(0, 255, 0));
            simple_marker->setShape(QgsSimpleMarkerSymbolLayerBase::Diamond);

            QgsSymbol * marker = new QgsMarkerSymbol();
            marker->changeSymbolLayer(0, simple_marker);

            //set up a renderer for the layer
            QgsSingleSymbolRenderer *mypRenderer = new QgsSingleSymbolRenderer(marker);
            vl->setRenderer(mypRenderer);

            add_layer_to_group(vl, subTreeGroup);
            connect(vl, SIGNAL(crsChanged()), this, SLOT(CrsChanged()));  // changing coordinate system of a layer
        }
    }
}
long qgis_umesh::compute_location_along_geometry(struct _ntw_geom * ntw_geom, struct _ntw_edges * ntw_edges, string branch_name, double chainage_node, double * xp, double * yp, double * rotation)
{
    long status = -1;
    int nr_ntw = 1;  // TODO HACK just one network supported
    *rotation = 0.0;
    vector<double> chainage;
    for (int branch = 0; branch < ntw_geom->geom[nr_ntw - 1]->count; branch++)  // loop over the geometries
    {
        if (status == 0) { break; }
        if (QString::fromStdString(ntw_geom->geom[nr_ntw - 1]->name[branch]).trimmed() == QString::fromStdString(branch_name).trimmed())  // todo Check on preformance
        {
            double branch_length = ntw_edges->edge[nr_ntw - 1]->branch_length[branch];
            size_t geom_nodes_count = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->count;

            chainage.reserve(geom_nodes_count);
            chainage[0] = 0.0;
            for (int i = 1; i < geom_nodes_count; i++)
            {
                double x1 = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->x[i - 1];
                double y1 = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->y[i - 1];
                double x2 = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->x[i];
                double y2 = ntw_geom->geom[nr_ntw - 1]->nodes[branch]->y[i];

                chainage[i] = chainage[i - 1] + sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));  // todo HACK: this is just the euclidian distance
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
    return status;
}
long qgis_umesh::find_location_boundary(struct _ntw_nodes * ntw_nodes, string bnd_name, double * xp, double * yp)
{
    long status = -1;
    for (int i = 0; i < ntw_nodes->node[0]->count; i++)
    {
        if (QString::fromStdString(ntw_nodes->node[0]->name[i]).trimmed() == QString::fromStdString(bnd_name))
        {
            *xp = ntw_nodes->node[0]->x[i];
            *yp = ntw_nodes->node[0]->y[i];
            status = 0;
            return status;
        }
    }
    return status;
}


void qgis_umesh::add_layer_to_group(QgsVectorLayer * vl, QgsLayerTreeGroup * treeGroup)
{
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
    return sPluginType; // eerste, na selectie in plugin manager
}

// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin* classFactory(QgisInterface* iface)
{
    std::cout << "::classFactory" << std::endl;
    qgis_umesh * p = new qgis_umesh(iface);
    return (QgisPlugin*) p; // tweede na selectie in plugin manager
}

QGISEXTERN QString name()
{
    std::cout << "::name" << std::endl;
    return sName; // eerste vanuit QGIS
}

QGISEXTERN QString category(void)
{
    std::cout << "::category" << std::endl;
    return sCategory; // 
}

QGISEXTERN QString description()
{
    std::cout << "::description" << std::endl;
    return sDescription; // tweede vanuit QGIS
}

QGISEXTERN QString version()
{
    std::cout << "::version" << std::endl;
    return sPluginVersion;
}

QGISEXTERN QString icon() // derde vanuit QGIS
{
    char * icon_file;

    QDir executable_dir = QDir::currentPath();  // get current working directory  (_getcwd(current_dir, _MAX_PATH);)
    QString q_icon_file = executable_dir.dirName() + QString("/icons/file_open.png");

    icon_file = q_icon_file.toUtf8().data();
    sPluginIcon = new QString(icon_file);
    return *sPluginIcon;
}
// Delete ourself
QGISEXTERN void unload(QgisPlugin* the_qgis_umesh_pointer)
{
    the_qgis_umesh_pointer->unload();
    delete the_qgis_umesh_pointer;
}

