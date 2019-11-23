
--------------------------------------------------------------------------------

Opstarten uit de taskbar

C:\OSGeo4W64\bin\qgis-bin-g7.exe --project d:\mooiman\home\models\delft3d\GIS\Netherlands\nl.qgs 

--------------------------------------------------------------------------------
            lMyAttribField << QgsField("TrackId", QVariant::Int)
                << QgsField("Label", QVariant::String)
                << QgsField("Type", QVariant::String)
                << QgsField("Color", QVariant::String)
                << QgsField("PosX", QVariant::Double)
                << QgsField("PosY", QVariant::Double)
                << QgsField("TimeStamp", QVariant::String);

MyFeature.setAttribute(QString("TimeStamp"), MyTime.currentDateTime().toString("dd.MM.yyyy hh:mm:ss.zzz"));

--------------------------------------------------------------------------------

git ls-remote --get-url // get the remote URL, ex. https://github.com/Deltares/qgis_umesh
git diff --name-status  // geeft een lijst van files die nog gecommit moeten worden, gecommitte files staan er niet in
git log @{u}.. // list there was a not pushed commit, but what file (is that necessary)
