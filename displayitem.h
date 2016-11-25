#ifndef DISPLAYITEM_H
#define DISPLAYITEM_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QTimer>
#include <QThread>
#include <QDateTime>
#include <QMessageBox>

#include "UserUartLink.h"

#define STATE_OPEN_FAILED 0
#define STATE_WAITING_WRITE 1
#define STATE_WRITE_SUCCESS 2
#define STATE_WRITE_FAILED 3
#define STATE_WRITTING 4
#define STATE_CURRENT_WRITING 5

#define STYLE_OPEN_FAILED "background-color: gray"
#define STYLE_WAITTING_WRITE "background-color: green"
#define STYLE_WRITE_SUCCESS "background-color: blue"
#define STYLE_WRITE_FAILED "background-color: red"
#define STYLE_CURRENT_WRITTING "background-color: yellow"

namespace Ui {
class DisplayItem;
}

class DisplayItem : public QWidget
{
    Q_OBJECT

public:
    explicit DisplayItem(QWidget *parent = 0);
    ~DisplayItem();

    bool openSerialPort(QString port);
    static bool CorrcetProtocol;   //修正原代码中的协议错误

    int getCurrentState() const;
    void setCurrentState(int value);

    void writeDeviceId(QString str,QString keys);

    void WriteMessage(QString str);

private:
    Ui::DisplayItem *ui;
    QSerialPort serialPort;

    int currentState=0; // 0:未打开串口 1：等待写入 2：写入成功 3：写入失败 4:正在写入

    unsigned char receivedFrame[60]={0};
    unsigned char sendingFrame[60]={0};

    unsigned char sentcmd;      //已发送命令
    unsigned char sentcnt;      //已发送次数

    QTimer timerResend;

    QString deviceId;
    QByteArray deviceIDArray;
    QString key;

    void StartResend();
    void EndResend();
    void QueryId();
    void QueryKey();
    void QueryInfo();
    void WrDeviceID();
    void WriteKey();



private slots:
    void on_comboBox_currentIndexChanged(int index);
    void slotPortScan();
    void slotReceivedData();
    void slotResend();

signals:
    void signalSetCurState();
    void signalWriteOver(bool);
};

#endif // DISPLAYITEM_H
