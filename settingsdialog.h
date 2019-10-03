#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void on_cboAutoOpenPortName_currentIndexChanged(const QString &currentText);
    void on_chkAutoOpenPortEnabled_stateChanged(int checked);

    void on_cboUnitTemperature_currentIndexChanged(int index);

    void on_cboUnitCharge_currentIndexChanged(int index);

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
