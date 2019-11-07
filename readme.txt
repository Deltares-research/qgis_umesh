
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
