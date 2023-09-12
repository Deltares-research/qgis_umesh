#include "cfts_history_api.h"

extern "C" {
    //namespace cfts_history_api
    //
    CFTSHISTORY_API HISCF* get_hiscf_obj_api(char* fname) {
        QString filename(_strdup(fname));
        return new HISCF(filename, nullptr);
    }
    CFTSHISTORY_API long read_nc_file_api(HISCF* cfts, char * fname)
    {
        long ret_value = -1;
        QString filename(_strdup(fname));
        ret_value = cfts->read();
        return ret_value;
    }
    CFTSHISTORY_API int get_observation_type_size_api(HISCF* cfts)
    {
        std::vector<_location_type*> obs_type = cfts->get_observation_location();
        return int(obs_type.size());
    }
    CFTSHISTORY_API char *  get_observation_type_api(HISCF* cfts, int i, int * type)
    {
        std::vector<_location_type*> obs_type = cfts->get_observation_location();
        *type = int(obs_type[i]->type);
        return obs_type[i]->location_long_name;
    }
    CFTSHISTORY_API int get_observation_location_name_size_api(HISCF* cfts, int i)
    {
        std::vector<_location_type*> obs_type = cfts->get_observation_location();
        return int(obs_type[i]->location.size());
    }
    CFTSHISTORY_API char * get_observation_location_name_api(HISCF* cfts, int i, int j)
    {
        std::vector<_location_type*> obs_type = cfts->get_observation_location();
        QByteArray ba;
        ba = obs_type[i]->location[j].name.toLatin1();
        return ba.data();
    }
    CFTSHISTORY_API int get_observation_geometry_dim_api(HISCF* cfts, int i, int j, int * n_vertex)
    {
        std::vector<_location_type*> obs_type = cfts->get_observation_location();
        * n_vertex = int(obs_type[i]->location[j].x.size());
        return 0;
    }
    CFTSHISTORY_API int get_observation_geometry_api(HISCF* cfts, int i, int j, double* x_vertex, double* y_vertex)
    {
        // get geometry of the observarion location (points or polylines)
        std::vector<_location_type*> obs_type = cfts->get_observation_location();
        std::vector<double> x = obs_type[i]->location[j].x;
        std::vector<double> y = obs_type[i]->location[j].y;
        std::memcpy(x_vertex, x.data(), x.size() * sizeof x[0]);
        std::memcpy(y_vertex, y.data(), y.size() * sizeof y[0]);
        return 0;
    }

    CFTSHISTORY_API void get_observation_print_api(HISCF* cfts)
    {
        std::vector<_location_type*> obs_type = cfts->get_observation_location();

        QString msg = QString("C++ function: \"%1\"; cfts->get_observation_location()").arg(__FUNCTION__);
        QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);

        msg = QString("Obs_point: \"%1\"  (%2,%3)").arg(obs_type[0]->location[0].name).arg(obs_type[0]->location[0].x[0]).arg(obs_type[0]->location[0].y[0]);
        QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);

        msg = QString("Obs_point: \"%1\"  (%2,%3)").arg(obs_type[0]->location[1].name).arg(obs_type[0]->location[1].x[0]).arg(obs_type[0]->location[1].y[0]);
        QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);

        for (int i = 0; i < obs_type.size(); ++i)
        {
            msg = QString("Number of: %1 ").arg(obs_type[i]->location.size());
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
        }
        std::vector<std::string> type_name;
        for (int i = 0; i < obs_type.size(); ++i)
        {
            type_name.push_back(obs_type[i]->location_long_name);
            msg = QString("Names: \"%1\" ").arg(QString::fromStdString(type_name[i]));
            QgsMessageLog::logMessage(msg, "QGIS umesh", Qgis::Info, true);
        }

    }
    CFTSHISTORY_API long read_epsg_api(HISCF* cfts)
    {
        long epsg = -1;
        epsg = cfts->get_epsg();
        return epsg;
    }
    //}
}
