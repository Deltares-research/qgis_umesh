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
    //QMessageBox::information(0, "qgis_umesh::onWillRemoveChildren()", QString("Node: %1").arg(node->name()));
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
                //QMessageBox::information(0, "qgis_umesh::CrsChanged()", QString("Layer[0] name: %1").arg(layer[0]->layer()->name()));
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
            QMessageBox::information(0, "qgis_umesh::CrsChanged()", QString("Layer name: %1\nChecked: %2").arg(active_layer->name()).arg(checked));
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

    QString pluginsHome = QString("d:/bin/qgis_umesh");
    QDir plugins_home_dir = QDir(pluginsHome);
    if (!plugins_home_dir.exists())
    {
        QMessageBox::warning(0, tr("Message"), QString(tr("Missing installation directory:\n")) + pluginsHome);
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
    icon_open = get_icon_file(plugins_home_dir, "/icons/file_open.png");
    openAction = new QAction(icon_open, tr("&Open Map"));
    openAction->setToolTip(tr("Open UGRID 1D2D file"));
    openAction->setStatusTip(tr("Open UGRID file containing 1D, 2D and/or 1D2D meshes"));
    openAction->setEnabled(true);
    connect(openAction, SIGNAL(triggered()), this, SLOT(openFile()));

    icon_open_his_cf = get_icon_file(plugins_home_dir, "/icons/file_open.png");
    open_action_his_cf = new QAction(icon_open_his_cf, tr("&Open HIS"));
    open_action_his_cf->setToolTip(tr("Open CF compliant time series file"));
    open_action_his_cf->setStatusTip(tr("Open Climate and Forecast compliant time series file"));
    open_action_his_cf->setEnabled(true);
    connect(open_action_his_cf, SIGNAL(triggered()), this, SLOT(open_file_his_cf()));

    icon_inspect = get_icon_file(plugins_home_dir, "/icons/remoteolv_icon.png");
    inspectAction = new QAction(icon_inspect, tr("&Show map output"));
    inspectAction->setToolTip(tr("Show map output time manager"));
    inspectAction->setStatusTip(tr("Show time dependent map output as animation via time manager"));
    inspectAction->setEnabled(true);
    connect(inspectAction, SIGNAL(triggered()), this, SLOT(set_show_map_output()));

    icon_plotcfts = get_icon_file(plugins_home_dir, "/icons/plotcfts.png");
    plotcftsAction = new QAction();
    plotcftsAction->setIcon(icon_plotcfts);
    plotcftsAction->setText("PlotCFTS");
    plotcftsAction->setToolTip(tr("Start PlotCFTS to view time series"));
    plotcftsAction->setStatusTip(tr("Start plot program PlotCFTS to view Climate and Forecast compliant time series"));
    connect(plotcftsAction, SIGNAL(triggered()), this, SLOT(start_plotcfts()));

    icon_mapoutput = get_icon_file(plugins_home_dir, "/icons/map_output.png");
    mapoutputAction = new QAction();
    mapoutputAction->setIcon(icon_mapoutput);
    mapoutputAction->setText("Map settings ...");
    mapoutputAction->setToolTip(tr("Supply the settings for the map animation"));
    mapoutputAction->setStatusTip(tr("Supply the settings for the map animation"));
    mapoutputAction->setEnabled(true);
    connect(mapoutputAction, SIGNAL(triggered()), this, SLOT(mapPropertyWindow()));
    //connect(mapoutputAction, &QAction::triggered, MapTimeManagerWindow, &MapTimeManagerWindow::contextMenu);

    icon_open = get_icon_file(plugins_home_dir, "/icons/file_open.png");
    QAction * saveAction = new QAction(icon_open, tr("&Save"));
    saveAction->setToolTip(tr("Dummmy Item"));
    saveAction->setStatusTip(tr("Dummy item shown in statusbar"));
    saveAction->setEnabled(true);
    connect(saveAction, SIGNAL(triggered()), this, SLOT(openFile()));

    aboutAction = new QAction(tr("&About"), this);
    aboutAction->setToolTip(tr("Show the About box"));
    aboutAction->setStatusTip(tr("Show the application's About box"));
    aboutAction->setEnabled(true);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

//------------------------------------------------------------------------------
    _menuToolBar = new QToolBar();

    QMenuBar * janm = new QMenuBar();
    janm->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QMenu * janm1 = janm->addMenu("File");
    janm1->addAction(openAction);
    janm1->addAction(open_action_his_cf);

    janm1 = janm->addMenu("Output");
    janm1->addAction(inspectAction);
    janm1->addAction(plotcftsAction);

    janm1 = janm->addMenu("Settings");
    janm1->addAction(mapoutputAction);

    janm1 = janm->addMenu("Help");
    janm1->addAction(aboutAction);

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
    mMyCanvas = new MyCanvas(mQGisIface->mapCanvas());
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::set_enabled()
{
    if (mainAction->isChecked())
    {
        mainAction->setChecked(true);
        mQGisIface->mapCanvas()->setMapTool(mMyCanvas);
        //QMessageBox::warning(0, tr("Message"), QString("Plugin will be enabled \n"));
        _menuToolBar->setEnabled(true);
        openAction->setEnabled(true);
        open_action_his_cf->setEnabled(true);

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
        mQGisIface->mapCanvas()->unsetMapTool(mMyCanvas);
        //QMessageBox::warning(0, tr("Message"), QString("Plugin will be disabled \n"));
        _menuToolBar->setEnabled(false);
        openAction->setEnabled(false);
        inspectAction->setEnabled(false);
        open_action_his_cf->setEnabled(false);
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
    //QMessageBox::information(0, "Information", QString("qgis_umesh::show_map_output()\nTime manager: %1").arg(MapTimeManagerWindow::get_count()));

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
        QMessageBox::warning(0, "Fatal error", QString("qgis_umesh::show_map_output()\nVariable ugrid_file is a nullptr"));
        return;
    }
    for (int i = 0; i < globals->count; i++)
    {
        if (globals->attribute[i]->name == "Conventions")
        {
            conventions_found = true;
            if (globals->attribute[i]->cvalue.find("UGRID-1.") == string::npos)
            {
                QMessageBox::information(0, "Information", QString("Time manager will not start because this file is not UGRID-1.* compliant.\nThis file has file format: %1").arg(globals->attribute[i]->cvalue.c_str()));
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
            connect(mtm_widget, &MapTimeManagerWindow::customContextMenuRequested, mtm_widget, &MapTimeManagerWindow::contextMenu);
        }
    }
    else
    {
        QString fname = ugrid_file->get_filename().canonicalFilePath();
        QMessageBox::information(0, tr("Message"), QString("No time-series available in file:\n%1").arg(fname));
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
    QString * qtext;
    QString * msg_text;
    text = getversionstring_qgis_umesh();
    qtext = new QString(text);

    msg_text = new QString("Deltares\n");
    msg_text->append("Plot UGRID compliant 1D mesh and geometry, and 2D meshes\n");
    msg_text->append(qtext);
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
    //QMessageBox::warning(0, tr("Warning"), tr("netCDF file opened:\n%1").arg(ncfile.absoluteFilePath()));
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
    //QMessageBox::warning(0, tr("Warning"), tr("netCDF file opened:\n%1").arg(ncfile.absoluteFilePath()));
    _his_cf_file->read();
    _his_cf_files[_his_cf_fil_index] = _his_cf_file;
    activate_observation_layers();
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
    //QMessageBox::information(0, tr("qgis_umesh::start_plotcfts()"), tr("start_plotcfts"));
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
    QString prgm = QString("d:/bin/plotcfts/plotcfts.exe");
    QProcessEnvironment env;
    env.insert("PATH", "d:/bin/plotcfts"); // Add an environment variable
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
        //QMessageBox::information(0, "Information", QString("qgis_umesh::get_active_layer()\nActive layer: %1").arg(active_layer->name()));
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
                        //QMessageBox::information(0, "Information", QString("qgis_umesh::get_active_layer()\nGroup name: %1\nActive layer: %2").arg(myGroup->name()).arg(active_layer->name()));
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
        //QMessageBox::information(0, "Information", QString("qgis_umesh::get_active_layer()\nActive layer: %1").arg(active_layer->name()));
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
                        //QMessageBox::information(0, "Information", QString("qgis_umesh::get_active_layer()\nGroup name: %1\nActive layer: %2").arg(myGroup->name()).arg(active_layer->name()));
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
        mQGisIface->mapCanvas()->setMapTool(mMyCanvas);

        QList <QgsLayerTreeGroup *> groups = treeRoot->findGroups();
        for (int i = 0; i< groups.length(); i++)
        {
            //QMessageBox::warning(0, "Message", QString("_fil_index: %1+1").arg(_fil_index+1));
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
            //QMessageBox::warning(0, "Message", QString("Create group: %1").arg(name));
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
            //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D nodes"));
            if (mesh1d != NULL)
            {
                create_nodes_vector_layer(QString("Mesh1D nodes"), mesh1d->node[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D edges"));
                create_edges_vector_layer(QString("Mesh1D edges"), mesh1d->node[0], mesh1d->edge[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
            }

            // Mesh1D Topology Edges between connection nodes, Network Connection node, Network geometry
            ntw_nodes = ugrid_file->get_connection_nodes();
            ntw_edges = ugrid_file->get_network_edges();
            ntw_geom = ugrid_file->get_network_geometry();

            if (ntw_nodes != NULL)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D Connection nodes"));
                QString layer_name = QString("Mesh1D Connection nodes");
                create_nodes_vector_layer(layer_name, ntw_nodes->node[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
 
                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D geometry"));
                create_geometry_vector_layer(QString("Mesh1D geometry"), ntw_geom, mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);

                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D Topology edges"));
                create_edges_vector_layer(QString("Mesh1D Topology edges"), ntw_nodes->node[0], ntw_edges->edge[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
            }

            // Mesh1D Topology Edges between connection nodes, Network Connection node, Network geometry

            mesh_contact = ugrid_file->get_mesh_contact();

            if (mesh_contact != NULL)
            {
                create_edges_vector_layer(QString("Mesh contact edges"), mesh_contact->node[0], mesh_contact->edge[0], mapping->epsg, treeGroup);
            }

            // Mesh 2D edges and Mesh 2D nodes

            mesh2d = ugrid_file->get_mesh2d();
            if (mesh2d != NULL)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Mesh2D nodes"));
                create_nodes_vector_layer(QString("Mesh2D faces"), mesh2d->face[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);

                //QMessageBox::warning(0, tr("Warning"), tr("Mesh2D nodes"));
                create_nodes_vector_layer(QString("Mesh2D nodes"), mesh2d->node[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);

                //QMessageBox::warning(0, tr("Warning"), tr("Mesh2D edges"));
                create_edges_vector_layer(QString("Mesh2D edges"), mesh2d->node[0], mesh2d->edge[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
            }
            pgbar_value = 600;  // start of pgbar counter
            this->pgBar->setValue(pgbar_value);
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

        mQGisIface->mapCanvas()->unsetMapTool(mMyCanvas);
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
        mQGisIface->mapCanvas()->setMapTool(mMyCanvas);

        QList <QgsLayerTreeGroup *> groups = treeRoot->findGroups();
        for (int i = 0; i< groups.length(); i++)
        {
            //QMessageBox::warning(0, "Message", QString("_fil_index: %1+1").arg(_fil_index+1));
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
            //QMessageBox::warning(0, "Message", QString("Create group: %1").arg(name));
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
            //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D nodes"));
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

        mQGisIface->mapCanvas()->unsetMapTool(mMyCanvas);
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
    mQGisIface->mapCanvas()->unsetMapTool(mMyCanvas);

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
    //QMessageBox::warning(0, tr("Message"), QString("qgis_umesh::unload()"));
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::unload_vector_layers()
{
    // remove scratch layer
    QList< QgsMapLayer * > layers = mQGisIface->mapCanvas()->layers();
    int cnt = layers.count();
    //QMessageBox::warning(0, tr("Message"), QString("qgis_umesh::unload_vector_layers()\nActive layers: %1\n").arg(cnt));

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
            lMyAttribField << QgsField("Node Id (0-based)", QVariant::Int)
                << QgsField("Node Id (1-based)", QVariant::Int);
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

            QString uri = QString("MultiPoint?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QVector<QgsPointXY> janm;

            int nsig = long( log10(nodes->count) ) + 1;
            for (int j = 0; j < nodes->count; j++)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Count: %1, %5\nCoord: (%2,%3)\nName : %4\nLong name: %5").arg(ntw_nodes->nodes[i]->count).arg(ntw_nodes->nodes[i]->x[j]).arg(ntw_nodes->nodes[i]->y[j]).arg(ntw_nodes->nodes[i]->id[j]).arg(ntw_nodes->nodes[i]->name[j]));

                janm.append(QgsPointXY(nodes->x[j], nodes->y[j]));
                QgsGeometry MyPoints = QgsGeometry::fromMultiPointXY(janm);
                janm.clear();
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
            //QMessageBox::information(0, tr("Message: create_geometry_vector_layer"), QString("CRS layer: %1").arg(crs.authid()));

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
                    simple_marker->setColor(QColor(0, 0, 0));
                    simple_marker->setFillColor(QColor(255, 255, 255));  // water level point
                }
                else if (layer_name.contains("Mesh2D faces") )
                {
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

            lMyAttribField << QgsField("Geometry Id (0-based)", QVariant::Int)
                           << QgsField("Geometry Id (1-based)", QVariant::Int);
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
                        //QMessageBox::warning(0, tr("Warning"), tr("Edge: %1 (%2, %3)->(%4, %5)").arg(j).arg(x1).arg(y1).arg(x2).arg(y2));
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
                line_marker->setWidth(0.25);
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

            lMyAttribField << QgsField("Edge Id (0-based)", QVariant::Int)
                << QgsField("Edge Id (1-based)", QVariant::Int);
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
                //QMessageBox::warning(0, tr("Warning"), tr("Edge: %1 (%2, %3)->(%4, %5)").arg(j).arg(x1).arg(y1).arg(x2).arg(y2));
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
            lMyAttribField << QgsField("Observation point Id (0-based)", QVariant::Int)
                << QgsField("Observation point Id (1-based)", QVariant::Int);

            if (obs_points->location_long_name != nullptr)
            {
                lMyAttribField << QgsField(obs_points->location_long_name, QVariant::String);
                nr_attrib_fields++;
            }

            QString uri = QString("MultiPoint?crs=epsg:") + QString::number(epsg_code);
            vl = new QgsVectorLayer(uri, layer_name, "memory");
            vl->startEditing();
            dp_vl = vl->dataProvider();
            dp_vl->addAttributes(lMyAttribField);
            //dp_vl->createSpatialIndex();
            vl->updatedFields();

            QVector<QgsPointXY> janm;

            int nsig = long(log10(obs_points->location.size())) + 1;
            for (int j = 0; j < obs_points->location.size(); j++)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Count: %1, %5\nCoord: (%2,%3)\nName : %4\nLong name: %5").arg(ntw_nodes->nodes[i]->count).arg(ntw_nodes->nodes[i]->x[j]).arg(ntw_nodes->nodes[i]->y[j]).arg(ntw_nodes->nodes[i]->id[j]).arg(ntw_nodes->nodes[i]->name[j]));

                janm.append(QgsPointXY(obs_points->location[j].x[0], obs_points->location[j].y[0]));
                QgsGeometry MyPoints = QgsGeometry::fromMultiPointXY(janm);
                janm.clear();
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
            simple_marker->setSize(3.5);
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

            lMyAttribField << QgsField("Cross section Id (0-based)", QVariant::Int)
                           << QgsField("Cross section Id (1-based)", QVariant::Int);
                    
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
    QMessageBox::information(0, tr("Message"), QString("Dummy slot called"));
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

