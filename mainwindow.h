#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "cellmonitordialog.h"

#include <QMainWindow>
#include <QSerialPort>
#include <QByteArray>
#include <QTimer>
#include <QMessageBox>
#include <QVector>
#include <QPointF>
#include <QFile>
#include <QDateTime>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
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
    void updatePlot(qreal voltage, qreal current, qreal charge, qreal temperature);

private slots:
    void on_serialPortReadyRead();
    void on_sleepTimerTimeout();
    void on_waitingMessageBoxButtonClicked(QAbstractButton *button);
    void on_chartUpdateTimer_timeout();
    void on_actClearData_triggered();
    void on_actSaveData_triggered();
    void on_actCellBalancing_triggered();
    void on_actShowHidePackVoltage_triggered(bool checked);
    void on_actShowHideCurrent_triggered(bool checked);
    void on_actShowHideChargeLevel_triggered(bool checked);
    void on_actShowHideTemperature_triggered(bool checked);
    void on_actExit_triggered();
    void on_actStartLogging_triggered();

private:
    Ui::MainWindow *ui;
    CellMonitorDialog *m_cellBalanceStatusForm = nullptr;
    QSerialPort *m_serialPort = nullptr;
    QTimer *m_sleepTimer = nullptr;
    QTimer *m_chartUpdateTimer = nullptr;
    QChart *m_chart = nullptr;
    QFile *m_dataLogFile = nullptr;
    QValueAxis *m_chartAxisTemperature;
    QValueAxis *m_chartAxisCurrent;
    QValueAxis *m_chartAxisCharge;
    QValueAxis *m_chartAxisPackVoltage;
    QDateTimeAxis *m_chartAxisTime;
    QLineSeries *m_chartSeriesPackVoltage;
    QLineSeries *m_chartSeriesCurrent;
    QLineSeries *m_chartSeriesCharge;
    QLineSeries *m_chartSeriesTemperature;
    QMessageBox *m_waitingMessageBox = nullptr;
    QByteArray m_dataBuffer;
    QVector<QPointF> m_chartDataPackVoltage;
    bool m_inHeader = false;
    bool m_inMessage = false;
    QDateTime m_startDateTime;
};

#endif // MAINWINDOW_H
