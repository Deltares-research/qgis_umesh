/****************************************************************************
**
** Copyright (c) 2012 Richard Steffen and/or its subsidiary(-ies).
** All rights reserved.
** Contact: rsteffen@messbild.de, rsteffen@uni-bonn.de
**
** QColorRampEditor is free to use unter the terms of the LGPL 2.1 License in
** Free and Commercial Products.
****************************************************************************/

#include "../include/QColorRampEditor.h"
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>

// -----------------------------------------------------------
// QColorRampEditor ------------------------------------------
// -----------------------------------------------------------
QColorRampEditor::QColorRampEditor(QWidget* parent, int orientation) : QWidget(parent),
    ori_(orientation),
    activeSlider_(-1), slideUpdate_(false),
    bspace_(5),
    visText_(false), textColor_(Qt::white), textAcc_(1)
{
    if (ori_==Qt::Horizontal)
        setMinimumSize(50,40);
    else
        setMinimumSize(40,50);

    setContentsMargins(0,0,0,0);
    if (ori_==Qt::Horizontal)
        setLayout(new QVBoxLayout());
    else
        setLayout(new QHBoxLayout());
    layout()->setMargin(0);
    layout()->setSpacing(0);
    layout()->setContentsMargins(0,0,0,0);

    rampwid_ = new QRampWidget();
    rampwid_->rampeditor_ = this;
    rampwid_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    rampwid_->setContentsMargins(0,0,0,0);

    layout()->addWidget(rampwid_);

    slidewid_ = new QSlidersWidget();
    slidewid_->rampeditor_ = this;
    if (ori_==Qt::Horizontal)
    {
        slidewid_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        slidewid_->setFixedHeight(16);
    }
    else
    {
        slidewid_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        slidewid_->setFixedWidth(16);
    }
    slidewid_->setContentsMargins(0,0,0,0);
    layout()->addWidget(slidewid_);

    textwid_ = new QSliderTextWidget();
    textwid_->rampeditor_ = this;
    if (ori_==Qt::Horizontal)
    {
        textwid_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        textwid_->setFixedHeight(12);
    }
    else
    {
        textwid_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        textwid_->setFixedWidth(12);
    }
    layout()->addWidget(textwid_);
    textwid_->setVisible(false);

    // init sliders
    QVector<QPair<qreal, QColor> > ini_ramp;
    ini_ramp.push_back(QPair<qreal, QColor>(0.0, Qt::black));
    ini_ramp.push_back(QPair<qreal, QColor>(1.0, Qt::red));
    setNormRamp(ini_ramp);
    setRamp(ini_ramp);

    chooser_ = new QColorDialog();
    chooser_->setOption(QColorDialog::ShowAlphaChannel, true);
}
// -----------------------------------------------------------
QColorRampEditor::~QColorRampEditor()
{
    for (int i=0; i<sliders_.size(); i++) delete(sliders_[i]);
    if (chooser_) delete(chooser_);
}
// -----------------------------------------------------------
int QColorRampEditor::getSliderNum()
{
    return sliders_.size();
}
// -----------------------------------------------------------
void QColorRampEditor::setColorChoose(QColorDialog* coldlg)
{
    chooser_ = coldlg;
}

// -----------------------------------------------------------
void QColorRampEditor::setSlideUpdate(bool val)
{
    slideUpdate_ = val;
}
// -----------------------------------------------------------
bool QColorRampEditor::colorRampSort(const QPair<qreal, QColor> &a1, const QPair<qreal, QColor> &a2)
{
    return a1.first < a2.first;
}

bool QColorRampEditor::SliderSort(const QColorRampEditorSlider* a1, const QColorRampEditorSlider* a2)
{
    return a1->val < a2->val;
}

