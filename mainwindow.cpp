#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "selectserialportdialog.h"
#include "statuspacket.h"

#include <math.h>
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QAbstractButton>
#include <QFile>
#include <QDataStream>
#include <QFileDialog>

#include <QtCharts/QChartView>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_waitingMessageBox = new QMessageBox(this);
    m_waitingMessageBox->setWindowTitle(tr("Waiting for Pack"));
    m_waitingMessageBox->setText(tr("Wake the pack by pressing the wake button, or by connecting a load or charger."));
    m_waitingMessageBox->setStandardButtons(QMessageBox::Cancel);
    m_waitingMessageBox->setModal(true);
    connect(m_waitingMessageBox, &QMessageBox::buttonClicked, this, &MainWindow::on_waitingMessageBoxButtonClicked);

    m_sleepTimer = new QTimer(this);
    m_sleepTimer->setInterval(1000);
    m_sleepTimer->setSingleShot(true);
    connect(m_sleepTimer, &QTimer::timeout, this, &MainWindow::on_sleepTimerTimeout);

    QSerialPortInfo selectedPort = SelectSerialPortDialog::getSerialPortInfo(this);

    if (selectedPort.isValid()) {
        m_serialPort = new QSerialPort(selectedPort);
        m_serialPort->setBaudRate(115200);
        m_serialPort->setParity(QSerialPort::NoParity);
        m_serialPort->setDataBits(QSerialPort::Data8);
        m_serialPort->setStopBits(QSerialPort::OneStop);
        m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

        connect(m_serialPort, &QSerialPort::readyRead, this, &MainWindow::on_serialPortReadyRead);

        if (!m_serialPort->open(QIODevice::ReadWrite)) {
            QMessageBox::information(
                        this,
                        tr("Port Error"),
                        tr("Unable to open serial port %1").arg(selectedPort.portName()),
                        QMessageBox::Retry | QMessageBox::Cancel);
        }
        else {
            m_waitingMessageBox->show();
        }
    }

    m_chart = new QChart();
    //m_chart->legend()->setVisible(false);

    m_chartAxisTime = new QValueAxis;
    m_chartAxisTime->setMin(0);
    m_chartAxisTime->setMax(100);
    m_chartAxisTime->setTitleText(tr("Seconds"));
    m_chart->addAxis(m_chartAxisTime, Qt::AlignBottom);

    m_chartAxisCharge = new QValueAxis;
    m_chartAxisCharge->setMin(0);
    m_chartAxisCharge->setMax(12000);
    m_chartAxisCharge->setTitleText(tr("Charge (C)"));
    m_chartAxisCharge->setTickInterval(1000);
    m_chart->addAxis(m_chartAxisCharge, Qt::AlignLeft);

    m_chartAxisCurrent = new QValueAxis;
    m_chartAxisCurrent->setMin(0);
    m_chartAxisCurrent->setMax(10);
    m_chartAxisCurrent->setTitleText(tr("Current (A)"));
    m_chartAxisCurrent->setTickInterval(0.1);
    m_chart->addAxis(m_chartAxisCurrent, Qt::AlignLeft);

    m_chartAxisPackVoltage = new QValueAxis;
    m_chartAxisPackVoltage->setMin(0);
    m_chartAxisPackVoltage->setMax(25.4);
    m_chartAxisPackVoltage->setTitleText(tr("Voltage (V)"));
    m_chartAxisPackVoltage->setTickInterval(1.0);
    m_chart->addAxis(m_chartAxisPackVoltage, Qt::AlignLeft);

    m_chartAxisTemperature = new QValueAxis;
    m_chartAxisTemperature->setMin(0);
    m_chartAxisTemperature->setMax(80);
    m_chartAxisTemperature->setTitleText(tr("Temperature (C)"));
    m_chartAxisTemperature->setTickInterval(5);
    m_chart->addAxis(m_chartAxisTemperature, Qt::AlignRight);

    m_chartSeriesPackVoltage = new QLineSeries;
    m_chart->addSeries(m_chartSeriesPackVoltage);
    m_chartSeriesPackVoltage->attachAxis(m_chartAxisPackVoltage);
    m_chartSeriesPackVoltage->attachAxis(m_chartAxisTime);

    m_chartSeriesCurrent = new QLineSeries;
    m_chart->addSeries(m_chartSeriesCurrent);
    m_chartSeriesCurrent->attachAxis(m_chartAxisCurrent);
    m_chartSeriesCurrent->attachAxis(m_chartAxisTime);

    m_chartSeriesCharge = new QLineSeries;
    m_chart->addSeries(m_chartSeriesCharge);
    m_chartSeriesCharge->attachAxis(m_chartAxisCharge);
    m_chartSeriesCharge->attachAxis(m_chartAxisTime);

    m_chartSeriesTemperature = new QLineSeries;
    m_chart->addSeries(m_chartSeriesTemperature);
    m_chartSeriesTemperature->attachAxis(m_chartAxisTemperature);
    m_chartSeriesTemperature->attachAxis(m_chartAxisTime);

    ui->chartView->setChart(m_chart);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_serialPort != nullptr) {
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
        }
        m_serialPort->deleteLater();
        m_serialPort = nullptr;
    }

    event->accept();
}

