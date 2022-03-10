#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include "neuroplaypro.h"
#include "chart.h"

namespace Ui {
class MainWindow;
}

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

    Chart *chart;

    QTreeWidget *tree;

    QLabel *status;
};

#endif // MAINWINDOW_H
