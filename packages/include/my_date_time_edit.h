#include <QtWidgets/QApplication>
#include <QtWidgets/QDateTimeEdit>

class MyQDateTimeEdit : public QDateTimeEdit
{
    Q_OBJECT

public:
    MyQDateTimeEdit(QVector<QDateTime>, int);
    void setAnsatz(int, int);

public slots:
    void stepBy(int steps);

signals:
    void set_step(int);

public:
    QVector<QDateTime> _qdt;
    QDateTime _ansatz_date_time;
    int _ansatz_d;
    int _ansatz_u;
    int _nsteps;
    bool _ansatz;
};
