#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "selectserialportdialog.h"
#include "statuspacket.h"

#include <math.h>
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QAbstractButton>
#include <QFileDialog>
#include <QTextStream>
#include <QDateTime>

#include <QtCharts/QChartView>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    bool dotest = false;

    ui->setupUi(this);

    m_waitingMessageBox = new QMessageBox(this);
    m_waitingMessageBox->setWindowTitle(tr("Waiting for Pack"));
    m_waitingMessageBox->setText(tr("Wake the pack by pressing the wake button, or by connecting a load or charger."));
    m_waitingMessageBox->setStandardButtons(QMessageBox::Ok);
    m_waitingMessageBox->setModal(true);
    connect(m_waitingMessageBox, &QMessageBox::buttonClicked, this, &MainWindow::on_waitingMessageBoxButtonClicked);

    m_sleepTimer = new QTimer(this);
    m_sleepTimer->setInterval(2000);
    m_sleepTimer->setSingleShot(true);
    connect(m_sleepTimer, &QTimer::timeout, this, &MainWindow::on_sleepTimerTimeout);

    m_chartUpdateTimer = new QTimer(this);
    m_chartUpdateTimer->setInterval(1000);
    m_chartUpdateTimer->setSingleShot(false);
    connect(m_chartUpdateTimer, &QTimer::timeout, this, &MainWindow::on_chartUpdateTimer_timeout);

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
    else {
        dotest = true;
    }

    m_chart = new QChart();
    //m_chart->legend()->setVisible(false);

    m_startDateTime = QDateTime::currentDateTime();

    m_chartAxisTime = new QDateTimeAxis;
    m_chartAxisTime->setMin(m_startDateTime);
    m_chartAxisTime->setMax(m_startDateTime.addSecs(300));
    m_chartAxisTime->setTitleText(tr("Time"));
    m_chartAxisTime->setFormat("h:mm:ss AP");
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
    m_chartSeriesPackVoltage->setUseOpenGL(true);

    m_chartSeriesCurrent = new QLineSeries;
    m_chart->addSeries(m_chartSeriesCurrent);
    m_chartSeriesCurrent->attachAxis(m_chartAxisCurrent);
    m_chartSeriesCurrent->attachAxis(m_chartAxisTime);
    m_chartSeriesCurrent->setUseOpenGL(true);

    m_chartSeriesCharge = new QLineSeries;
    m_chart->addSeries(m_chartSeriesCharge);
    m_chartSeriesCharge->attachAxis(m_chartAxisCharge);
    m_chartSeriesCharge->attachAxis(m_chartAxisTime);
    m_chartSeriesCharge->setUseOpenGL(true);

    m_chartSeriesTemperature = new QLineSeries;
    m_chart->addSeries(m_chartSeriesTemperature);
    m_chartSeriesTemperature->attachAxis(m_chartAxisTemperature);
    m_chartSeriesTemperature->attachAxis(m_chartAxisTime);
    m_chartSeriesTemperature->setUseOpenGL(true);

    ui->chartView->setChart(m_chart);
    m_chartUpdateTimer->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    int result = QMessageBox::question(
                this,
                tr("Exit Application"),
                tr("Do you really want to exit the application?"),
                QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::No) {
        event->ignore();
        return;
    }

    if (m_dataLogFile != nullptr) {
        if (m_dataLogFile->isOpen()) {
            m_dataLogFile->close();
        }
        m_dataLogFile->deleteLater();
        m_dataLogFile = nullptr;
    }

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

                    qreal voltage = static_cast<qreal>(packet.pack_voltage) / 1000.0;
                    qreal current = abs(static_cast<qreal>(packet.current) / 1000.0);
                    qreal charge = static_cast<qreal>(packet.charge_state);
                    qreal temperature = static_cast<qreal>(packet.temperature) / 1000.0;

                    if (m_dataLogFile != nullptr && m_dataLogFile->isOpen()) {
                        QTextStream stream(m_dataLogFile);
                        stream << m_chartSeriesPackVoltage->count() << ",";
                        stream << voltage << ",";
                        stream << current << ",";
                        stream << charge << ",";
                        stream << temperature << "\n";
                    }

                    updatePlot(
                        voltage,
                        current,
                        charge,
                        temperature
                        );

                    ui->lblPackVoltage->setText(
                                QString("%1 V").arg(voltage, 5, 'f', 2));

                    ui->lblCurrent->setText(
                                QString("%1 A").arg(current, 5, 'f', 2));

                    for (int i = 0; i < 6; i++) {
                        qreal v = static_cast<qreal>(packet.cell_voltage[i]) / 1000.0;
                        if (m_cellBalanceStatusForm != nullptr) {
                            m_cellBalanceStatusForm->setCellVoltage(i, v);
                        }
                    }

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
    m_waitingMessageBox->raise();
}

