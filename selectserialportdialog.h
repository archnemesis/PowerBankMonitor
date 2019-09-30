#ifndef SELECTSERIALPORTDIALOG_H
#define SELECTSERIALPORTDIALOG_H

#include <QDialog>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QList>

namespace Ui {
class SelectSerialPortDialog;
}

class SelectSerialPortDialog : public QDialog
{
    Q_OBJECT

public:
    static QSerialPortInfo getSerialPortInfo(QWidget *parent = nullptr);
    explicit SelectSerialPortDialog(QWidget *parent = nullptr);
    ~SelectSerialPortDialog();

private:
    Ui::SelectSerialPortDialog *ui;
    QList<QSerialPortInfo> m_serialPortList;
};

#endif // SELECTSERIALPORTDIALOG_H
