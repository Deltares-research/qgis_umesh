#ifndef HISCF_H
#define HISCF_H

#include <QFileInfo>
#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QDate>
#include <QtCore/QFileInfo>
#include <QtCore/QTime>
#include <QtCore/qtextcodec.h>
#include <QtCore/qdebug.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>

#include <QtGui/QIcon>

#include <qgsmessagelog.h>

#include "data_struct.h"

struct _observation_strings {
    std::string var_name;
    std::string long_name;
    std::string x_node_name;  // 2D
    std::string y_node_name;  // 2D
};
struct _observation {
    size_t count;
    double x;
    double y;
    std::string name;
    std::string long_name;
};

struct _location {
    QString name;  // needed QString due to using the QTextCode
    std::vector<double> x;
    std::vector<double> y;
};

struct _location_type {
    enum OBSERVATION_TYPE type = OBS_NONE;
    char * location_var_name;  // to check the coordinate attribute
    char * location_long_name;  // to present in combobox i.e. observation point or cross-section
    char * location_dim_name;
    long ndim;
    std::vector<int> node_count;
    std::vector<_location> location;
    std::string x_location_name;
    std::string y_location_name;
};

class HISCF
{
public:
#ifdef NATIVE_C
    HISCF(char *, int * dummy);
    char * get_filename();
#else
    HISCF(QFileInfo, QProgressBar *);
    QFileInfo get_filename();
#endif
    ~HISCF();
    long read();
    long read_global_attributes();
    long read_locations();
    long read_parameters();

    long read_times();
    long read_variables();

    struct _global_attributes * get_global_attributes(void);
    std::vector<_location_type *> get_observation_location();

    struct _mapping * get_grid_mapping();

    long set_grid_mapping_epsg(long, std::string);
    std::vector<std::string> get_names(int, std::string, size_t);

    std::vector<std::string> tokenize(const std::string & , const char );
    std::vector<std::string> tokenize(const std::string & , std::size_t );


#if defined (DEBUG)
    char  janm;
#endif

private:
    int get_attribute(int, int, char *, char **);
    int get_attribute(int, int, char *, std::string *);
    int get_attribute(int, int, std::string, std::string *);
    int get_attribute(int, int, char *, double *);
    int get_attribute(int, int, char *, int *);
    int get_attribute(int, int, char *, long *);
    int get_dimension(int, char *, size_t *);
    int get_dimension(int, std::string, size_t *);
    int get_dimension_var(int, std::string, size_t *);
    std::vector<std::string>  get_dimension_names(int, std::string);

    int read_grid_mapping(int, std::string, std::string);

    long _nr_mesh_contacts;
    std::size_t _two;
    std::size_t * m_dimids;
    std::vector<std::string> m_dim_names;
    int m_message_count;
    int m_ncid;

#ifdef NATIVE_C
    char * m_fname;
    char * m_hiscf_file_name;
#else
    QFileInfo m_fname;
    QString m_hiscf_file_name;
    QProgressBar * _pgBar;
#endif
    std::vector<_location_type *> loc_type;  // observation point, observation cross_section
    _global_attributes * global_attributes;
    struct _mapping * m_mapping = NULL;
    std::size_t name_len;
    std::vector<std::string> obs_names;
};

#endif  // HISCF_H
