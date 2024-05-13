#include "qgis_umesh_version.h"
#include "qgis_umesh.h"
#include "perf_timer.h"

#if defined(WIN64)
#  include <windows.h>
#  define strdup _strdup
#endif

#define EXPERIMENT 1

/* static */ const QString qgis_umesh::s_ident = QObject::tr("@(#)" qgis_umesh_company ", " qgis_umesh_program ", " qgis_umesh_version_number ", " qgis_umesh_arch", " __DATE__", " __TIME__);
/* static */ const QString qgis_umesh::s_name = QObject::tr("" qgis_umesh_company ", " qgis_umesh_program " Development");
/* static */ const QString qgis_umesh::s_description = QObject::tr("Plugin to read 1D2D3D (un)structured meshes; UGRID/SGRID-format (" __DATE__", " __TIME__")");
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
    this->m_hvl = new HVL(mQGisIface);

    this->pgBar = new QProgressBar();
    this->pgBar->setMaximum(1000);
    this->pgBar->setValue(0);
    this->pgBar->hide();

    status_bar = mQGisIface->statusBarIface();
    status_bar->addPermanentWidget(this->pgBar, 0, QgsStatusBar::Anchor::AnchorRight);

    QgsLayerTree* treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible}
    //connect(treeRoot, &QgsLayerTree::removedChildren, this, &qgis_umesh::onWillRemoveChildren);
    connect(treeRoot, SIGNAL(removedChildren(QgsLayerTreeNode*, int, int)), this, SLOT(onWillRemoveChildren(QgsLayerTreeNode*, int, int)));
    connect(treeRoot, SIGNAL(layerWillBeRemoved(QString)), this, SLOT(onRemovedChildren(QString)));
}
void qgis_umesh::onWillRemoveChildren(QgsLayerTreeNode* node, int indexFrom, int indexTo)
{
    Q_UNUSED(indexTo);
    QString name = node->name();  // contains the group name
    QList <QgsLayerTreeNode*> children =  node->children();
    if (children.size() == 0 && (name.contains("GRID - ") || name.contains("History -")))
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
            for (int i = m_grid_file.size() - 1; i >= 0; --i)  // counter 'i' have to be an integer, there is a test on >=
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
    connect(this->mQGisIface, SIGNAL(currentLayerChanged(QgsMapLayer*)), this, SLOT(set_enabled()));

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
    open_action_map = new QAction(icon_open, tr("&Open GRID ..."));
    open_action_map->setToolTip(tr("Open SGRID 2D or UGRID 1D2D3D file"));
    open_action_map->setStatusTip(tr("Open UGRID file containing 1D2D3D data or SGRID file containing 2D data"));
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
    QAction* saveAction = new QAction(icon_open, tr("&Save"));
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

    QMenuBar* janm = new QMenuBar();
    janm->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QMenu* janm1 = janm->addMenu("File");
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
    //mMyEditTool = new MyEditTool(mQGisIface->mapCanvas());
    mMyCanvas = new MyCanvas(mQGisIface);
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::set_enabled()
{
    if (mainAction->isChecked())
    {
        //QMessageBox::warning(0, tr("Message"), QString("Plugin will be enabled.\n"));
        mainAction->setChecked(true);
        _menuToolBar->setEnabled(true);
        open_action_map->setEnabled(true);
        open_action_his_cf->setEnabled(true);
        open_action_mdu->setEnabled(true);

        inspectAction->setEnabled(false);
        GRID* active_grid_file = m_hvl->get_active_grid_file("");
        if (active_grid_file != nullptr)
        {
            inspectAction->setEnabled(true);
        }
    }
    else
    {
        //QMessageBox::warning(0, tr("Message"), QString("Plugin will be disabled.\n"));
        mainAction->setChecked(false);
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
void qgis_umesh::show_map_output(GRID* grid_file)
{
    //QMessageBox::information(0, "Information", QString("qgis_umesh::show_map_output()\nTime manager: %1.").arg(MapTimeManagerWindow::get_count()));

    // Check on at least UGRID-1.0 file formaat
    long nr_times = 0;
    struct _global_attributes* globals;

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
            QgsMapLayer* active_layer = mQGisIface->activeLayer();
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
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::experiment()
{
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::mapPropertyWindow()
{
    if (MapPropertyWindow::get_count() == 0)  // create a window if it is not already there.
    {
        MapPropertyWindow* map_property = new MapPropertyWindow(mMyCanvas);
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
    char* pdf_document = manual.data();

    FILE* fp = fopen(pdf_document, "r");
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
int qgis_umesh::QT_SpawnProcess(int waiting, char* prgm, char** args)
{
    int i = 0;
    int status;
    QStringList argList;

    status = -1;  /* set default status as not started*/

    QProcess* proc = new QProcess();

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
    char* text;
    char* source_url;
    QString* qtext;
    QString* qsource_url;
    QString* msg_text;
    text = getversionstring_qgis_umesh();
    qtext = new QString(text);
    source_url = getsourceurlstring_qgis_umesh();
    qsource_url = new QString(source_url);

    msg_text = new QString("Deltares\n");
    msg_text->append("Plot results on UGRID/SGRID compliant meshes. 1D mesh with its geometry, 1D2D, 2D and 3D meshes.\n");
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
    QString* str = new QString();
    QFileDialog* fd = new QFileDialog();
    QStringList* list = new QStringList();

    str->clear();

    str->append("netCDF files");
    str->append(" (*.nc)");
    list->append(*str);
    str->clear();

    str->append("UGRID/SGRID map");
    str->append(" (*_map.nc)");
    list->append(*str);
    str->clear();

    str->append("UGRID/SGRID mesh");
    str->append(" (*_net.nc)");
    list->append(*str);
    str->clear();

//    str->append("UGRID/SGRID");
//    str->append(" (*_clm.nc)");
//    list->append(*str);
//    str->clear();

    str->append("All files");
    str->append(" (*.*)");
    list->append(*str);

    fd->setWindowTitle("Open (Un)structured grid file");
    fd->setNameFilters(*list);
    fd->selectNameFilter(list->at(0));
    fd->setFileMode(QFileDialog::ExistingFiles);  // Enable multiple file selection at once

    //QDir path("d:/mooiman/home/models/delft3d/grids/test_qgis");
    //if (path.exists())
    //{
    //    fd->setDirectory(path);
    //}

    bool canceled = fd->exec() != QDialog::Accepted;
    if (!canceled)
    {
        QStringList* QFilenames = new QStringList();
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
    m_hvl->set_grid_file(m_grid_file);

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
    QString* str = new QString();
    QFileDialog* fd = new QFileDialog();
    QStringList* list = new QStringList();

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

    //QDir path("d:/mooiman/home/models/delft3d/grids/test_qgis");
    //if (path.exists())
    //{
    //    fd->setDirectory(path);
    //}

    bool canceled = fd->exec() != QDialog::Accepted;
    if (!canceled)
    {
        QStringList* QFilenames = new QStringList();
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
#if DO_TIMING == 1
    auto start = std::chrono::steady_clock::now();
#endif
    char* fname = strdup(ncfile.absoluteFilePath().toUtf8());
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
    HISCF* _his_cf_file = m_his_cf_file.back();
    //QMessageBox::warning(0, tr("Warning"), tr("netCDF file opened:\n%1.").arg(ncfile.absoluteFilePath()));
    _his_cf_file->read();
    activate_observation_layers();

#if DO_TIMING == 1
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapse_time = end - start;
    QString msg = QString(tr("Timing reading meta data from netCDF file \"%1\": %2 [sec]").arg(ncfile.fileName()).arg(elapse_time.count()));
    QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
#endif
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::open_file_mdu()
{
    QString* str = new QString();
    QFileDialog* fd = new QFileDialog();
    QStringList* list = new QStringList();

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
        QStringList* QFilenames = new QStringList();
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
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::open_file_mdu(QFileInfo jsonfile)
{
    std::string fname = jsonfile.absoluteFilePath().toStdString();
    JSON_READER* pt_mdu = new JSON_READER(fname);
    if (pt_mdu == nullptr)
    {
        QMessageBox::warning(0, tr("Warning"), tr("Cannot open JSON file:\n%1.").arg(jsonfile.absoluteFilePath()));
        return;
    }
    m_mdu_files.push_back(pt_mdu);
    //MDU* _mdu_file = new MDU(jsonfile, this->pgBar);
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
        QString msg = QString(tr("No UGRID/SGRID mesh file given.\nTag \"%2\" does not exist in file \"%1\".").arg(QString::fromStdString(json_key)).arg(qname));
        QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Warning, true);
        return;
    }
    QgsLayerTree* treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
    QgsLayerTreeGroup* myGroup = treeRoot->findGroup(QString("GRID - %1").arg(jsonfile.fileName()));
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
                JSON_READER* pt_obs = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_grid_mapping();
                m_hvl->create_vector_layer_observation_point(grid_file, pt_obs, mapping->epsg, myGroup);  // i.e. a JSON file
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
                JSON_READER* pt_extold_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_grid_mapping();
                m_hvl->create_vector_layer_1D_external_forcing(grid_file, pt_extold_file, mapping->epsg, myGroup);  // i.e. a JSON file
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
                JSON_READER* pt_ext_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_grid_mapping();
                m_hvl->create_vector_layer_1D_external_forcing(grid_file, pt_ext_file, mapping->epsg, myGroup);  // i.e. a JSON file
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
                JSON_READER* pt_struct_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_grid_mapping();
                m_hvl->create_vector_layer_structure(grid_file, pt_struct_file, mapping->epsg, myGroup);  // i.e. a JSON file
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
                JSON_READER* pt_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_grid_mapping();
                m_hvl->create_vector_layer_drypoints(grid_file, pt_file, mapping->epsg, myGroup);  // i.e. a JSON file
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
                JSON_READER* pt_profloc = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_grid_mapping();
                m_hvl->create_vector_layer_sample_point(grid_file, pt_profloc, mapping->epsg, myGroup);  // i.e. a JSON file
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
                JSON_READER* pt_obs_cross_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_grid_mapping();
                m_hvl->create_vector_layer_observation_cross_section(grid_file, pt_obs_cross_file, mapping->epsg, myGroup);  // i.e. a JSON file
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
                JSON_READER* pt_thin_dam_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_grid_mapping();
                m_hvl->create_vector_layer_thin_dams(grid_file, pt_thin_dam_file, mapping->epsg, myGroup);  // i.e. a JSON file
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
                JSON_READER* pt_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_grid_mapping();
                m_hvl->create_vector_layer_fixed_weir(grid_file, pt_file, mapping->epsg, myGroup);  // i.e. a JSON file
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
                JSON_READER* pt_file = new JSON_READER(fname);
                GRID* grid_file = m_grid_file[_fil_index];
                if (grid_file->get_filename().fileName() != QString::fromStdString(ncfile[0]))
                {
                    QMessageBox::warning(0, tr("qgis_umesh::open_file_mdu"), tr("Mesh files not the same:\n\"%1\",\n\"%2\".").arg(QString::fromStdString(ncfile[0])).arg(grid_file->get_filename().fileName()));
                    return;
                }
                struct _mapping* mapping;
                mapping = grid_file->get_grid_mapping();
                m_hvl->create_vector_layer_1D_cross_section(grid_file, pt_file, mapping->epsg, myGroup);  // i.e. a JSON file
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
                mapping = grid_file->get_grid_mapping();
                m_hvl->create_vector_layer_1D_retention(grid_file, pt_file, mapping->epsg, myGroup);  // i.e. a JSON file
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
    QString* str = new QString();
    QFileDialog* fd = new QFileDialog();
    QStringList* list = new QStringList();

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
        QStringList* QFilenames = new QStringList();
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
    JSON_READER* pt_link1d2d = new JSON_READER(fname);
    if (pt_link1d2d == nullptr)
    {
        QMessageBox::warning(0, tr("Warning"), tr("Cannot open JSON file:\n%1.").arg(jsonfile.absoluteFilePath()));
        return;
    }

    long epsg = 0;
    m_hvl->create_vector_layer_1D2D_link(pt_link1d2d, epsg);  // i.e. a JSON file
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::open_file_obs_point_json()
{
    QString fname;
    QString* str = new QString();
    QFileDialog* fd = new QFileDialog();
    QStringList* list = new QStringList();

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
        QStringList* QFilenames = new QStringList();
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
    JSON_READER* pt_file = new JSON_READER(fname);
    if (pt_file == nullptr)
    {
        QMessageBox::warning(0, tr("Warning"), tr("Cannot open JSON file:\n%1.").arg(jsonfile.absoluteFilePath()));
        return;
    }

    long epsg = 0;
    QgsLayerTree* treeRoot = QgsProject::instance()->layerTreeRoot();
    QgsLayerTreeGroup* treeGroup;
    treeGroup = m_hvl->get_subgroup(treeRoot, QString("Location point"));

    std::vector<std::string> token = m_hvl->tokenize(pt_file->get_filename(), '/');
    GRID* grid_file = new GRID();
    long status = grid_file->open(QFileInfo(QString::fromStdString(token[token.size()-1])), nullptr);
    if (status == 0) { m_hvl->create_vector_layer_observation_point(grid_file, pt_file, epsg, treeGroup); }  // i.e. a JSON file
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::set_show_map_output()
{
    GRID* grid_file = m_hvl->get_active_grid_file("");
    if (grid_file != nullptr) { show_map_output(grid_file); }
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::start_plotcfts()
{
    //QMessageBox::information(0, tr("qgis_umesh::start_plotcfts()"), tr("start_plotcfts."));
    QString program_files = QProcessEnvironment::systemEnvironment().value("ProgramFiles", "");
    QString prgm_env = program_files + QString("/deltares/plotcfts");
    QString prgm = program_files + QString("/deltares/plotcfts/plotcfts.exe");
    QDir program_files_dir = QDir(prgm_env);
    if (!program_files_dir.exists())
    {
        QMessageBox::warning(0, tr("Message"), QString(tr("Missing installation directory:\n")) + prgm_env + QString(".\nProgram PlotCFTS will not start."));
        return;
    }
    HISCF* _his_cf_file = get_active_his_cf_file("");
    if (_his_cf_file == nullptr)
    {
        QMessageBox::warning(0, tr("Warning"), tr("There is no layer selected which contains a Climate and Forecast compliant time series set.\nTime series plot program PlotCFTS will start without preselected history file."));
    }
    //QgsVectorLayer* theVectorLayer = dynamic_cast<QgsVectorLayer*>(active_layer);
    //QgsFeature feature = theVectorLayer->getFeature();
    QStringList argList;
    QProcess* proc = new QProcess();
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
//-----------------------------------------------------------------------------
//
HISCF* qgis_umesh::get_active_his_cf_file(QString layer_id)
{
    // Function to find the filename belonging by the highlighted layer,
    // or to which group belongs the highligthed layer,
    // or the highlighted group
    QgsLayerTree* treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
    QList <QgsLayerTreeGroup*> groups = treeRoot->findGroups();
    QgsMapLayer* active_layer = NULL;
    HISCF* _his_cf_file = nullptr;

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
                QgsLayerTreeGroup* myGroup = treeRoot->findGroup(QString("History - %1").arg(m_his_cf_file[_his_cf_fil_index]->get_filename().fileName()));
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
            QgsLayerTreeGroup* myGroup = treeRoot->findGroup(QString("History - %1").arg(m_his_cf_file[_his_cf_fil_index]->get_filename().fileName()));
            if (myGroup != nullptr)
            {
                QList <QgsLayerTreeLayer*> layers = myGroup->findLayers();
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
    struct _ntw_nodes* ntw_nodes;
    struct _ntw_edges* ntw_edges;
    struct _ntw_geom* ntw_geom;
    struct _mesh1d* mesh_1d;
    struct _mesh2d* mesh_2d;
    struct _mesh_contact* mesh_contact;

    if (_fil_index == -1)
    {
        return;
    }

    QgsLayerTree* treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible

    if (mainAction->isChecked())
    {
        QList <QgsLayerTreeGroup*> groups = treeRoot->findGroups();
        for (int i = 0; i < groups.length(); i++)
        {
            //QMessageBox::warning(0, "Message", QString("_fil_index: %1+1.").arg(_fil_index+1));
            for (int j = 0; j < _fil_index + 1; j++)
            {
                QgsLayerTreeGroup* myGroup = treeRoot->findGroup(QString("GRID Mesh - %1").arg(j + 1));
                if (myGroup != nullptr)
                {
                    myGroup->setItemVisibilityChecked(true);
                }
            }
        }

        QgsLayerTreeGroup* treeGroup = treeRoot->findGroup(QString("GRID - %1").arg(m_grid_file[_fil_index]->get_filename().fileName()));
        if (treeGroup == nullptr)
        {
            QString fname = m_grid_file[_fil_index]->get_filename().fileName();
            QString name = QString("GRID - %1").arg(fname);
            treeGroup = treeRoot->insertGroup(_fil_index, name);  // set this group on top if _fil_index == 0
            treeGroup->setExpanded(true);  // true is the default
            treeGroup->setItemVisibilityChecked(true);
            treeGroup->setItemVisibilityCheckedRecursive(true);
        }

        if (_fil_index != -1)
        {
            GRID* grid_file = m_grid_file[_fil_index];
            struct _mapping* mapping = grid_file->get_grid_mapping();
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
                m_hvl->create_vector_layer_nodes(fname, QString("Mesh1D nodes"), mesh_1d->node[0], mapping->epsg, treeGroup);
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
                m_hvl->create_vector_layer_edges(fname, QString("Mesh1D edges"), mesh_1d->node[0], mesh_1d->edge[0], mapping->epsg, treeGroup);
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
                m_hvl->create_vector_layer_nodes(fname, layer_name, ntw_nodes->node[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);

                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D geometry."));
                m_hvl->create_vector_layer_geometry(fname, QString("Mesh1D geometry"), ntw_geom, mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);

                //QMessageBox::warning(0, tr("Warning"), tr("Mesh1D Topology edges."));
                m_hvl->create_vector_layer_edges(fname, QString("Mesh1D Topology edges"), ntw_nodes->node[0], ntw_edges->edge[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);
            }

            // Mesh1D Topology Edges between connection nodes, Network Connection node, Network geometry

            mesh_contact = grid_file->get_mesh_contact();

            if (mesh_contact != nullptr)
            {
                m_hvl->create_vector_layer_edges(fname, QString("Mesh contact edges"), mesh_contact->node[0], mesh_contact->edge[0], mapping->epsg, treeGroup);
            }

            // Mesh 2D edges and Mesh 2D nodes

            mesh_2d = grid_file->get_mesh_2d();
            if (mesh_2d != nullptr)
            {
                //QMessageBox::warning(0, tr("Warning"), tr("Mesh2D nodes."));
                if (mesh_2d->face[0]->count > 0)
                {
                    m_hvl->create_vector_layer_nodes(fname, QString("Mesh2D faces"), mesh_2d->face[0], mapping->epsg, treeGroup);
                    pgbar_value += 10;
                    this->pgBar->setValue(pgbar_value);
                }

                //QMessageBox::warning(0, tr("Warning"), tr("Mesh2D nodes."));
                m_hvl->create_vector_layer_nodes(fname, QString("Mesh2D nodes"), mesh_2d->node[0], mapping->epsg, treeGroup);
                pgbar_value += 10;
                this->pgBar->setValue(pgbar_value);

                if (mesh_2d->edge[0]->count > 0)
                {
                    m_hvl->create_vector_layer_edges(fname, QString("Mesh2D edges"), mesh_2d->node[0], mesh_2d->edge[0], mapping->epsg, treeGroup);
                    pgbar_value += 10;
                    this->pgBar->setValue(pgbar_value);
                }
            }
            pgbar_value = 600;  // start of pgbar counter

            this->pgBar->setValue(pgbar_value);

            //
            // get the time independent variables and list them in the layer-panel as treegroup
            //
            struct _mesh_variable* var = grid_file->get_variables();

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
                    QgsLayerTreeGroup* subTreeGroup = treeGroup->findGroup(name);

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
                                double* z_value = std_data_at_edge.GetValueAtIndex(0, 0);
                                m_hvl->create_vector_layer_data_on_edges(fname, var->variable[i], mesh_2d->node[0], mesh_2d->edge[0], z_value, mapping->epsg, subTreeGroup);
                            }
                            if (var->variable[i]->location == "edge" && var->variable[i]->topology_dimension == 2 && var->variable[i]->nc_type == NC_INT)
                            {
                                QString name1 = QString("Edge type");
                                subTreeGroup->addGroup(name1);
                                QgsLayerTreeGroup* sGroup = treeGroup->findGroup(name1);

                                DataValuesProvider2D<double>std_data_at_edge = grid_file->get_variable_values(var->variable[i]->var_name);
                                double* z_value = std_data_at_edge.GetValueAtIndex(0, 0);
                                m_hvl->create_vector_layer_edge_type(fname, var->variable[i], mesh_2d->node[0], mesh_2d->edge[0], z_value, mapping->epsg, sGroup);
                            }
                            if (var->variable[i]->location == "face" && var->variable[i]->topology_dimension == 2 && var->variable[i]->nc_type == NC_DOUBLE)
                            {
                                if (var->variable[i]->sediment_index == -1)
                                {
                                    DataValuesProvider2D<double>std_data_at_face = grid_file->get_variable_values(var->variable[i]->var_name, var->variable[i]->sediment_index);
                                    double* z_value = std_data_at_face.GetValueAtIndex(0, 0);
                                    m_hvl->create_vector_layer_data_on_nodes(fname, var->variable[i], mesh_2d->face[0], z_value, mapping->epsg, subTreeGroup);
                                }
                                else
                                {
                                    QgsLayerTreeGroup* Group = m_hvl->get_subgroup(subTreeGroup, QString("Sediment"));

                                    DataValuesProvider2D<double>std_data_at_face = grid_file->get_variable_values(var->variable[i]->var_name, var->variable[i]->sediment_index);
                                    double* z_value = std_data_at_face.GetValueAtIndex(var->variable[i]->sediment_index, 0);
                                    m_hvl->create_vector_layer_data_on_nodes(fname, var->variable[i], mesh_2d->face[0], z_value, mapping->epsg, Group);
                                    Group->setExpanded(false);
                                }
                            }
                            if (var->variable[i]->location == "node" && var->variable[i]->topology_dimension == 2 && var->variable[i]->nc_type == NC_DOUBLE)
                            {
                                DataValuesProvider2D<double>std_data_at_node = grid_file->get_variable_values(var->variable[i]->var_name);
                                double* z_value = std_data_at_node.GetValueAtIndex(0, 0);
                                m_hvl->create_vector_layer_data_on_nodes(fname, var->variable[i], mesh_2d->node[0], z_value, mapping->epsg, subTreeGroup);
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
        QList <QgsLayerTreeGroup*> groups = treeRoot->findGroups();
        for (int i = 0; i < groups.length(); i++)
        {
            QString name = groups[i]->name();
            QgsLayerTreeGroup* myGroup = treeRoot->findGroup(name);
            myGroup->setItemVisibilityChecked(false);
        }
    }
}
//
//-----------------------------------------------------------------------------
//
void qgis_umesh::activate_observation_layers()
{
    std::vector<_location_type*> obs_type;

    if (_his_cf_fil_index < 0) { return; }  // No history file opened

    QgsLayerTree* treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible

    if (mainAction->isChecked())
    {
        QList <QgsLayerTreeGroup*> groups = treeRoot->findGroups();
        for (int i = 0; i < groups.length(); i++)
        {
            //QMessageBox::warning(0, "Message", QString("_fil_index: %1_b1.").arg(_fil_index+1));
            for (int j = 0; j < _his_cf_fil_index + 1; j++)
            {
                QgsLayerTreeGroup* myGroup = treeRoot->findGroup(QString("History - %1").arg(m_his_cf_file[_his_cf_fil_index]->get_filename().fileName()));
                if (myGroup != nullptr)
                {
                    myGroup->setItemVisibilityChecked(true);
                }
            }
        }

        QgsLayerTreeGroup* treeGroup = treeRoot->findGroup(QString("History - %1").arg(m_his_cf_file[_his_cf_fil_index]->get_filename().fileName()));
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
            HISCF* _his_cf_file = m_his_cf_file[_his_cf_fil_index];
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
                        m_hvl->create_vector_layer_observation_point(fname, QString(obs_type[i]->location_long_name), obs_type[i], mapping->epsg, treeGroup);
                    }
                    else if (obs_type[i]->type == OBSERVATION_TYPE::OBS_POLYLINE)
                    {
                        m_hvl->create_vector_layer_observation_polyline(fname, QString(obs_type[i]->location_long_name), obs_type[i], mapping->epsg, treeGroup);
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
        QList <QgsLayerTreeGroup*> groups = treeRoot->findGroups();
        for (int i = 0; i < groups.length(); i++)
        {
            QString name = groups[i]->name();
            QgsLayerTreeGroup* myGroup = treeRoot->findGroup(name);
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
    QgsLayerTree* treeRoot = QgsProject::instance()->layerTreeRoot();  // root is invisible
    QList< QgsLayerTreeGroup* > groups = treeRoot->findGroups();
    for (int i = 0; i < groups.size(); i++)
    {
        QgsLayerTreeGroup* janm = groups.at(i);
        QString str = janm->name();
        if (str.contains("GRID - ") ||
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
    QList< QgsMapLayer* > layers = mQGisIface->mapCanvas()->layers();

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
//
//-----------------------------------------------------------------------------
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
    return int(qgis_umesh::s_plugin_type); // eerste, na selectie in plugin manager
}

// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin* classFactory(QgisInterface* iface)
{
    //QgsMessageLog::logMessage("::classFactory()", "QGIS umesh", Qgis::Info, true);
    qgis_umesh* p = new qgis_umesh(iface);
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
    return &qgis_umesh::s_category; 
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
    //QString q_icon_file = ("C:/Program Files/deltares/qgis_umesh/icons/qgis_umesh.png");
    //* static */ const QString qgis_umesh::s_plugin_icon = "C:/Program Files/deltares/qgis_umesh/icons/qgis_umesh.png";
    //return &qgis_umesh::s_plugin_icon;

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
