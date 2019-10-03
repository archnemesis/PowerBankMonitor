#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QSerialPortInfo>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    QSettings settings;

    QString autoOpenPortName = settings.value("port/autoOpenPortName").toString();
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    for (QSerialPortInfo portInfo : portList) {
        ui->cboAutoOpenPortName->addItem(portInfo.portName());
    }

    if (settings.value("port/autoOpenPortEnabled").toBool()) {
        ui->chkAutoOpenPortEnabled->setChecked(true);
        ui->cboAutoOpenPortName->setCurrentText(autoOpenPortName);
    }

    QString unit_temp = settings.value("units/temperature", "celsius").toString();
    QString unit_charge = settings.value("units/charge", "coulomb").toString();

    if (unit_temp == "celsius") ui->cboUnitTemperature->setCurrentIndex(0);
    else if (unit_temp == "farenheit") ui->cboUnitTemperature->setCurrentIndex(1);

    if (unit_charge == "coulomb") ui->cboUnitCharge->setCurrentIndex(0);
    else if (unit_charge == "amphour") ui->cboUnitCharge->setCurrentIndex(1);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::on_cboAutoOpenPortName_currentIndexChanged(const QString &currentText)
{
    QSettings settings;
    settings.setValue("port/autoOpenPortName", currentText);
}

void SettingsDialog::on_chkAutoOpenPortEnabled_stateChanged(int checked)
{
    QSettings settings;
    settings.setValue("port/autoOpenPortEnabled", checked);
}

void SettingsDialog::on_cboUnitTemperature_currentIndexChanged(int index)
{
    QSettings settings;

    if (index == 0) settings.setValue("units/temperature", "celsius");
    else if (index == 1) settings.setValue("units/temperature", "farenheit");
}

void SettingsDialog::on_cboUnitCharge_currentIndexChanged(int index)
{
    QSettings settings;

    if (index == 0) settings.setValue("units/charge", "coulomb");
    else if (index == 1) settings.setValue("units/charge", "amphour");
}
