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

using namespace std;

struct _observation_strings {
    string var_name;
    string long_name;
    string x_node_name;  // 2D
    string y_node_name;  // 2D
};
struct _observation {
    size_t count;
    double x;
    double y;
    string name;
    string long_name;
};

struct _location {
    QString name;  // needed QString due to using the QTextCode
    vector<double> x;
    vector<double> y;
};

struct _location_type {
    enum OBSERVATION_TYPE type;
    char * location_var_name;  // to check the coordinate attribute
    char * location_long_name;  // to present in combobox i.e. observation point or cross-section
    char * location_dim_name;
    long ndim;
    vector<int> node_count;
    vector<_location> location;
    string x_location_name;
    string y_location_name;
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
    vector<_location_type *> get_observation_location();

    struct _mapping * get_grid_mapping();

    long set_grid_mapping_epsg(long, string);
    vector<string> get_names(int, string, size_t);

    std::vector<std::string> tokenize(const std::string & , const char );
    std::vector<std::string> tokenize(const std::string & , std::size_t );


#if defined (DEBUG)
    char  janm;
#endif

private:
    int get_attribute(int, int, char *, char **);
    int get_attribute(int, int, char *, string *);
    int get_attribute(int, int, string, string *);
    int get_attribute(int, int, char *, double *);
    int get_attribute(int, int, char *, int *);
    int get_attribute(int, int, char *, long *);
    int get_dimension(int, char *, size_t *);
    int get_dimension(int, string, size_t *);
    int get_dimension_var(int, string, size_t *);
    vector<string>  get_dimension_names(int, string);

    int read_grid_mapping(int, string, string);

    long _nr_mesh_contacts;
    size_t _two;
    size_t * m_dimids;
    vector<string> m_dim_names;
    int m_message_count;

    int ncid;

#ifdef NATIVE_C
    char * m_fname;
    char * m_hiscf_file_name;
#else
    QFileInfo m_fname;
    QFileInfo m_hiscf_file_name;
    QProgressBar * _pgBar;
#endif
    vector<_location_type *> loc_type;  // observation point, observation cross_section
    _global_attributes * global_attributes;
    struct _mapping * m_mapping = NULL;
    size_t name_len;
    vector<string> obs_names;
};

#endif  // HISCF_H
