#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include "neuroplaypro.h"

namespace Ui {
class MainWindow;
}

class ChartTest: public QWidget
{
    Q_OBJECT
public:
    ChartTest(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumSize(300, 100);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    }
    void setData(const NeuroplayDevice::ChannelsData &data)
    {
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
    void setLimit(double limit) {m_limit = limit; update();}
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        int h = height();
        int w = width();
        p.fillRect(rect(), Qt::white);
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
        p.end();
    }
private:
    QVector< QVector<QPointF> > m_series;
    double m_limit = 1000000;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    NeuroplayPro *neuroplay;

    QPushButton *btnOpen, *btnSend;
    QLineEdit *editCmd;
    QTextEdit *log;

    ChartTest *chart;

    QTreeWidget *tree;

    QLabel *status;
};

#endif // MAINWINDOW_H
