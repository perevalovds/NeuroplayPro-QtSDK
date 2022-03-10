#ifndef CHART_H
#define CHART_H

#include <QtWidgets>
#include "neuroplaypro.h"


class Chart: public QWidget
{
    Q_OBJECT
public:
    Chart(QWidget *parent = nullptr);
    void setData(const NeuroplayDevice::ChannelsData &data);
    void setLimit(double limit);
protected:
    void paintEvent(QPaintEvent *) override;
private:
    QVector< QVector<QPointF> > m_series;
    double m_limit = 1000000;
};


#endif // CHART_H
