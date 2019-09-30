#include "cellbalancestatusform.h"
#include "ui_cellbalancestatusform.h"

CellBalanceStatusForm::CellBalanceStatusForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CellBalanceStatusForm)
{
    ui->setupUi(this);
}

CellBalanceStatusForm::~CellBalanceStatusForm()
{
    delete ui;
}
