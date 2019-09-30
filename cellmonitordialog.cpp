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