void MainWindow::on_waitingMessageBoxButtonClicked(QAbstractButton *button)
{
    //close();
}

void MainWindow::on_chartUpdateTimer_timeout()
{
    qreal width = m_chart->geometry().width();

    qDebug() << "Chart Area Width:" << width;

    if (m_chartDataPackVoltage.count() > 300) {

    }
}

void MainWindow::updatePlot(qreal voltage, qreal current, qreal charge, qreal temperature)
{
    QDateTime timestamp = QDateTime::currentDateTime();

    m_chartSeriesPackVoltage->append(timestamp.toMSecsSinceEpoch(), voltage);
    m_chartSeriesCurrent->append(timestamp.toMSecsSinceEpoch(), current);
    m_chartSeriesCharge->append(timestamp.toMSecsSinceEpoch(), charge);
    m_chartSeriesTemperature->append(timestamp.toMSecsSinceEpoch(), temperature);

    if ((timestamp.toSecsSinceEpoch() - m_startDateTime.toSecsSinceEpoch()) > 300) {
        m_chartAxisTime->setMax(timestamp);
    }
}

void MainWindow::on_actClearData_triggered()
{
    int result = QMessageBox::question(
                this,
                tr("Clear Data"),
                tr("Do you really want to clear all data from the graph?"));

    if (result == QMessageBox::Yes) {

        m_chartSeriesCharge->clear();
        m_chartSeriesPackVoltage->clear();
        m_chartSeriesCurrent->clear();
        m_chartSeriesTemperature->clear();

        m_startDateTime = QDateTime::currentDateTime();
        m_chartAxisTime->setMin(m_startDateTime);
        m_chartAxisTime->setMax(m_startDateTime.addSecs(300));
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
        m_cellBalanceStatusForm = new CellMonitorDialog;
        m_cellBalanceStatusForm->setModal(false);
    }
    m_cellBalanceStatusForm->show();
    m_cellBalanceStatusForm->raise();
}

void MainWindow::on_actShowHidePackVoltage_triggered(bool checked)
{
    m_chartSeriesPackVoltage->setVisible(checked);
    m_chartAxisPackVoltage->setVisible(checked);
}

void MainWindow::on_actShowHideCurrent_triggered(bool checked)
{
    m_chartSeriesCurrent->setVisible(checked);
    m_chartAxisCurrent->setVisible(checked);
}

void MainWindow::on_actShowHideChargeLevel_triggered(bool checked)
{
    m_chartSeriesCharge->setVisible(checked);
    m_chartAxisCharge->setVisible(checked);
}

void MainWindow::on_actShowHideTemperature_triggered(bool checked)
{
    m_chartSeriesTemperature->setVisible(checked);
    m_chartAxisTemperature->setVisible(checked);
}

void MainWindow::on_actExit_triggered()
{
    close();
}

void MainWindow::on_actStartLogging_triggered()
{
    while (true) {
        QString fileName = QFileDialog::getSaveFileName(
                    this,
                    tr("Choose a file to save data to"));

        if (!fileName.isEmpty()) {
            m_dataLogFile = new QFile(fileName);

            QTextStream stream(m_dataLogFile);
            stream << "time,voltage,current,charge,temperature\n";

            if (!m_dataLogFile->open(QIODevice::WriteOnly)) {
                int result = QMessageBox::critical(
                            this,
                            tr("File Error"),
                            tr("Could not open file %1 for writing!").arg(fileName),
                            QMessageBox::Retry | QMessageBox::Cancel);
                if (result == QMessageBox::Cancel) {
                    return;
                }
            }
            else {
                int result = QMessageBox::question(
                            this,
                            tr("Save Buffered Data"),
                            tr("Do you want to save all data buffered so far?"),
                            QMessageBox::Yes | QMessageBox::No);

                if (result == QMessageBox::Yes) {
                    for (int i = 0; i < m_chartSeriesPackVoltage->count(); i++) {
                        qreal voltage = m_chartSeriesPackVoltage->at(i).y();
                        qreal current = m_chartSeriesCurrent->at(i).y();
                        qreal charge = m_chartSeriesCharge->at(i).y();
                        qreal temperature = m_chartSeriesTemperature->at(i).y();
                        stream << i << ",";
                        stream << voltage << ",";
                        stream << current << ",";
                        stream << charge << ",";
                        stream << temperature << "\n";
                    }
                }

                return;
            }
        }
        else {
            return;
        }
    }
}