void MainWindow::on_serialPortReadyRead()
{
    QByteArray data = m_serialPort->readAll();

    qDebug() << "Got" << data.length() << "bytes";

    for (int i = 0; i < data.length(); i++) {
        if (data.at(i) == 'D' && m_inHeader == false && m_inMessage == false) {
            m_inHeader = true;
        }
        else if (data.at(i) == 'E' && m_inHeader == true) {
            m_inHeader = false;
            m_inMessage = true;
            m_dataBuffer.clear();
        }
        else if (m_inMessage) {
            m_dataBuffer.append(data.at(i));
            if (m_dataBuffer.length() == sizeof(status_packet_t)) {
                status_packet_t packet;
                memcpy((void *)&packet, m_dataBuffer.data(), sizeof(status_packet_t));

                if (packet.a == 'A' && packet.b == 'B') {
                    m_waitingMessageBox->hide();
                    m_sleepTimer->stop();
                    m_sleepTimer->start();

                    updatePlot(m_chartSeriesPackVoltage, static_cast<double>(packet.pack_voltage) / 1000.0);
                    updatePlot(m_chartSeriesCurrent, abs(static_cast<double>(packet.current) / 1000.0));
                    updatePlot(m_chartSeriesCharge, static_cast<double>(packet.charge_state));
                    updatePlot(m_chartSeriesTemperature, static_cast<double>(packet.temperature) / 1000.0);

                    ui->lblPackVoltage->setText(
                                QString("%1 V").arg(
                                            static_cast<double>(packet.pack_voltage) / 1000.0,
                                                5, 'f', 2));

                    ui->lblCurrent->setText(
                                QString("%1 A").arg(
                                            static_cast<double>(packet.current) / 1000.0,
                                                5, 'f', 2));


                    switch (packet.mode) {
                    case MODE_DISCHARGING:
                        ui->lblMode->setText(tr("Discharging"));
                        break;
                    case MODE_CHARGING:
                        ui->lblMode->setText(tr("Charging"));
                        break;
                    case MODE_LOAD_TEST:
                        ui->lblMode->setText(tr("Load Test"));
                        break;
                    }

                    ui->lblChargeState->setText(
                                QString("%1 Ah").arg(
                                    static_cast<double>(packet.charge_state) / 3600.0,
                                    6, 'f', 4));

                    ui->lblTemperature->setText(
                                QString("%1 °C").arg(
                                    static_cast<double>(packet.temperature) / 1000.0,
                                    4, 'f', 2));
                }

                m_inMessage = false;
                m_inHeader = false;
                m_dataBuffer.clear();
            }
        }
    }
}

void MainWindow::on_sleepTimerTimeout()
{
    m_waitingMessageBox->show();
}

void MainWindow::on_waitingMessageBoxButtonClicked(QAbstractButton *button)
{
    (void)button;
    m_serialPort->close();
    m_serialPort->deleteLater();
    m_serialPort = nullptr;
    qApp->quit();
}

void MainWindow::updatePlot(QLineSeries *series, double value)
{
//    QVector<QPointF> oldPoints = series->pointsVector();
//    QVector<QPointF> points;

//    points.append(QPointF(points.count(), value));
//    series->replace(points);
    QVector<QPointF> points = series->pointsVector();
    points.append(QPointF(points.count(), value));
    series->replace(points);

    if (points.count() > 100) {
        m_chart->axisX()->setRange(0, points.count());
    }
}

void MainWindow::on_chkPackVoltageAxisVisible_stateChanged(int state)
{
    m_chartSeriesPackVoltage->setVisible(state);
    m_chartAxisPackVoltage->setVisible(state);
}

void MainWindow::on_chkCurrentAxisVisible_stateChanged(int state)
{
    m_chartSeriesCurrent->setVisible(state);
    m_chartAxisCurrent->setVisible(state);
}

void MainWindow::on_chkChargeAxisVisible_stateChanged(int state)
{
    m_chartSeriesCharge->setVisible(state);
    m_chartAxisCharge->setVisible(state);
}

void MainWindow::on_chkTemperatureAxisVisible_stateChanged(int state)
{
    m_chartSeriesTemperature->setVisible(state);
    m_chartAxisTemperature->setVisible(state);
}

void MainWindow::on_actClearData_triggered()
{
    int result = QMessageBox::question(
                this,
                tr("Clear Data"),
                tr("Do you really want to clear all data from the graph?"));

    if (result == QMessageBox::Accepted) {
        m_chartSeriesCharge->clear();
        m_chartSeriesPackVoltage->clear();
        m_chartSeriesCurrent->clear();
        m_chartSeriesTemperature->clear();
    }
}

void MainWindow::on_actSaveData_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(
                this,
                tr("Choose a file to save data to"));

    if (!fileName.isEmpty()) {
        for (int i = 0; i < m_chartSeriesPackVoltage->count(); i++) {

        }
    }
}

void MainWindow::on_actCellBalancing_triggered()
{
    if (m_cellBalanceStatusForm == nullptr) {
        m_cellBalanceStatusForm = new CellBalanceStatusForm;
    }
    m_cellBalanceStatusForm->show();
    m_cellBalanceStatusForm->raise();
}
