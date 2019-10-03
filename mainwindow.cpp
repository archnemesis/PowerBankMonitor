#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "selectserialportdialog.h"
#include "settingsdialog.h"
#include "aboutdialog.h"
#include "statuspacket.h"

#include <math.h>
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QAbstractButton>
#include <QFileDialog>
#include <QTextStream>
#include <QDateTime>
#include <QColorDialog>
#include <QSettings>

#include <QtCharts/QChartView>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    bool dotest = false;

    QSettings settings;

    QColor defaultColor_voltage(Qt::red);
    QColor defaultColor_current(Qt::blue);
    QColor defaultColor_charge(Qt::green);
    QColor defaultColor_temperature(Qt::yellow);

    ui->setupUi(this);

    m_packStatusLabel = new QLabel;
    m_packStatusLabel->setText(tr("Waiting for pack..."));
    statusBar()->addWidget(m_packStatusLabel, 1);

    m_dataLogLabel = new QLabel;
    m_dataLogLabel->setText(tr("Data log inactive"));
    statusBar()->addWidget(m_dataLogLabel, 3);

    m_serialPortLabel = new QLabel;
    m_serialPortLabel->setText(tr("Not connected"));
    statusBar()->addWidget(m_serialPortLabel, 1);

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

    m_chart = new QChart();

    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);
    m_chart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);

    m_startDateTime = QDateTime::currentDateTime();

    m_chartAxisTime = new QDateTimeAxis;
    m_chartAxisTime->setMin(m_startDateTime);
    m_chartAxisTime->setMax(m_startDateTime.addSecs(300));
    m_chartAxisTime->setTitleText(tr("Time"));
    m_chartAxisTime->setFormat("h:mm:ss AP");
    m_chart->addAxis(m_chartAxisTime, Qt::AlignBottom);

    m_chartAxisCharge = new QValueAxis;

    //
    // changes to these settings only take effect on startup
    //
    m_chargeUnit = settings.value("units/charge", "coulomb").toString();
    m_temperatureUnit = settings.value("units/temperature", "celsius").toString();

    if (m_chargeUnit == "amphour") {
        m_chartAxisCharge->setTitleText(tr("Charge (Ah)"));
        m_chartAxisCharge->setMin(0);
        m_chartAxisCharge->setMax(3.2);
    }
    else {
        m_chartAxisCharge->setTitleText(tr("Charge (C)"));
        m_chartAxisCharge->setMin(0);
        m_chartAxisCharge->setMax(11520);
    }

    m_chartAxisCharge->setTickInterval(1000);
    m_chart->addAxis(m_chartAxisCharge, (settings.value("chart/axes/charge/orientation", true).toBool() ? Qt::AlignLeft : Qt::AlignRight));

    m_chartAxisCurrent = new QValueAxis;
    m_chartAxisCurrent->setMin(0);
    m_chartAxisCurrent->setMax(10);
    m_chartAxisCurrent->setTitleText(tr("Current (A)"));
    m_chartAxisCurrent->setTickInterval(0.1);
    m_chart->addAxis(m_chartAxisCurrent, (settings.value("chart/axes/current/orientation", true).toBool() ? Qt::AlignLeft : Qt::AlignRight));

    m_chartAxisPackVoltage = new QValueAxis;
    m_chartAxisPackVoltage->setMin(0);
    m_chartAxisPackVoltage->setMax(25.4);
    m_chartAxisPackVoltage->setTitleText(tr("Voltage (V)"));
    m_chartAxisPackVoltage->setTickInterval(1.0);
    m_chart->addAxis(m_chartAxisPackVoltage, (settings.value("chart/axes/voltage/orientation", true).toBool() ? Qt::AlignLeft : Qt::AlignRight));

    m_chartAxisTemperature = new QValueAxis;

    if (m_temperatureUnit == "farenheit") {
        m_chartAxisTemperature->setTitleText(tr("Temperature (F)"));
        m_chartAxisTemperature->setMin(64.4);
        m_chartAxisTemperature->setMax(212.0);
    }
    else {
        m_chartAxisTemperature->setTitleText(tr("Temperature (C)"));
        m_chartAxisTemperature->setMin(18.0);
        m_chartAxisTemperature->setMax(100.0);
    }

    m_chartAxisTemperature->setTickInterval(5);
    m_chart->addAxis(m_chartAxisTemperature, (settings.value("chart/axes/temperature/orientation", false).toBool() ? Qt::AlignLeft : Qt::AlignRight));

    m_chartSeriesPackVoltage = new QLineSeries;
    m_chartSeriesPackVoltage->setName(tr("Voltage"));
    m_chart->addSeries(m_chartSeriesPackVoltage);
    m_chartSeriesPackVoltage->attachAxis(m_chartAxisPackVoltage);
    m_chartSeriesPackVoltage->attachAxis(m_chartAxisTime);
    m_chartSeriesPackVoltage->setColor(settings.value("chart/axes/voltage/lineColor", defaultColor_voltage).value<QColor>());
    m_chartSeriesPackVoltage->setUseOpenGL(true);

    m_chartSeriesCurrent = new QLineSeries;
    m_chartSeriesCurrent->setName(tr("Current"));
    m_chart->addSeries(m_chartSeriesCurrent);
    m_chartSeriesCurrent->attachAxis(m_chartAxisCurrent);
    m_chartSeriesCurrent->attachAxis(m_chartAxisTime);
    m_chartSeriesCurrent->setColor(settings.value("chart/axes/current/lineColor", defaultColor_current).value<QColor>());
    m_chartSeriesCurrent->setUseOpenGL(true);

    m_chartSeriesCharge = new QLineSeries;
    m_chartSeriesCharge->setName(tr("Charge"));
    m_chart->addSeries(m_chartSeriesCharge);
    m_chartSeriesCharge->attachAxis(m_chartAxisCharge);
    m_chartSeriesCharge->attachAxis(m_chartAxisTime);
    m_chartSeriesCharge->setColor(settings.value("chart/axes/charge/lineColor", defaultColor_charge).value<QColor>());
    m_chartSeriesCharge->setUseOpenGL(true);

    m_chartSeriesTemperature = new QLineSeries;
    m_chartSeriesTemperature->setName(tr("Temperature"));
    m_chart->addSeries(m_chartSeriesTemperature);
    m_chartSeriesTemperature->attachAxis(m_chartAxisTemperature);
    m_chartSeriesTemperature->attachAxis(m_chartAxisTime);
    m_chartSeriesTemperature->setColor(settings.value("chart/axes/temperature/lineColor", defaultColor_temperature).value<QColor>());
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

