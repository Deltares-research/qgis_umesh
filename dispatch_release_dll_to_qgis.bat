rem copy  .\bin\x64\Release\qgis_umesh.dll c:\OSGeo4W64\apps\qgis\plugins\*.*
copy  .\bin\x64\Release\qgis_umesh.pdb c:\OSGeo4W64\apps\qgis\plugins\*.*

copy .\doc\qgis_umesh_um.pdf            "c:\Program Files\Deltares\qgis_umesh\doc\*.*"
copy .\doc\qgis_umesh_installation.pdf  "c:\Program Files\Deltares\qgis_umesh\doc\*.*"
copy .\icons\*.*                        "c:\Program Files\Deltares\qgis_umesh\icons\*.*"

rem copy to Bulletin
rem copy "c:\OSGeo4W64\apps\qgis\plugins\qgis_umesh.dll"    n:\Deltabox\Bulletin\mooiman\programs\QGIS_5.12_qgis_umesh\*.*
rem copy .\doc\qgis_umesh_um.pdf                            n:\Deltabox\Bulletin\mooiman\programs\QGIS_5.12_qgis_umesh\doc\*.* 
rem copy .\doc\qgis_umesh_installation.pdf                  n:\Deltabox\Bulletin\mooiman\programs\QGIS_5.12_qgis_umesh\doc\*.* 
rem copy .\icons\*.*                                        n:\Deltabox\Bulletin\mooiman\programs\QGIS_5.12_qgis_umesh\icons\*.* 

rem pause
