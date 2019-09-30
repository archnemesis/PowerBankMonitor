#ifndef CELLBALANCESTATUSFORM_H
#define CELLBALANCESTATUSFORM_H

#include <QWidget>

namespace Ui {
class CellBalanceStatusForm;
}

class CellBalanceStatusForm : public QWidget
{
    Q_OBJECT

public:
    explicit CellBalanceStatusForm(QWidget *parent = nullptr);
    ~CellBalanceStatusForm();

private:
    Ui::CellBalanceStatusForm *ui;
};

#endif // CELLBALANCESTATUSFORM_H
