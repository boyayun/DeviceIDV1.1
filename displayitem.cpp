#include "displayitem.h"
#include "ui_displayitem.h"

/***********协议命令*************/
#define CMDCOR      //校正协议命令

#define CmdAck      0x05

#define CmdRdId     0x10
#define CmdRdIdAck  0x11

#ifdef  CMDCOR
#define CmdRdKey    0x14        //new protocol
#else
#define CmdRdKey    0x87        //old protocol
#endif
#define CmdRdKeyAck 0x15

#define CmdRdInfo    0x82
#define CmdRDInfoAck 0x83
#define CmdWrID      0x85

#ifdef  CMDCOR
#define CmdWrKey     0x87       //new protocol'
#else
#define CmdWrKey     0x14       //old protocol
#endif

bool DisplayItem::CorrcetProtocol = true;

DisplayItem::DisplayItem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DisplayItem)
{
    ui->setupUi(this);
    serialPort.setBaudRate(QSerialPort::Baud9600);
    serialPort.setParity(QSerialPort::NoParity);
    serialPort.setStopBits(QSerialPort::OneStop);
    serialPort.setFlowControl(QSerialPort::NoFlowControl);
    this->currentState = 0;
    setCurrentState(STATE_OPEN_FAILED);

    timerResend.setSingleShot(true);
    connect(&serialPort,SIGNAL(readyRead()),this,SLOT(slotReceivedData()));
    connect(&timerResend,SIGNAL(timeout()),this,SLOT(slotResend()));

    this->sentcnt = 0;
}

DisplayItem::~DisplayItem()
{
    this->serialPort.close();
    delete ui;
}

void DisplayItem::slotPortScan()
{
    QSerialPortInfo::availablePorts();
    foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts()) {
        this->ui->comboBox->addItem(info.portName());
    }
}

void DisplayItem::on_comboBox_currentIndexChanged(int index)
{
    qDebug()<<this->ui->comboBox->itemText(index);
    if(serialPort.isOpen())
    {
        serialPort.close();
    }
    if(this->ui->comboBox->itemText(index).contains("COM"))
    {
        this->serialPort.setPortName(ui->comboBox->itemText(index));
        if(serialPort.open(QIODevice::ReadWrite))
        {
            this->ui->lineEdit->setText("串口"+this->ui->comboBox->itemText(index)+"打开成功!");
            setCurrentState(STATE_WAITING_WRITE);
        }
        else
        {
            setCurrentState(STATE_OPEN_FAILED);
            this->ui->comboBox->setCurrentIndex(0);
        }
    }
    else
    {
        this->ui->lineEdit->setText("请打开串口");
        setCurrentState(STATE_OPEN_FAILED);
        this->ui->comboBox->setCurrentIndex(0);
    }
    emit signalSetCurState();
}

int DisplayItem::getCurrentState() const
{
    return currentState;
}

void DisplayItem::setCurrentState(int value)
{
    this->ui->label->setStyleSheet("color: black");
    if(value == STATE_OPEN_FAILED)
    {
        this->setStyleSheet("background-color: gray");
        this->ui->label->setText("串口打开失败！");
    }
    else if(value == STATE_WAITING_WRITE)
    {
        this->setStyleSheet("background-color: green;");
        this->ui->label->setText("待输入！");
    }
    else if(value == STATE_WRITE_FAILED)
    {
        this->setStyleSheet("background-color: red;");
        this->ui->label->setText(deviceId.mid(deviceId.length()-6)+"写失败");
    }
    else if(value == STATE_WRITE_SUCCESS)
    {
        this->setStyleSheet("background-color: blue;");
        this->ui->label->setText(deviceId.mid(deviceId.length()-6)+"写成功");
    }
    else if(value == STATE_WRITTING)
    {
        this->setStyleSheet("background-color: green;");
        this->ui->label->setText(deviceId.mid(deviceId.length()-6)+"正在写");
    }
    else if(value == STATE_CURRENT_WRITING)
    {
        this->ui->label->setText("当前输入");
        ///////////////////////////////////
        this->serialPort.close();
        this->serialPort.open(QIODevice::ReadWrite);
        ////////////////////////////////////
        this->setStyleSheet(STYLE_CURRENT_WRITTING);
    }
    else
    {

    }
    currentState = value;
}

void DisplayItem::WriteMessage(QString str)
{
    this->ui->label->setStyleSheet("color:red");
    this->ui->label->setText(str);

}