// -----------------------------------------------------------
QVector<QPair<qreal, QColor> > QColorRampEditor::getRamp()
{

    // create output
    QVector<QPair<qreal, QColor> > ret;
    for (int i=0; i<sliders_.size(); i++)
    {
        QColor col = sliders_[i]->getColor();
        ret.push_back(QPair<qreal, QColor>(sliders_[i]->val,col));
    }
    // sort the slider list
    qSort(ret.begin(), ret.end(), QColorRampEditor::colorRampSort);

    return ret;
}
// -----------------------------------------------------------
QVector<QRgb> QColorRampEditor::getColorTable()
{
    // get ramp and normalize
    ramp = getRamp();
    for (int i=0; i<ramp.size(); i++) ramp[i].first = (ramp[i].first - mi_)/(ma_-mi_);

    QVector<QRgb> ctable;
    int index = 0;
    for (int i = 0; i < 256; i++)
    {
        double val = 1.0f*i/255;
        double a = ramp[index + 1].first;
        if (val>a) index++;
        // linear interpolate color
        QColor cc;
        float d = val - ramp[index].first;
        float dr = ramp[index+1].second.redF() - ramp[index].second.redF();
        float dg = ramp[index+1].second.greenF() - ramp[index].second.greenF();
        float db = ramp[index+1].second.blueF() - ramp[index].second.blueF();
        qreal dd = ramp[index+1].first - ramp[index].first;
        float red = ramp[index].second.redF() +  d*dr/dd;
        float green = ramp[index].second.greenF() +  d*dg/dd;
        float blue = ramp[index].second.blueF() +  d*db/dd;
        if (red<0.0f) red=0;
        if (red>1.0f) red=1;
        if (green<0.0f) green=0;
        if (green>1.0f) green=1;
        if (blue<0.0f) blue=0;
        if (blue>1.0f) blue=1;
        cc.setRedF(red);
        cc.setGreenF(green);
        cc.setBlueF(blue);
        ctable.push_back(cc.rgb());
    }
    return ctable;
}
// -----------------------------------------------------------
void QColorRampEditor::setNormRamp(QVector<QPair<qreal, QColor> > ramp_in)
{
    // Set the ramp normalized values, i.e. [0.0, 1.0]
    norm_ramp = ramp_in;
}
// -----------------------------------------------------------
void QColorRampEditor::setRamp(QVector<QPair<qreal, QColor> > ramp_in)
{
    ramp = ramp_in;
    if (ramp.size()<2) return;

    // sort the slider list
    qSort(ramp.begin(), ramp.end(), QColorRampEditor::colorRampSort);
    for (int i=0; i<sliders_.size(); i++) delete(sliders_[i]);
    sliders_.clear();

    // find min/max
    mi_= ramp.first().first;
    ma_= ramp.last().first;

    // create sliders
    for (int i=0; i<ramp.size(); i++)
    {
        QColorRampEditorSlider* sl = new QColorRampEditorSlider(ori_, ramp[i].second, slidewid_);
        sl->val = ramp[i].first;
        sliders_.push_back(sl);
        updatePos(sl);
        sl->show();
    }
    // emit rampChanged();
    slidewid_->update();
}

void QColorRampEditor::setMinMax(double z_min, double z_max)
{
    ramp = norm_ramp;

    double m_z_min = z_min;
    double m_z_max = z_max;
    if (m_z_max - m_z_min < 0.01)
    {
        if (m_z_min == 0.0)
        {
            m_z_min = -0.01;
            m_z_max =  0.01;
        }
        else
        {
            int delta = log10(m_z_min) - 2;
            m_z_min = m_z_min - std::pow(10., delta);
            m_z_max = m_z_max + std::pow(10., delta);
        }
    }

    for (int i = 0; i<ramp.size(); i++)
    {
        double value = m_z_min + ramp[i].first * (m_z_max - m_z_min);
        ramp[i].first = value; // (QPair<qreal, QColor>(value, color));
    }
    setRamp(ramp);  // this ramp contains the real values
}
QColor QColorRampEditor::getRgbFromValue(double z_value)
{
    bool within_ramp_range = true;
    ramp = getRamp();
    QColor color = qRgba(128, 128, 128, 0);  // default grey
    int i = 0;
    if (z_value <= ramp[i].first)  // z_value before first color in ramp
    {
        int r = ramp[i].second.red();
        int g = ramp[i].second.green();
        int b = ramp[i].second.blue();
        int a = ramp[i].second.alpha();
        color = qRgb(r, g, b);
        color.setAlpha(a);
        within_ramp_range = false;
    }
    i = ramp.size() - 1;  // zero based
    if (z_value >= ramp[i].first)  // z_value after last color in ramp
    {
        int r = ramp[i].second.red();
        int g = ramp[i].second.green();
        int b = ramp[i].second.blue();
        int a = ramp[i].second.alpha();
        color = qRgb(r, g, b);
        color.setAlpha(a);
        within_ramp_range = false;
    }
    if (within_ramp_range)
    {
        for (i = 1; i < ramp.size(); i++)
        {
            if (ramp[i].first >= z_value)
            {
                double alpha = (z_value - ramp[i - 1].first) / (ramp[i].first - ramp[i - 1].first);
                int r = ramp[i - 1].second.red() + alpha * (ramp[i].second.red() - ramp[i - 1].second.red());
                int g = ramp[i - 1].second.green() + alpha * (ramp[i].second.green() - ramp[i - 1].second.green());
                int b = ramp[i - 1].second.blue() + alpha * (ramp[i].second.blue() - ramp[i - 1].second.blue());
                int a = ramp[i - 1].second.alpha() + alpha * (ramp[i].second.alpha() - ramp[i - 1].second.alpha());
                color = qRgb(r, g, b);
                color.setAlpha(a);
                break;
            }
        }
    }
    return color;
}

