echo off

rem set netCDF_DIR=c:\Program Files\netCDF 4.9.2\
rem set PATH=c:/OSGeo4W/apps/Qt5/;%PATH%
rem set PATH=c:\boost\Boost-1.85.0\;%PATH%

del _build

copy packages\include\qgis_sumesh_version.h.vcs packages\include\qgis_sumesh_version.h 
rem copy packages\include\qqis_sumesh_version.rc.vcs packages\include\qgis_sumesh_version.rc 


echo on
cmake -B _build > cmake_configure.log 2>&1
cmake --build _build --verbose > cmake_build.log 2>&1
