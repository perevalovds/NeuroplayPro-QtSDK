#include "chart.h"

Chart::Chart(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(300, 100);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
}

void Chart::setData(const NeuroplayDevice::ChannelsData &data, double limit)
{
    m_limit = limit;
    m_series.clear();
    for (QVector<double> chdata: data)
    {
        QVector<QPointF> line;
        for (int i=0; i<chdata.size(); i++)
            line << QPointF(i, chdata[i] * double(height()) / data.size() / m_limit);
        m_series << line;
    }
    update();
}

void Chart::clear()
{
    m_series.clear();
    update();
}

void Chart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    int h = height();
    int w = width();
    p.fillRect(rect(), Qt::white);
    if (!m_series.isEmpty())
    {
        int channels = m_series.size();//? 1: 0;
        int h1 = channels? h / channels: 0;
        for (int i=0; i<channels; i++)
        {
            int count = m_series[i].size();
            int y0 = lrintf((i + 0.5f) * h1);

            p.resetTransform();
            p.translate(0, y0);
            p.scale(double(w) / count, 1);//double(h1) / m_limit);
            //            p.scale(double(w) / count, h1 / m_limit);
            p.drawPolyline(m_series[i]);
        }
    }
    p.end();
}
