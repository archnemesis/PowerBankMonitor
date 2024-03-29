#ifndef CELLMONITORDIALOG_H
#define CELLMONITORDIALOG_H

#include <QDialog>

namespace Ui {
class CellMonitorDialog;
}

class CellMonitorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CellMonitorDialog(QWidget *parent = nullptr);
    ~CellMonitorDialog();
    void setCellVoltage(int cell, qreal voltage);

private:
    Ui::CellMonitorDialog *ui;
};

#endif // CELLMONITORDIALOG_H