void QColorRampEditor::setMappingTextVisualize(bool vis)
{
    visText_ = vis;
    textwid_->setVisible(visText_);
    update();
}

void QColorRampEditor::setMappingTextColor(QColor col)
{
    textColor_ = col;
    update();
}

void QColorRampEditor::setMappingTextAccuracy(int acc)
{
    textAcc_ = acc;
    update();
}

qreal QColorRampEditor::updateValue(QColorRampEditorSlider* sl)
{
    QRect crec = slidewid_->contentsRect();
    if (ori_ == Qt::Horizontal)
    {
        crec.adjust(bspace_, 0, -bspace_, 0);
        sl->val = mi_ + (1.0*sl->pos().x() - bspace_) / crec.width()*(ma_ - mi_);
    }
    else
    {
        crec.adjust(0, bspace_, 0, -bspace_);
        sl->val = mi_ + (1.0*sl->pos().y() - bspace_) / crec.height()*(ma_ - mi_);
    }
    return sl->val;
}
qreal QColorRampEditor::updateValue(QColorRampEditorSlider* sl, int slider_pointer)
{
    QRect crec = slidewid_->contentsRect();
    if (ori_==Qt::Horizontal)
    {
        crec.adjust(bspace_,0,-bspace_,0);
        sl->val = mi_ + (1.0*sl->pos().x() - bspace_)/crec.width()*(ma_-mi_);

        qreal norm_val = (1.0*sl->pos().x() - bspace_) / crec.width();
        norm_ramp[slider_pointer].first = norm_val; // (QPair<qreal, QColor>(value, color));
    }
    else
    {
        crec.adjust(0,bspace_,0,-bspace_);
        sl->val = mi_ + (1.0*sl->pos().y()-bspace_)/crec.height()*(ma_-mi_);
    }
    return sl->val;
}
void QColorRampEditor::removeSliderPointer(int slider_pointer)
{
    norm_ramp.remove(slider_pointer);
}
int QColorRampEditor::updatePos(QColorRampEditorSlider* sl)
{
    QRect crec = slidewid_->contentsRect();
    qreal pos;
    if (ori_==Qt::Horizontal)
    {
        crec.adjust(bspace_,0,-bspace_,0);
        pos = (sl->val- mi_)/(ma_-mi_)*crec.width();
        pos += bspace_ - sl->width()/2.;
        sl->move(pos,0);
    }
    else
    {
        crec.adjust(0, bspace_,0,-bspace_);
        pos = (sl->val- mi_)/(ma_-mi_)*crec.height();
        pos += bspace_ - sl->width()/2.;
        sl->move(0,pos);
    }
    return pos;
}

// -----------------------------------------------------------
void QColorRampEditor::setSliderColor(int index, QColor col)
{
    if (index<0 || index>=sliders_.size()) return;
    sliders_[index]->setColor(col);
    emit rampChanged();
}

// -----------------------------------------------------------
void QColorRampEditor::resizeEvent (QResizeEvent* e)
{
    for (int i=0; i<sliders_.size(); i++)
    {
        QColorRampEditorSlider* sl = sliders_[i];
        updatePos(sl);
    }
}
// -----------------------------------------------------------
void QColorRampEditor::mousePressEvent(QMouseEvent* e)
{
    if (e->button()== Qt::LeftButton)
    {
        QRect crec = contentsRect();
        if (ori_==Qt::Horizontal)
        {
            crec.adjust(bspace_,0,-bspace_,0);
            if (crec.contains(e->pos(), true )) // test mouse is in ramp
            {
                QColorRampEditorSlider* sl = new QColorRampEditorSlider(ori_, Qt::white, slidewid_);
                sliders_.push_back(sl);
                sl->move(e->pos().x(),0);
                updateValue(sl);
                QPair<qreal, QColor> a;
                a.first = 1.0*(e->pos().x() - bspace_) / crec.width();
                a.second = Qt::white;
                for (int i = 1; i < norm_ramp.size(); i++)
                {
                    if (a.first < norm_ramp[i].first) 
                    {
                        norm_ramp.insert(i, a);
                        break;
                    }
                }
                sl->show();
                qSort(sliders_.begin(), sliders_.end(), QColorRampEditor::SliderSort);
                updateRamp();
            }
        }
        else
        {
            crec.adjust(0,bspace_,0,-bspace_);
            if (crec.contains(e->pos(), true )) // test mouse is in ramp
            {
                QColorRampEditorSlider* sl = new QColorRampEditorSlider(ori_, Qt::white, slidewid_);
                sliders_.push_back(sl);
                sl->move(0,e->pos().y());
                updateValue(sl);
                sl->show();
                qSort(sliders_.begin(), sliders_.end(), QColorRampEditor::SliderSort);
                updateRamp();
            }
        }
    }
}

