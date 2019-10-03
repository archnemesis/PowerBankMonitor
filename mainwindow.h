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
#include <QLabel>
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
    void showEvent(QShowEvent *event);
    void updatePlot(qreal voltage, qreal current, qreal charge, qreal temperature);
    qreal convertTemperature(qreal temperature_c);
    qreal convertCharge(qreal current_c);
    QString chargeSuffix();
    QString temperatureSuffix();

private slots:
    void on_serialPortReadyRead();
    void on_sleepTimerTimeout();
    void on_waitingMessageBoxButtonClicked(QAbstractButton *button);
    void on_chartUpdateTimer_timeout();
    void on_actClearData_triggered();
    void on_actSaveData_triggered();
    void on_actCellBalancing_triggered();
    void on_actShowHideCurrent_triggered(bool checked);
    void on_actShowHideChargeLevel_triggered(bool checked);
    void on_actShowHideTemperature_triggered(bool checked);
    void on_actExit_triggered();
    void on_actStartLogging_triggered();
    void on_actStopLogging_triggered();
    void on_actPackVoltageShow_triggered(bool checked);
    void on_actCurrentShow_triggered(bool checked);
    void on_actChargeShow_triggered(bool checked);
    void on_actTemperatureShow_triggered(bool checked);
    void on_actPackVoltageColor_triggered();
    void on_actCurrentColor_triggered();
    void on_actChargeColor_triggered();
    void on_actTemperatureColor_triggered();
    void on_actPackVoltageOrientationLeft_triggered(bool checked);
    void on_actPackVoltageOrientationRight_triggered(bool checked);
    void on_actCurrentOrientationLeft_triggered(bool checked);
    void on_actCurrentOrientationRight_triggered(bool checked);
    void on_actChargeOrientationLeft_triggered(bool checked);
    void on_actChargeOrientationRight_triggered(bool checked);
    void on_actTemperatureOrientationLeft_triggered(bool checked);
    void on_actTemperatureOrientationRight_triggered(bool checked);

    void on_actViewSettings_triggered();

    void on_actAbout_triggered();

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
    QLabel *m_packStatusLabel = nullptr;
    QLabel *m_dataLogLabel = nullptr;
    QLabel *m_serialPortLabel = nullptr;
    QByteArray m_dataBuffer;
    QVector<QPointF> m_chartDataPackVoltage;
    bool m_inHeader = false;
    bool m_inMessage = false;
    QDateTime m_startDateTime;
    QString m_chargeUnit;
    QString m_temperatureUnit;
};

#endif // MAINWINDOW_H
