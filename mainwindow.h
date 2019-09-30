#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

    void on_chkPackVoltageAxisVisible_stateChanged(int state);
    void on_chkCurrentAxisVisible_stateChanged(int state);
    void on_chkChargeAxisVisible_stateChanged(int state);
    void on_chkTemperatureAxisVisible_stateChanged(int state);

private:
    Ui::MainWindow *ui;
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
