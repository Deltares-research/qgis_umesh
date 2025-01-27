echo off

set netCDF_DIR=c:\Program Files\netCDF 4.9.2\
set PATH=c:/OSGeo4W/apps/Qt5/;%PATH%
set PATH=c:\boost\Boost_1_85_0\;%PATH%

set BOOST_INCLUDEDIR=C:\boost\boost_1_85_0\
set BOOST_LIBRARYDIR=C:\boost\boost_1_85_0\lib64-msvc-14.2
set BOOST_ROOT=C:\boost\boost_1_85_0\boost

del _build

copy packages\include\qgis_sumesh_version.h.vcs packages\include\qgis_sumesh_version.h 
rem copy packages\include\qqis_sumesh_version.rc.vcs packages\include\qgis_sumesh_version.rc 


echo on
cmake -B _build > cmake_configure.log 2>&1
cmake --build _build --verbose > cmake_build.log 2>&1