void QColorRampEditor::updateRamp()
{
    rampwid_->update();
    if (textwid_->isVisible()) textwid_->update();
    //emit rampChanged();
}
// -----------------------------------------------------------
void QColorRampEditor::msg()
{
    //QMessageBox::information(0, "Message", "QColorRampEditor::msg()");
}


// -----------------------------------------------------------
// QRampWidget -----------------------------------------------
// -----------------------------------------------------------

QRampWidget::QRampWidget(QWidget* parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    setContentsMargins(0,5,0,5);
    this->setMinimumHeight(5);
    this->setMinimumWidth(100);
}

void QRampWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);
    painter.setPen(Qt::black);

    QLinearGradient grad;
    QRect crec = contentsRect();
    if (rampeditor_->ori_==Qt::Horizontal)
    {
        crec.adjust(rampeditor_->bspace_, 0, -rampeditor_->bspace_-1, -1);
        grad = QLinearGradient((qreal)crec.left(), 0, (qreal)crec.right(), 0);
    }
    else
    {
        crec.adjust(0, rampeditor_->bspace_, -1, -rampeditor_->bspace_-1);
        grad = QLinearGradient( 0., (qreal)crec.bottom(), 0., (qreal)crec.top());
    }

    for (int i=0; i<rampeditor_->sliders_.size(); i++)
    {
        qreal nval = (rampeditor_->sliders_[i]->val - rampeditor_->mi_)/(rampeditor_->ma_-rampeditor_->mi_);
        grad.setColorAt(nval, rampeditor_->sliders_[i]->getColor());
    }

    painter.fillRect(crec, grad);
    painter.drawRect(crec);

    QWidget::paintEvent(e);
}

// -----------------------------------------------------------
// QSlidersWidget --------------------------------------------
// -----------------------------------------------------------
QSlidersWidget::QSlidersWidget(QWidget* parent) : QWidget(parent),
    activeSlider_(-1)
{
    setContentsMargins(0,0,0,0);
}

// -----------------------------------------------------------
void QSlidersWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button()== Qt::LeftButton)
    {
        if (rampeditor_->sliders_.size()<2) return;
        for (int i=1; i<rampeditor_->sliders_.size()-1; i++)
        {
            QRect srec = rampeditor_->sliders_[i]->geometry();
            if (srec.contains(e->pos(), true ))
            {
                activeSlider_ = i;
                break;
            }
        }
    }
}
// -----------------------------------------------------------
void QSlidersWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (activeSlider_>=0)
    {
        QRect crec = contentsRect();

        qreal pos;
        if (rampeditor_->ori_==Qt::Horizontal)
        {
            crec.adjust(rampeditor_->bspace_,0,-rampeditor_->bspace_,0);
            pos = 1.0*(e->pos().x()-rampeditor_->bspace_)/(crec.width());
        }
        else
        {
            crec.adjust(0,rampeditor_->bspace_,0,-rampeditor_->bspace_);
            pos = 1.0*(e->pos().y()-rampeditor_->bspace_)/(crec.height());
        }

        if (pos<0.0 || pos>1.0)
        {
            delete(rampeditor_->sliders_[activeSlider_]);
            rampeditor_->sliders_.removeAt(activeSlider_);
            rampeditor_->removeSliderPointer(activeSlider_);
            activeSlider_ = -1;
            rampeditor_->updateRamp();
            emit rampeditor_->rampChanged();
        }
        else
        {
            if (rampeditor_->ori_==Qt::Horizontal)
                rampeditor_->sliders_[activeSlider_]->move(e->pos().x(), 0);
            else
                rampeditor_->sliders_[activeSlider_]->move(0,e->pos().y());

            rampeditor_->updateValue(rampeditor_->sliders_[activeSlider_], activeSlider_);
            qSort(rampeditor_->sliders_.begin(), rampeditor_->sliders_.end(), QColorRampEditor::SliderSort);
            if (rampeditor_->slideUpdate_) rampeditor_->updateRamp();
        }
        update();
    }
}
// -----------------------------------------------------------
void QSlidersWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton)
    {
        //int pos = e->pos().x();
        activeSlider_ = -1;
        rampeditor_->updateRamp();
        emit rampeditor_->rampChanged();
    }
}
// -----------------------------------------------------------
void QSlidersWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (e->button()== Qt::LeftButton)
    {
        int index = -1;
        for (int i=0; i<rampeditor_->sliders_.size(); i++)
        {
            QRect srec = rampeditor_->sliders_[i]->geometry();
            if (srec.contains(e->pos(), true ))
            {
                index = i;
                break;
            }
        }
        if (index>=0)
        {
            if (rampeditor_->chooser_)
            {
                rampeditor_->chooser_->setCurrentColor(rampeditor_->sliders_[index]->getColor());
                rampeditor_->chooser_->exec();
                if (rampeditor_->chooser_->result() == QDialog::Accepted)
                {
                    rampeditor_->sliders_[index]->setColor(rampeditor_->chooser_->selectedColor());
                    rampeditor_->norm_ramp[index].second = rampeditor_->chooser_->selectedColor();
                    rampeditor_->updateRamp();
                    rampeditor_->rampChanged();
                }
            }
        }
    }
}