void DisplayItem::slotResend()
{
    if(3 != sentcnt)
    {
        switch (sentcmd)
        {
            case CmdRdId:
                this->QueryId();
                break;

            case CmdRdKey:
                QueryKey();
                break;

            case CmdWrID:
                WrDeviceID();
                break;

            case CmdWrKey:
                WriteKey();
                break;

            default:
                break;
        }
    }
    else
    {
        EndResend();
        setCurrentState(STATE_WRITE_FAILED);
        emit signalWriteOver(false);
    }
}

void DisplayItem::StartResend()
{
    this->timerResend.start(200);
    sentcnt++;
}

void DisplayItem::EndResend()
{
    this->timerResend.stop();
    sentcnt = 0;
}

void DisplayItem::QueryId()
{
    unsigned char queryFrame[]={0xf5,0xf5,0x00,0x03,CmdRdId,0x55,0x10};
    serialPort.write((char *)queryFrame,7);
    StartResend();
    sentcmd = CmdRdId;
}

void DisplayItem::QueryKey()
{
#ifdef CMDCOR
    unsigned char queryFrame[] ={0xf5,0xf5,0x00,0x03,CmdRdKey,0x13,0x34};
#else
    unsigned char queryFrame[] ={0xf5,0xf5,0x00,0x03,CmdRdKey,0xb5,0x26};
#endif
    serialPort.write((char *) queryFrame,7);
    StartResend();
    sentcmd = CmdRdKey;
}

void DisplayItem::writeDeviceId(QString str,QString keys)
{
    this->deviceId = str;
    this->key = keys;
    setCurrentState(STATE_WRITTING);

    QByteArray writeData ;
    for( int i=0;i<str.length();i=i+2)
    {
        writeData.append(str.mid(i,2).toInt(0,16));
    }
    for(int i=0;i<8;i++)
    {
        writeData.append('\0');
    }
    this->deviceIDArray = writeData;
    qDebug()<<__FILE__<<__LINE__<<writeData.length();

    WrDeviceID();
}

void DisplayItem::WrDeviceID()
{
    unsigned char dataFrame[60];
    dataFrame[0] = 0x85;

    memcpy(dataFrame+1,this->deviceIDArray.data(),this->deviceIDArray.length());

    unsigned char len = UserUartLinkPack(this->sendingFrame,dataFrame,this->deviceIDArray.length()+1,0);
    qDebug()<<"写入设备ID帧："<<QByteArray((char *)this->sendingFrame,len).toHex();

    serialPort.write((char *)sendingFrame,len);
    StartResend();
    sentcmd = CmdWrID;
}

void DisplayItem::WriteKey()
{
    qDebug()<<"写秘钥";
    QByteArray writeData;
    unsigned char dataFrame[60];
    qDebug()<<this->key;

    writeData = this->key.toLocal8Bit();

    dataFrame[0] = CmdWrKey;

    memcpy(dataFrame+1,writeData.data(),writeData.length());

    int len = UserUartLinkPack(this->sendingFrame,dataFrame,writeData.length()+1,0);
    qDebug()<<QByteArray((char *)this->sendingFrame,len).toHex();
    serialPort.write((char *)sendingFrame,len);

    StartResend();
    sentcmd = CmdWrKey;
}

void DisplayItem::slotReceivedData()
{
    if(this->currentState == STATE_WRITTING)
    {
        QByteArray receivedData = serialPort.readAll();
        bool uartsuccess = UserUartLinkUnpack((unsigned char *)receivedData.data(),receivedData.length());

        if(uartsuccess)
        {
            int len = getUserUartLinkMsg(this->receivedFrame);
            qDebug()<<"received frame is:"<<QByteArray((char*)receivedFrame,len);

            switch(this->receivedFrame[0])
            {
                case CmdAck:
                {
                    EndResend();
                    switch (sentcmd)
                    {
                        case CmdWrID:
                            QueryId();
                            break;

                        case CmdWrKey:
                            QueryKey();
                            break;

                        default:
                            break;
                    }
                }
                break;

                case CmdRdIdAck:
                        qDebug()<<"deviceid is:"<<deviceId;
                        qDebug()<<"received deviceid is:"<<QByteArray((char*)receivedFrame+1,len);

                        if(this->deviceIDArray == QByteArray((char *)receivedFrame+1,36))
                        {
                            EndResend();
                            WriteKey();
                        }
                    break;

                case CmdRdKeyAck: // key report frame
                        qDebug()<<QTime::currentTime()<<":key is:"<<this->key;
                        qDebug()<<"received key is:"<<QByteArray((char *)receivedFrame+1,16);

                        if(this->key == QString(QByteArray((char *)receivedFrame+1,16)))
                        {
                            sentcmd = 0;
                            EndResend();
                            setCurrentState(STATE_WRITE_SUCCESS);
                            emit signalWriteOver(true);
                        }
                    break;

                default:
                    break;
            }
        }
    }
}