void MainWindow::showEvent(QShowEvent *event)
{
    if (m_serialPort == nullptr) {
        QSettings settings;

        if (settings.value("port/autoOpenPortEnabled").toBool()) {
            while (true) {
                QString portName = settings.value("port/autoOpenPortName").toString();
                m_serialPort = new QSerialPort(portName);
                m_serialPort->setBaudRate(115200);
                m_serialPort->setParity(QSerialPort::NoParity);
                m_serialPort->setDataBits(QSerialPort::Data8);
                m_serialPort->setStopBits(QSerialPort::OneStop);
                m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

                connect(m_serialPort, &QSerialPort::readyRead, this, &MainWindow::on_serialPortReadyRead);

                if (!m_serialPort->open(QIODevice::ReadWrite)) {
                    int result = QMessageBox::information(
                                this,
                                tr("Port Error"),
                                tr("Unable to open serial port %1").arg(portName),
                                QMessageBox::Retry | QMessageBox::Cancel);
                    if (result == QMessageBox::Cancel) {
                        break;
                    }
                }
                else {
                    m_serialPortLabel->setText(QString("%1:115200").arg(m_serialPort->portName()));
                    m_waitingMessageBox->show();
                    event->accept();
                    return;
                }
            }
        }

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
                m_serialPortLabel->setText(QString("%1:115200").arg(m_serialPort->portName()));
                m_waitingMessageBox->show();
                event->accept();
                return;
            }
        }
    }
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
                    m_packStatusLabel->setText(tr("Last seen %1").arg(QDateTime::currentDateTime().toString("h:mm:ss AP")));

                    qreal voltage = static_cast<qreal>(packet.pack_voltage) / 1000.0;
                    qreal current = abs(static_cast<qreal>(packet.current) / 1000.0);
                    qreal charge = static_cast<qreal>(packet.charge_state);
                    qreal temperature = static_cast<qreal>(packet.temperature) / 1000.0;

                    if (m_dataLogFile != nullptr && m_dataLogFile->isOpen()) {
                        QTextStream stream(m_dataLogFile);
                        stream << QDateTime::currentDateTime().toString("h:mm:ss AP") << ",";
                        stream << voltage << ",";
                        stream << current << ",";
                        stream << charge << ",";
                        stream << temperature << "\n";
                    }

                    charge = convertCharge(charge);
                    temperature = convertTemperature(temperature);

                    updatePlot(
                        voltage,
                        current,
                        charge,
                        temperature
                        );

                    ui->lblPackVoltage->setText(QString("%1 V").arg(voltage, 5, 'f', 2));
                    ui->lblCurrent->setText(QString("%1 A").arg(current, 5, 'f', 2));

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
                                QString("%1 %2")
                                    .arg(charge, 6, 'f', 4)
                                    .arg(chargeSuffix()));

                    ui->lblTemperature->setText(
                                QString("%1 °%2")
                                    .arg(temperature, 4, 'f', 2)
                                    .arg(temperatureSuffix()));
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

