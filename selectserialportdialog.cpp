#include "selectserialportdialog.h"
#include "ui_selectserialportdialog.h"

#include <QSerialPort>
#include <QSerialPortInfo>

QSerialPortInfo SelectSerialPortDialog::getSerialPortInfo(QWidget *parent)
{
    SelectSerialPortDialog dlg(parent);
    int result = dlg.exec();

    if (result == SelectSerialPortDialog::Accepted) {
        return dlg.m_serialPortList[dlg.ui->cboSerialPort->currentIndex()];
    }
    else {
        return QSerialPortInfo();
    }
}

SelectSerialPortDialog::SelectSerialPortDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SelectSerialPortDialog)
{
    ui->setupUi(this);

    m_serialPortList = QSerialPortInfo::availablePorts();

    for (QSerialPortInfo port : m_serialPortList) {
        ui->cboSerialPort->addItem(port.portName());
    }
}

SelectSerialPortDialog::~SelectSerialPortDialog()
{
    delete ui;
}
