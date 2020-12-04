# QGIS UMESH
QGIS plugin to plot 1D, 1D2D and 2D time series map results as animation. The map results should be stored on a netCDF file according the UGRID standard for 1D meshes, 1D2D contact meshes and/or 2D meshes. All types could be in one netCDF file. 

![alt tag](doc/pictures/oosterschelde_velocity_arrow.png)

## To build
To build the plugin qgis_umesh you have to install QGIS (OSGeo4W network installer (64 bit), https://qgis.org/en/site/forusers/download.html ), QT 5.12 LTS, BOOST 1.72.0 and netCDF4.
The windows solution will place the qgis_umesh.dll on the qgis plugin directory (ex. c:\OSGeo4W64\apps\qgis\plugins\qgis_umesh.dll)

## Development environment
At this moment the development environment is based on Visual Studio 2017.
 
## Environment variables
Environment variables (example)
QT5DIR64=c:\Qt\Qt5.12.6\5.12.6\msvc2017_64\
QT5DIR64OSGEO=c:\OSGeo4W64\apps\Qt5

## Link Libraries
netCDF library:
debug and release folder contain the same libraries
./lib/x64/release/netcdf.lib
./lib/x64/debug/netcdf.lib
              
qgis library:
debug and release folder contain the same libraries
./lib/x64/release/qgis_analysis.lib
                 /qgis_core.lib
                 /qgis_gui.lib
                 /qgis_networkanalysis.lib
./lib/x64/debug/qgis_analysis.lib
               /qgis_core.lib
               /qgis_gui.lib
               /qgis_networkanalysis.lib



## Installing QGIS from OSGeo4W
Installing QGIS from OSGeo4W network installer (64 bit)
After a few screens.
Selectpackages to install.
Desktop:
    qgis: QGIS Desktop
    qgis-full: QGIS Full Desktop (meta package for express install)
Libs: 
    QGIS-devel: QGIS development files
    QT5-libs: Qt5 runtime libraries
    qt5-libs-debug
    qt5-libs-debug-pdb
    qt5-libs-pdb
    qt5-devel; qt5 headers and libraries (Development)
Probably not needed
    qt5-qml: Qt5 QML
    qt5-tools: Qt5Designe & linguist (Development)
            
## Note (QGIS 3.14.16)
When compiling the source code I had to adjusted the file
c:\OSGeo4W64\apps\qgis\include\qgsabstractgeometry.h
An extra define of M_PI is added.

Line +/- 520
#define M_PI 3.14159265358979323846264338327950288

## Note (QGIS 3.16.00)
When QGIS 3.16.00 does close/crash without any message if reading a netCDF file, please use QGIS 3.16.01.
 
end document
          