qreal MainWindow::convertTemperature(qreal temperature_c)
{
    if (m_temperatureUnit == "farenheit") return ((temperature_c * (9.0/5.0)) + 32.0);
    else return temperature_c;
}

qreal MainWindow::convertCharge(qreal current_c)
{
    if (m_chargeUnit == "amphour") return (current_c / 3600);
    else return current_c;
}

QString MainWindow::chargeSuffix()
{
    if (m_chargeUnit == "amphour") return QString("Ah");
    else return QString("C");
}

QString MainWindow::temperatureSuffix()
{
    if (m_temperatureUnit == "farenheit") return QString("F");
    else return QString("C");
}

void MainWindow::on_actClearData_triggered()
{
    int result = QMessageBox::question(
                this,
                tr("Battery Pack Analyzer"),
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
                tr("Save As"));

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

void MainWindow::on_actPackVoltageShow_triggered(bool checked)
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
                    tr("Save As"));

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
                ui->actStartLogging->setEnabled(false);
                ui->actStopLogging->setEnabled(true);
                m_dataLogLabel->setText(tr("Logging data to %1").arg(fileName));

                int result = QMessageBox::question(
                            this,
                            tr("Save Buffered Data"),
                            tr("Do you want to save all data buffered so far?"),
                            QMessageBox::Yes | QMessageBox::No);

                if (result == QMessageBox::Yes) {
                    for (int i = 0; i < m_chartSeriesPackVoltage->count(); i++) {
                        qreal timestamp = m_chartSeriesPackVoltage->at(i).x();
                        QDateTime timestampDateTime = QDateTime::fromMSecsSinceEpoch(timestamp);

                        qreal voltage = m_chartSeriesPackVoltage->at(i).y();
                        qreal current = m_chartSeriesCurrent->at(i).y();
                        qreal charge = m_chartSeriesCharge->at(i).y();
                        qreal temperature = m_chartSeriesTemperature->at(i).y();
                        stream << timestampDateTime.toString("h:mm:ss AP") << ",";
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

void MainWindow::on_actStopLogging_triggered()
{
    if (m_dataLogFile && m_dataLogFile->isOpen()) {
        int result = QMessageBox::question(
                    this,
                    tr("Battery Pack Analyzer"),
                    tr("Stop data logging?"),
                    QMessageBox::Yes | QMessageBox::No);

        if (result == QMessageBox::Yes) {
            m_dataLogFile->close();
            m_dataLogFile->deleteLater();
            m_dataLogFile = nullptr;

            ui->actStartLogging->setEnabled(true);
            ui->actStopLogging->setEnabled(false);
            m_dataLogLabel->setText(tr("Data log inactive"));
        }
    }
}

void MainWindow::on_actCurrentShow_triggered(bool checked)
{
   m_chartAxisCurrent->setVisible(checked);
}

void MainWindow::on_actChargeShow_triggered(bool checked)
{
    m_chartAxisCharge->setVisible(checked);
}

void MainWindow::on_actTemperatureShow_triggered(bool checked)
{
    m_chartAxisTemperature->setVisible(checked);
}

void MainWindow::on_actPackVoltageColor_triggered()
{
    QColor oldColor = m_chartSeriesPackVoltage->color();
    QColor newColor = QColorDialog::getColor(
                oldColor,
                this,
                tr("Choose Line Color"));

    if (newColor.isValid()) {
        m_chartSeriesPackVoltage->setColor(newColor);

        QSettings settings;
        settings.setValue("chart/axes/voltage/lineColor", newColor);
    }
}

void MainWindow::on_actCurrentColor_triggered()
{
    QColor oldColor = m_chartSeriesCurrent->color();
    QColor newColor = QColorDialog::getColor(
                oldColor,
                this,
                tr("Choose Line Color"));

    if (newColor.isValid()) {
        m_chartSeriesCurrent->setColor(newColor);

        QSettings settings;
        settings.setValue("chart/axes/current/lineColor", newColor);
    }
}

void MainWindow::on_actChargeColor_triggered()
{
    QColor oldColor = m_chartSeriesCharge->color();
    QColor newColor = QColorDialog::getColor(
                oldColor,
                this,
                tr("Choose Line Color"));

    if (newColor.isValid()) {
        m_chartSeriesCharge->setColor(newColor);

        QSettings settings;
        settings.setValue("chart/axes/charge/lineColor", newColor);
    }
}

void MainWindow::on_actTemperatureColor_triggered()
{
    QColor oldColor = m_chartSeriesTemperature->color();
    QColor newColor = QColorDialog::getColor(
                oldColor,
                this,
                tr("Choose Line Color"));

    if (newColor.isValid()) {
        m_chartSeriesTemperature->setColor(newColor);

        QSettings settings;
        settings.setValue("chart/axes/temperature/lineColor", newColor);
    }
}

void MainWindow::on_actPackVoltageOrientationLeft_triggered(bool checked)
{
    m_chart->removeAxis(m_chartAxisPackVoltage);
    m_chart->addAxis(m_chartAxisPackVoltage, checked ? Qt::AlignLeft : Qt::AlignRight);
    ui->actPackVoltageOrientationRight->setChecked(!checked);
    QSettings settings;
    settings.setValue("chart/axes/voltage/orientation", checked);
}

void MainWindow::on_actPackVoltageOrientationRight_triggered(bool checked)
{
    m_chart->removeAxis(m_chartAxisPackVoltage);
    m_chart->addAxis(m_chartAxisPackVoltage, !checked ? Qt::AlignLeft : Qt::AlignRight);
    ui->actPackVoltageOrientationLeft->setChecked(!checked);
    QSettings settings;
    settings.setValue("chart/axes/voltage/orientation", !checked);
}

void MainWindow::on_actCurrentOrientationLeft_triggered(bool checked)
{
    m_chart->removeAxis(m_chartAxisCurrent);
    m_chart->addAxis(m_chartAxisCurrent, checked ? Qt::AlignLeft : Qt::AlignRight);
    ui->actCurrentOrientationRight->setChecked(!checked);
    QSettings settings;
    settings.setValue("chart/axes/current/orientation", checked);
}

void MainWindow::on_actCurrentOrientationRight_triggered(bool checked)
{
    m_chart->removeAxis(m_chartAxisCurrent);
    m_chart->addAxis(m_chartAxisCurrent, !checked ? Qt::AlignLeft : Qt::AlignRight);
    ui->actCurrentOrientationLeft->setChecked(!checked);
    QSettings settings;
    settings.setValue("chart/axes/current/orientation", !checked);
}

void MainWindow::on_actChargeOrientationLeft_triggered(bool checked)
{
    m_chart->removeAxis(m_chartAxisCharge);
    m_chart->addAxis(m_chartAxisCharge, checked ? Qt::AlignLeft : Qt::AlignRight);
    ui->actChargeOrientationRight->setChecked(!checked);
    QSettings settings;
    settings.setValue("chart/axes/charge/orientation", checked);
}

void MainWindow::on_actChargeOrientationRight_triggered(bool checked)
{
    m_chart->removeAxis(m_chartAxisCharge);
    m_chart->addAxis(m_chartAxisCharge, !checked ? Qt::AlignLeft : Qt::AlignRight);
    ui->actChargeOrientationLeft->setChecked(!checked);
    QSettings settings;
    settings.setValue("chart/axes/charge/orientation", !checked);
}

void MainWindow::on_actTemperatureOrientationLeft_triggered(bool checked)
{
    m_chart->removeAxis(m_chartAxisTemperature);
    m_chart->addAxis(m_chartAxisTemperature, checked ? Qt::AlignLeft : Qt::AlignRight);
    ui->actTemperatureOrientationRight->setChecked(!checked);
    QSettings settings;
    settings.setValue("chart/axes/temperature/orientation", checked);
}

void MainWindow::on_actTemperatureOrientationRight_triggered(bool checked)
{
    m_chart->removeAxis(m_chartAxisTemperature);
    m_chart->addAxis(m_chartAxisTemperature, !checked ? Qt::AlignLeft : Qt::AlignRight);
    ui->actTemperatureOrientationLeft->setChecked(!checked);
    QSettings settings;
    settings.setValue("chart/axes/temperature/orientation", !checked);
}

void MainWindow::on_actViewSettings_triggered()
{
    SettingsDialog settings(this);
    settings.exec();
}

void MainWindow::on_actAbout_triggered()
{
    AboutDialog dlg(this);
    dlg.exec();
}
