#include "cellmonitordialog.h"
#include "ui_cellmonitordialog.h"

CellMonitorDialog::CellMonitorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CellMonitorDialog)
{
    ui->setupUi(this);
}

CellMonitorDialog::~CellMonitorDialog()
{
    delete ui;
}

void CellMonitorDialog::setCellVoltage(int cell, qreal voltage)
{
    QLabel *lbl;
    QProgressBar *pgb;

    switch (cell) {
    case 0:
        lbl = ui->lblCellVoltage1;
        pgb = ui->pgbCellVoltage1;
        break;
    case 1:
        lbl = ui->lblCellVoltage2;
        pgb = ui->pgbCellVoltage2;
        break;
    case 2:
        lbl = ui->lblCellVoltage3;
        pgb = ui->pgbCellVoltage3;
        break;
    case 3:
        lbl = ui->lblCellVoltage4;
        pgb = ui->pgbCellVoltage4;
        break;
    case 4:
        lbl = ui->lblCellVoltage5;
        pgb = ui->pgbCellVoltage5;
        break;
    case 5:
        lbl = ui->lblCellVoltage6;
        pgb = ui->pgbCellVoltage6;
        break;
    default:
        return;
    }

    lbl->setText(QString("%1 V").arg(voltage, 4, 'f', 2));
    pgb->setValue(static_cast<int>(voltage * 1000));
}
