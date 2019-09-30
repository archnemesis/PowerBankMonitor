#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "cellmonitordialog.h"

#include <QMainWindow>
#include <QSerialPort>
#include <QByteArray>
#include <QTimer>
#include <QMessageBox>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>

QT_CHARTS_USE_NAMESPACE

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);
    void updatePlot(QLineSeries *series, double value);

private slots:
    void on_serialPortReadyRead();
    void on_sleepTimerTimeout();
    void on_waitingMessageBoxButtonClicked(QAbstractButton *button);
    void on_actClearData_triggered();
    void on_actSaveData_triggered();
    void on_actCellBalancing_triggered();
    void on_actShowHidePackVoltage_triggered(bool checked);
    void on_actShowHideCurrent_triggered(bool checked);
    void on_actShowHideChargeLevel_triggered(bool checked);
    void on_actShowHideTemperature_triggered(bool checked);

private:
    Ui::MainWindow *ui;
    CellMonitorDialog *m_cellBalanceStatusForm = nullptr;
    QSerialPort *m_serialPort = nullptr;
    QTimer *m_sleepTimer = nullptr;
    QChart *m_chart = nullptr;
    QValueAxis *m_chartAxisTemperature;
    QValueAxis *m_chartAxisCurrent;
    QValueAxis *m_chartAxisCharge;
    QValueAxis *m_chartAxisPackVoltage;
    QValueAxis *m_chartAxisTime;
    QLineSeries *m_chartSeriesPackVoltage;
    QLineSeries *m_chartSeriesCurrent;
    QLineSeries *m_chartSeriesCharge;
    QLineSeries *m_chartSeriesTemperature;
    QMessageBox *m_waitingMessageBox = nullptr;
    QByteArray m_dataBuffer;
    bool m_inHeader = false;
    bool m_inMessage = false;
};

#endif // MAINWINDOW_H