// -----------------------------------------------------------
// QColorRampEditorSlider ------------------------------------
// -----------------------------------------------------------
QColorRampEditorSlider::QColorRampEditorSlider(int orientation, QColor col, QWidget* parent) : QWidget(parent),
    ori_(orientation), color_(col)
{
    this->val = 0.0;
    if (ori_==Qt::Horizontal)
        setFixedSize(9, 16);
    else
        setFixedSize(16,9);
}
// -----------------------------------------------------------
void QColorRampEditorSlider::setColor(QColor col)
{
    color_ = col;
}
// -----------------------------------------------------------
QColor QColorRampEditorSlider::getColor()
{
    return color_;
}

// -----------------------------------------------------------
void QColorRampEditorSlider::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);
    painter.setPen(Qt::black);
    painter.setBrush(color_);
    if (ori_==Qt::Horizontal)
    {
        QRect rec(0,7,8,8);
        painter.drawRect(rec);
        QPolygon pp;
        pp << QPoint(0,7) << QPoint(4,0) << QPoint(9,7);
        painter.drawPolygon(pp, Qt::OddEvenFill);
    }
    else
    {
        QRect rec(7,0,8,8);
        painter.drawRect(rec);
        QPolygon pp;
        pp << QPoint(7,0) << QPoint(0,4) << QPoint(7,9);
        painter.drawPolygon(pp, Qt::OddEvenFill);
    }
}


// -----------------------------------------------------------
// QSliderTextWidget -----------------------------------------
// -----------------------------------------------------------
QSliderTextWidget::QSliderTextWidget(QWidget* parent) : QWidget(parent)
{
    setContentsMargins(0,0,0,0);
}

void QSliderTextWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);

    QFont f = painter.font();
    f.setPixelSize(8);
    painter.setFont(f);

    painter.setPen(rampeditor_->textColor_);
    painter.setBrush(rampeditor_->textColor_);

    QFontMetrics fm(f);

    // adjust size for vertical
    if (rampeditor_->ori_==Qt::Vertical)
    {
        {
            QString txt1 = QString::number(rampeditor_->sliders_.first()->val, 'f', rampeditor_->textAcc_);
            QString txt2 = QString::number(rampeditor_->sliders_.last()->val, 'f', rampeditor_->textAcc_);
            int w = fm.width(txt1) + 4;
            if (w>this->width()) setFixedWidth(w);
            w = fm.width(txt2) + 4;
            if (w>this->width()) setFixedWidth(w);
        }
        // draw the text for vertical orientation
        for (int i=0; i<rampeditor_->sliders_.size(); i++)
        {
            int pos = rampeditor_->sliders_[i]->pos().y();
            qreal val = rampeditor_->sliders_[i]->val;
            QString txt = QString::number(val, 'f', rampeditor_->textAcc_);
            painter.drawText(2, pos + rampeditor_->sliders_[i]->height(), txt);
        }
    }
    else // draw the text for horizontal orientation
    {
        for (int i=0; i<rampeditor_->sliders_.size(); i++)
        {
            int pos = rampeditor_->sliders_[i]->pos().x();
            qreal val = rampeditor_->sliders_[i]->val;
            QString txt = QString::number(val, 'f', rampeditor_->textAcc_);
            if ((pos+fm.width(txt))>width()) pos += -fm.width(txt) + rampeditor_->sliders_[i]->width();
            painter.drawText(pos, 2+fm.height(), txt);
        }
    }

    QWidget::paintEvent(e);
}

