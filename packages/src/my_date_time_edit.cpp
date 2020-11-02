#include "../include/my_date_time_edit.h"

MyQDateTimeEdit::MyQDateTimeEdit(QVector<QDateTime> q_times)
{
    _qdt = q_times;
    _nsteps = 0;
    _ansatz = false;
    _ansatz_d = 0;
    _ansatz_u = 0;
}
void MyQDateTimeEdit::stepBy(int step)
{
    if (_ansatz)
    {
        if (_ansatz_d == _ansatz_u)
        {
            _nsteps = _ansatz_u;
            if (step == 1) _nsteps = _ansatz_d;
            _ansatz = false;
        }
        else
        {
            _nsteps = _ansatz_d;
            if (step == -1) _nsteps = _ansatz_u;
            _ansatz = false;
        }
    }
    switch (step) {
    case 1:
        if (maximumDateTime() > _qdt[_nsteps])
        {
            _nsteps = _nsteps + step;
            setDateTime(_qdt[_nsteps]);
        }
        break;
    case -1:
        if (minimumDateTime() < _qdt[_nsteps])
        {
            _nsteps = _nsteps + step;
            setDateTime(_qdt[_nsteps]);
        }
        break;
    default:    
        break;
        //emit set_step(_nsteps);
    }
}
void MyQDateTimeEdit::setAnsatz(int step_d, int step_u)
{
    _ansatz_d = step_d;
    _ansatz_u = step_u;
    _ansatz = true;
}
