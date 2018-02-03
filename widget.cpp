#include "widget.h"
#include "ui_widget.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QList>
#include <QFileDialog>
#include <QFile>
#include <QTime>
#include <QDate>
#include <QTimer>
#include <QMessageBox>
#include <QMenu>
#include <QTextCursor>
//#include <mybutton.h>
#include <QTimer>
#include <QtTest/QTest>
#include <synchapi.h>
#include <QThread>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    isConnected = 0;
    ui->bandRateBox->addItem(tr("9600"));
    ui->bandRateBox->addItem(tr("38400"));
    ui->bandRateBox->addItem(tr("115200"));
    on_pushButton_clicked();

    ui->textEdit->setReadOnly(true);
    QAction *clear = new QAction(tr("Clear Screen"), ui->textEdit);
    QAction *selectAll = new QAction(tr("Select All"), ui->textEdit);
    QAction *copy = new QAction(tr("Copy"), ui->textEdit);
    ui->textEdit->addAction(selectAll);
    ui->textEdit->addAction(copy);
    ui->textEdit->addAction(clear);
    connect(selectAll, SIGNAL(triggered(bool)), this, SLOT(textEditSelectAll()));
    connect(copy, SIGNAL(triggered(bool)), this, SLOT(textEditCopy()));
    connect(clear, SIGNAL(triggered(bool)), this, SLOT(clearScreen()));
    ui->textEdit->setContextMenuPolicy(Qt::ActionsContextMenu);

    port = new QSerialPort(this);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(displayCurrentTime()));
    timer->start(1000);

    carInfoTimer = new QTimer(this);

    connect(port, SIGNAL(readyRead()), this, SLOT(readPortData()));

#if 0
    configFile = new QFile("./config.txt");
    configFile->open(QIODevice::ReadWrite);
    if (configFile->size() == 0) {
        QMessageBox *box = new QMessageBox(this);
        box->setText(tr("配置文件异常！"));
        box->show();
    }
#endif

    ui->plainTextEdit->setReadOnly(true);

    /** init car info */
    Speed = 0;
    Gear = 0;
    TurnLeft = 0;
    TurnRight = 0;
    Acc = 0;
    Youmen = 0;
    Brake = 0;
    HandBrake = 0;
    Yuan = 0;
    Jin = 0;
    Fog = 0;
    Postion = 0;
    SeatBelt = 0;
    ARS = 0;

    autoTestTimer = new QTimer(this);

    recordFlag = 0;


}

Widget::~Widget()
{
    delete ui;
}

void Widget::printDebugInfo(QString info)
{
    QString time = QTime::currentTime().toString("hh:mm:ss.zzz");
    ui->textEdit->append("["+ time +"] " + info);
}

char Widget::charTohex(char ch)
{
    if((ch >= '0') && (ch <= '9'))
        return ch-0x30;
    else if((ch >= 'A') && (ch <= 'F'))
        return ch-'A'+10;
    else if((ch >= 'a') && (ch <= 'f'))
        return ch - 'a' + 10;
    else
        return -1;
}

int Widget::stringTohex(const char *str, char *buf)
{
    char hhex, lhex;
    int hexlen = 0;
    int len = qstrlen(str);
    char hstr, lstr;

    for (int i = 0; i < len; ) {
        hstr = str[i];
        if (hstr == ' ') {
            i++;
            continue;
        }
        i++;
        if (i > len)
            break;
        lstr = str[i];
        hhex = charTohex(hstr);
        lhex = charTohex(lstr);
        if ((hhex == 16) || (lhex == 16))
            break;
        else
            hhex = hhex * 16 + lhex;
        i++;
        buf[hexlen] = hhex;
        hexlen++;
    }
    return hexlen;
}

quint16 Widget::crc16_check(char *data, int len)
{
    quint8 i, j = 0;
    quint16 crc16 = 0;

    while (len--) {
        for (i = 0x80; i != 0; i >>= 1) {
            if((crc16 & 0x8000) != 0) {
                crc16 <<= 1;
                crc16 ^= 0x1021;
            } else {
                crc16 <<= 1;
            }

            if ((data[j] & i) != 0) {
                crc16 ^= 0x1021;
            }
        }
        j++;
    }
    return crc16;
}

void Widget::writeButtonData(char *buttonName)
{
    char data[1024] = {0};
    char *temp = NULL;
    int len = 0;
    quint16 checkSum = 0;

    if (!configFile->isOpen() || !port->isOpen()) {
        printDebugInfo(tr("串口未连接！"));
        return;
    }
    while (!configFile->atEnd()) {
        len = configFile->readLine(data, sizeof(data));
        if (strstr(data, buttonName)) {
            data[len - 1] = '\0';
            break;
        }
    }
    configFile->seek(0);
    temp = strchr(data, ' ') + 1;
    len = stringTohex(temp, data);
    //checkSum = crc16_check(data, len);
    //data[len] = char(checkSum >> 8);
    //data[len + 1] = char(checkSum & 0xFF);
    //len = len + 2;
    port->write(data, len);

}

char Widget::DecToHex(int num)
{
}


void Widget::on_pushButton_clicked()
{
    portInfoList = QSerialPortInfo::availablePorts();
    if (portInfoList.isEmpty())
        return;
    ui->serialNameBox->clear();
    foreach(QSerialPortInfo temp, portInfoList) {
        ui->serialNameBox->addItem(temp.portName());
    }
    selectedPortInfo = portInfoList.at(0);
}

void Widget::on_connectButton_clicked()
{
    bool ok;
    int baudRate = ui->bandRateBox->currentText().toInt(&ok);
    port->setPort(selectedPortInfo);
    if (isConnected == 0) {
        port->open(QIODevice::ReadWrite);
        if (!port->isOpen()) {
            QMessageBox *box = new QMessageBox(this);
            box->setText(tr("连接失败！"));
            box->show();
        } else {
            isConnected = 1;
            port->setBaudRate(baudRate);
            port->setDataBits(QSerialPort::Data8);
            port->setFlowControl(QSerialPort::NoFlowControl);
            port->setParity(QSerialPort::NoParity);
            port->setStopBits(QSerialPort::OneStop);
            ui->connectButton->setText(tr("断开连接"));
            printDebugInfo(tr("连接: SerialPort(%1)    BandRate(%2)    OK.").arg(selectedPortInfo.portName()).arg(baudRate));
        }
    } else {
        isConnected = 0;
        if (port->isOpen()) {
            port->close();
            ui->connectButton->setText(tr("连接"));
            printDebugInfo(tr("断开连接: SerialPort(%1)    BandRate(%2)").arg(ui->serialNameBox->currentText()).arg(ui->bandRateBox->currentText()));
        }
    }
}

void Widget::on_loadButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("加载配置文件"));
    if (filename.isNull())
        return;
    if (configFile->isOpen()) {
        configFile->close();
    }
    configFile = new QFile(filename);
    if (configFile->open(QIODevice::ReadWrite)) {
        printDebugInfo(tr("打开配置文件: %1").arg(configFile->fileName()));
    } else {
        QMessageBox *box = new QMessageBox(this);
        box->setText("打开文件失败！");
        box->show();
    }
}

void Widget::on_saveButton_clicked()
{
    QString saveFileName("./config.txt");
    if (configFile->fileName() == saveFileName) {
        printDebugInfo(tr("默认配置文件使用中，无需保存！"));
        return;
    }
    QFile *saveFile = new QFile(saveFileName);
    if (saveFile->exists()) {
        saveFile->remove();
    }
    if (configFile->copy(configFile->fileName(), saveFileName)) {
        QMessageBox *box = new QMessageBox(this);
        box->setText(tr("保存成功！"));
        box->show();
    }
}

void Widget::displayCurrentTime()
{
    QString time = QTime::currentTime().toString("hh:mm:ss");
    QString date = QDate::currentDate().toString("yyyy-MM-dd");
    ui->systemTime->setText(date + "  " + time);
}

void Widget::on_serialNameBox_activated(int index)
{
    selectedPortInfo = portInfoList.at(index);
    printDebugInfo(tr("当前选择端口：%1").arg(selectedPortInfo.portName()));
}

void Widget::on_clearButton_clicked()
{
    //ui->lineEdit->clear();
}

void Widget::on_sendButton_clicked()
{
#if 0
    QString context = ui->lineEdit->text();
    const char *string = context.toLatin1().constData();
    int len;
    char hex[20] = {0};
    quint16 nCheck = 0;

    if (port->isOpen()) {
        if (ui->hexRadioButton->isChecked()) {
            len = stringTohex(string, hex);
            if (ui->crcCheckBox->isChecked()) {
                nCheck = crc16_check(hex, len);
                hex[len] = (char)(nCheck >> 8);
                hex[len + 1] = (char)(nCheck & 0xFF);
                len = len + 2;
            }
            printDebugInfo(tr("==========================="));
            for (int i = 0; i < len; i++) {
                printDebugInfo(tr("send: %1").arg(hex[i] & 0xFF, 2, 16));
            }
            printDebugInfo(tr("==========================="));
            port->write(hex, len);
        } else if (ui->stringRadioButton->isChecked()) {
            port->write(string, qstrlen(string));
            printDebugInfo(tr("Send: %1").arg(context));
        }
    } else {
        QMessageBox *box = new QMessageBox(this);
        box->setText(tr("请先连接端口！"));
        box->show();
        printDebugInfo(tr("Send failed!"));
    }
#endif
}

void Widget::parseRecvData(QByteArray array)
{
    int i = 0;
    int len = array.length();
    quint8 data[len];
    quint8 outChar;
    QDataStream out(&array, QIODevice::ReadWrite);

    for (i = 0; !out.atEnd(); i++) {
        out >> outChar;
        data[i] = outChar;
    }
    switch(data[3]) {
        case 0x0:
        {
            char version_h = data[4];
            char version_l = data[5];
            QMessageBox *msg = new QMessageBox(this);
            msg->setText(tr("初始化请求!\nDVR版本号：V%1.%2").arg(data[4]).arg(data[5]));
            //msg->show();
            on_initButton_clicked();
        }
        break;
        case 0x81:
        {
            char accoff = data[4];
            QMessageBox *msg = new QMessageBox(this);
            msg->setText(tr("关机确认!\nDVR已准备好关机！\n关机原因：用户操作关机"));
            msg->show();
        }
        break;
        case 0x02:
        case 0x03:
        {
            switch (data[4]) {
            case 0x01:
            {
                QString string;
                switch (data[5]) {
                case 0x01:
                    string = "正在拍照...";
                    break;
                case 0x02:
                    string = "正在照片回放...";
                    break;
                case 0x03:
                    string = "正在普通录像...";
                    break;
                case 0x04:
                    string = "录像已停止";
                    break;
                case 0x05:
                    string = "正在紧急录像...";
                    break;
                case 0x06:
                    string = "正在播放视频...";
                    break;
                case 0x07:
                    string = "SD卡格式化中...";
                    break;
                case 0x08:
                    string = "正在浏览文件...";
                    break;
                case 0x09:
                    string = "系统升级中...";
                    break;
                case 0x0a:
                    string = "文件修复中...";
                    break;
                }
                ui->DVRStatuslabel->setText(string);
                ui->DVRStatuslabel_2->setText(string);
                break;
            }
            case 0x02:
            {
                QString str1;
                QString str2;
                QString str3;
                QString str4;
                QString str5;
                if (data[5]&0x1) {
                    str1 = "1、未插入SD卡";
                }
                if ((data[5]>>1)&0x1) {
                    str2 = "2、SD卡识别故障";
                }
                if ((data[5] >> 2)&0x1) {
                    str3 = "3、SD卡异常：不能录像";
                }
                if ((data[5] >> 5)&0x1) {
                    str3 = "4、SD卡未格式化";
                }
                if ((data[5] >> 6)&0x1) {
                    str4 = "5、紧急录像文件夹满";
                }
                if ((data[5] >> 7)&0x1) {
                    str5 = "6、照片文件夹满";
                }
                ui->DVRErrorlabel->setText(tr("%1\n%2\n%3\n%4\n%5").arg(str1).arg(str2).arg(str3).arg(str4).arg(str5));
                break;
            }
            case 0x03:
            {
                QString split;
                QString res;
                QString travelSen;
                QString parkSen;
                QString parkMode;
                QString mic;
                switch(data[5] & 0xf) {
                case 0x0:
                    split = "录制长度：1分钟";
                    break;
                case 0x1:
                    split = "录制长度：3分钟";
                    break;
                case 0x2:
                    split = "录制长度：5分钟";
                    break;
                }
                switch((data[5] >> 4)&0xf) {
                case 0x0:
                    res = "录像分辨率：1080P";
                    break;
                case 0x1:
                    res = "录像分辨率：720P";
                    break;
                }
                switch(data[6] & 0x3) {
                case 0x0:
                    travelSen = "行车监控灵敏度：低";
                    break;
                case 0x1:
                    travelSen = "行车监控灵敏度：中";
                    break;
                case 0x2:
                    travelSen = "行车监控灵敏度：高";
                    break;
                }
                switch((data[6] >> 2)&0x3) {
                case 0x0:
                    parkSen = "停车监控灵敏度：低";
                    break;
                case 0x1:
                    parkSen = "停车监控灵敏度：中";
                    break;
                case 0x2:
                    parkSen = "停车监控灵敏度：高";
                    break;
                }
                switch((data[6] >> 4)&0x1) {
                case 0x0:
                    parkMode = "停车监控：关";
                    break;
                case 0x1:
                    parkMode = "停车监控：开";
                    break;
                }
                switch((data[6] >> 5)&0x1) {
                case 0x0:
                    mic = "麦克风：关";
                    break;
                case 0x1:
                    mic = "麦克风：开";
                    break;
                }
                ui->DVRSettinglabel->setText(tr("%1\n%2\n%3\n%4\n%5\n%6").arg(split).arg(res).arg(travelSen)
                                             .arg(parkSen).arg(parkMode).arg(mic));

                break;
            }
            default:
                break;
            }
        }
        break;
        case 0x0d:
        {
                switch (data[5]) {
                case 0x01:
                {

                }
                    break;
                    case 0x06:
                    {
                        switch (data[6]) {
                            case 0x03:
                                ui->lineEdit_2->setText(tr("V%1.%2").arg(data[8]).arg(data[7], 2, 10, QChar('0')));
                                break;
                            case 0x05:
                                ui->TotalSpacelabel->setText(tr("%1G").arg((data[7] + 1) * 8));
                                break;
                        }

                        break;
                    }
                    case 0x09:
                    {
                        switch (data[6]) {
                            case 0x01:
                            ui->PhotoSpacelabel_2->setText(tr("%1M").arg((data[7]<<16)|(data[8]<<8)|data[9]));
                            break;
                        case 0x02:
                            ui->NormalSpacelabel_2->setText(tr("%1M").arg((data[7]<<16)|(data[8]<<8)|data[9]));
                            break;
                        case 0x03:
                            ui->EventSpacelabel_2->setText(tr("%1M").arg((data[7]<<16)|(data[8]<<8)|data[9]));
                            break;
                        }

                        break;
                    }
                }
        }

    }
}

void Widget::SendCarInfo()
{
    char data[13];
    char checkSum = 0;

    if (Speed >= 255) {
        Speed = 0;
    }
    Speed++;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear &0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5) | (HandBrake <<1) | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    checkSum = 0 - (data[2] + data[3] + data[4] + data[5] + data[6] + data[7] + data[8] + data[9] + data[10] + data[11]);
    data[12] = checkSum;
    port->write(data, 13);

}

void Widget::readPortData()
{
    QTextCursor cursor = ui->textEdit->textCursor();

    if (ui->uart_1_Button->isChecked()) {
        QByteArray data = port->readAll();
        parseRecvData(data);
        QDataStream out(&data, QIODevice::ReadWrite);
        printDebugInfo("Recieve: ");
        while (!out.atEnd()) {
            qint8 outChar = 0;
            out >> outChar;
            QString string = QString("%1 ").arg(outChar&0xFF, 2, 16, QLatin1Char('0'));
            ui->textEdit->insertPlainText(string);
        }
    } else {
        char ch;
        while (port->getChar(&ch)) {
            ui->textEdit->insertPlainText(QString("%1").arg(ch));
        }
    }
    cursor.movePosition(QTextCursor::End);
    ui->textEdit->setTextCursor(cursor);
}

void Widget::clearScreen()
{
    ui->textEdit->clear();
}

void Widget::textEditSelectAll()
{
    ui->textEdit->selectAll();
}

void Widget::textEditCopy()
{
    ui->textEdit->copy();
}

void Widget::on_selectAllButton_clicked()
{

    char data[12];
    quint8 select = 0;

    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = (select << 4) | 0x2;//0x12;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_clearAllButton_clicked()
{
    char data[12];
    quint8 select = 0;

    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = (select << 4) | 0x1;//0x12;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_moveSelectButton_clicked()
{
#if 0
    quint16 index = 0;
    quint16 checkSum = 0;
    char data[16];//{0xab, 0xba, 0x90, 0xf, 0x08, 0x0, 0x0, 0x50, 0x0, 0x0, 0x0, 0x0, 0x0};
    data[0] = 0xab;
    data[1] = 0xba;
    data[2] = 0x90;
    data[3] = 0xf;
    data[4] = 0x08;
    data[5] = 0x0;
    data[6] = 0x0;
    data[7] = 0x50;

    if (ui->checkBox_1->isChecked()) {
        index |= (1 << 3);
    }
    if (ui->checkBox_2->isChecked())
        index |= (1 << 4);
    if (ui->checkBox_3->isChecked())
        index |= (1 << 5);
    if (ui->checkBox_4->isChecked())
        index |= (1 << 6);
    if (ui->checkBox_5->isChecked())
        index |= (1 << 7);
    if (ui->checkBox_6->isChecked())
        index |= (1 << 8);
    if (ui->checkBox_7->isChecked())
        index |= (1 << 9);
    if (ui->checkBox_8->isChecked())
        index |= (1 << 10);
    if (ui->checkBox_9->isChecked())
        index |= (1 << 11);

    data[8] = (char)(index >> 8);
    data[9] = (char)(index & 0xff);
    data[10] = 0x0;
    data[11] = 0x0;
    data[12] = 0x0;

    checkSum = crc16_check(data, 13);
    data[13] = char(checkSum >> 8);
    data[14] = char(checkSum & 0xff);
    if (port->isOpen())
        port->write(data, 15);
    printDebugInfo(tr("Select index: 0x%1").arg(index, 0, 16));
#endif
    char data[13];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x09;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[12] = 0x00;

    port->write(data, 13);
    on_clearAllButton_clicked();
}

void Widget::on_deleteSelectButton_clicked()
{
    //printDebugInfo(tr("Select index: 0x%1").arg(index, 0, 16));
    char data[13];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x08;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[12] = 0x00;
    port->write(data, 13);
    on_clearAllButton_clicked();
}

void Widget::on_eventRecordButton_clicked()
{
    //writeButtonData("eventRecord");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x1;
    data[6] = 0x1d;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 紧急录像").arg(autoTestCount));
    }
}

void Widget::on_startRecordButton_clicked()
{
    //writeButtonData("startRecord");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x08;
    data[6] = 0x1;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 普通录像").arg(autoTestCount));
    }
}

void Widget::autoStartRecord()
{
    static int count = 0;

    if (count == 0)
        on_startRecordButton_clicked();
    count = 1;


}


void Widget::on_stopRecordButton_clicked()
{
    //writeButtonData("stopRecord");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x08;
    data[6] = 0x00;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 停止录像").arg(autoTestCount));
    }
}

void Widget::on_captureButton_clicked()
{
    //writeButtonData("capture");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x1;
    data[6] = 0x12;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 拍照").arg(autoTestCount));
    }
}

void Widget::on_oneMinButton_clicked()
{
    //writeButtonData("split_1min");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x0B;
    data[6] = 0x01;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 1分钟").arg(autoTestCount));
    }

}

void Widget::on_threeMinButton_clicked()
{
    //writeButtonData("split_3min");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x0B;
    data[6] = 0x02;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 3分钟").arg(autoTestCount));
    }

}

void Widget::on_fiveMinButton_clicked()
{
    //writeButtonData("split_5min");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x0B;
    data[6] = 0x03;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 5分钟").arg(autoTestCount));
    }

}

void Widget::on_micOnButton_clicked()
{
    //writeButtonData("mic_on");
    //writeButtonData("clear");

    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x05;
    data[6] = 0x01;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. MIC开").arg(autoTestCount));
    }

}

void Widget::on_micOffButton_clicked()
{
    //writeButtonData("mic_off");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x05;
    data[6] = 0x00;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. MIC关").arg(autoTestCount));
    }

}

void Widget::on_res1080PButton_clicked()
{
    //writeButtonData("res_1080p");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x02;
    data[6] = 0x01;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 1080P").arg(autoTestCount));
    }

}

void Widget::on_res1080PHDRButton_clicked()
{
    //writeButtonData("res_1080p_hdr");
    //writeButtonData("clear");

}

void Widget::on_res720PButton_clicked()
{
    //writeButtonData("res_720p");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x02;
    data[6] = 0x02;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 720P").arg(autoTestCount));
    }


}

void Widget::on_res720PHDRButton_clicked()
{
    writeButtonData("res_720p_hdr");
    writeButtonData("clear");

}

void Widget::on_setSystemTimeButton_clicked()
{
#if 0
    char systemTime[15] = {0};//{0xab, 0xba, 0x02, 0x0e, 0x07, 0x20, 0x17, 0x1, 0x1, 0x0, 0x0, 0x0};
    quint16 checkSum = 0;

    systemTime[0] = 0x5a;
    systemTime[1] = 0xa5;
    systemTime[2] = 0x02;
    systemTime[3] = 0x0e;
    systemTime[4] = 0x07;

    QDate currentDate = QDate::currentDate();
    systemTime[5] = 0x20;

    quint8 year = currentDate.year() - 2000;
    systemTime[6] = char(((year / 10) << 4) | ((year % 10) & 0xF));
    quint8 month = currentDate.month();
    systemTime[7] = char((month / 10) << 4 | ((month % 10) & 0xF));

    quint8 day = currentDate.day();
    systemTime[8] = char((day / 10) << 4 | ((day % 10) & 0xF));

    QTime currentTime = QTime::currentTime();
    quint8 hour = currentTime.hour();
    systemTime[9] = char((hour / 10) << 4 | ((hour % 10) & 0xF));

    quint8 min = currentTime.minute();
    systemTime[10] = char((min / 10) << 4 | ((min % 10) & 0xF));

    quint8 sec = currentTime.second();
    systemTime[11] = char((sec / 10) << 4 | ((sec % 10) & 0xF));

    checkSum = crc16_check(systemTime, 12);
    systemTime[12] = char(checkSum >> 8);
    systemTime[13] = char(checkSum & 0xFF);
    if (port->isOpen())
        port->write(systemTime, 14);
    printDebugInfo(tr("设置DVR系统时间: 20%1-%2-%3 %4:%5:%6").arg(year).arg(month, 2, 10, QChar('0')).arg(day, 2, 10, QChar('0'))
                   .arg(hour, 2, 10, QChar('0')).arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')));

    writeButtonData("clear");
#endif
    char data[16];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0d;
    data[3] = 0x80;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x00;
    QTime currentTime = QTime::currentTime();
    data[7] = currentTime.hour();
    data[8] = currentTime.minute();
    data[9] = currentTime.second();
    QDate currentDate = QDate::currentDate();
    data[10] = (currentDate.year() >> 8) & 0xFF;
    data[11] = currentDate.year() & 0xFF;
    data[12] = currentDate.month();
    data[13] = currentDate.day();
    data[14] = 0x00;
    data[15] = 0x00;
    port->write(data, 16);
    if (recordFlag) {
        setRecordData(data, 16);
        ui->textEdit_2->append(tr("%1. 设置系统时间").arg(autoTestCount));
    }
}

void Widget::on_formatButton_clicked()
{
    //writeButtonData("format_card");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x6;
    data[6] = 0x01;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 格式化").arg(autoTestCount));
    }
}

void Widget::on_loadDefButton_clicked()
{
    //writeButtonData("load_def");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x6;
    data[6] = 0x04;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 恢复出厂设置").arg(autoTestCount));
    }

}

void Widget::on_rebootButton_clicked()
{
    //writeButtonData("mcu_reset");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x6;
    data[6] = 0x02;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 重启").arg(autoTestCount));
    }

}

void Widget::on_powerOffButton_clicked()
{
    //writeButtonData("power_off");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x1;
    data[6] = 0x14;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);

}

void Widget::on_normalButton_clicked()
{
    //writeButtonData("normalBrowse");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x16;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 普通目录").arg(autoTestCount));
    }
}

void Widget::on_eventButton_clicked()
{
    //writeButtonData("eventBrowse");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x1;
    data[6] = 0x15;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 紧急目录").arg(autoTestCount));
    }
}

void Widget::on_photoButton_clicked()
{
    //writeButtonData("photoBrowse");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x1;
    data[6] = 0x12;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 照片目录").arg(autoTestCount));
    }

}

void Widget::on_exitBrowseButton_clicked()
{

    //writeButtonData("exit_browse");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x1A;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 进入编辑").arg(autoTestCount));
    }
}

void Widget::on_prevPageButton_clicked()
{

    //writeButtonData("key_prev");
    //writeButtonData("clear");
    //5a a5 09 8d 00 02 00 00 00 00 00 00
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x02;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 上一页").arg(autoTestCount));
    }
}

void Widget::on_nextPageButton_clicked()
{

   // writeButtonData("key_next");
    //writeButtonData("clear");
    //5a a5 09 8d 00 01 00 00 00 00 00 00
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x01;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 下一页").arg(autoTestCount));
    }
}

void Widget::on_upButton_clicked()
{

    //writeButtonData("key_up");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x03;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 上一个").arg(autoTestCount));
    }
}

void Widget::on_downButton_clicked()
{

    //writeButtonData("key_down");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x04;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 下一个").arg(autoTestCount));
    }
}

void Widget::on_enterPlayButton_clicked()
{

    //writeButtonData("item1");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x05;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 播放").arg(autoTestCount));
    }
}

void Widget::on_moveCurrentButton_clicked()
{

    //writeButtonData("mv_t_emergency");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x07;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 移动当前文件").arg(autoTestCount));
    }
}

void Widget::on_deleteCurrentButton_clicked()
{
    //writeButtonData("Del_file");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x06;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 删除当前文件").arg(autoTestCount));
    }

}

void Widget::on_deleteAllButton_clicked()
{
    //writeButtonData("Del_all");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x0a;
    data[6] = 0x01;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 删除所有文件").arg(autoTestCount));
    }
}

void Widget::on_item_1_Button_clicked()
{
    //writeButtonData("item1");
    //writeButtonData("clear");

}

void Widget::on_item_2_Button_clicked()
{

    //writeButtonData("item2");
    //writeButtonData("clear");
}

void Widget::on_item_3_Button_clicked()
{
    //writeButtonData("item3");
    //writeButtonData("clear");

}

void Widget::on_item_4_Button_clicked()
{

    //writeButtonData("item4");
    //writeButtonData("clear");
}

void Widget::on_item_5_Button_clicked()
{
    //writeButtonData("item5");
    //writeButtonData("clear");

}

void Widget::on_item_6_Button_clicked()
{
    //writeButtonData("item6");
    //writeButtonData("clear");

}

void Widget::on_item_7_Button_clicked()
{
    //writeButtonData("item7");
    //writeButtonData("clear");

}

void Widget::on_item_8_Button_clicked()
{
    //writeButtonData("item8");
    //writeButtonData("clear");

}

void Widget::on_item_9_Button_clicked()
{
    //writeButtonData("item9");
    //writeButtonData("clear");

}

void Widget::on_playPrevButton_clicked()
{
    //writeButtonData("play_prev");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x0c;
    data[6] = 0x00;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 播放上一个").arg(autoTestCount));
    }

}

void Widget::on_playNextButton_clicked()
{
    //writeButtonData("play_next");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x0d;
    data[6] = 0x00;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 播放下一个").arg(autoTestCount));
    }

}

void Widget::on_pauseButton_clicked()
{
    //writeButtonData("pause");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x0b;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 暂停播放").arg(autoTestCount));
    }

}

void Widget::on_playButton_clicked()
{
    //writeButtonData("play");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x05;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 开始播放").arg(autoTestCount));
    }

}

void Widget::on_exitPlayButton_clicked()
{
    //writeButtonData("exit_play");
    //writeButtonData("clear");
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x0e;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 退出播放").arg(autoTestCount));
    }

}

void Widget::on_dvrVersionButton_clicked()
{
    //writeButtonData("init");
}

void Widget::on_initButton_clicked()
{
    char data[17];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0E;
    data[3] = 0x80;
    data[4] = 0x12;
    data[5] = 0x03;//power on reason
    data[6] = 0x00;//car type
    data[7] = 0x0a;
    QTime currentTime = QTime::currentTime();
    data[8] = currentTime.hour();
    data[9] = currentTime.minute();
    data[10] = currentTime.second();
    QDate currentDate = QDate::currentDate();
    data[11] = (currentDate.year() >> 8) & 0xFF;
    data[12] = currentDate.year() & 0xFF;
    data[13] = currentDate.month();
    data[14] = currentDate.day();
    data[15] = 0x00;
    data[16] = 0x00;
    QMessageBox *message = new QMessageBox(this);
    message->setText(tr("初始化：\nMCU版本号：%1\n开机原因：%2\n日期：%3/%4/%5\n时间：%6:%7:%8\n时区：%9").arg(0)
            .arg(1).arg(currentDate.year()).arg(currentDate.month(), 2, 10, QChar('0')).arg(currentDate.day(), 2, 10, QChar('0'))
            .arg(currentTime.hour(), 2, 10, QChar('0')).arg(currentTime.minute(), 2, 10, QChar('0')).arg(currentTime.second(), 2, 10, QChar('0'))
            .arg(0));
    //message->show();
    port->write(data, 17);




}

void Widget::on_checkBox_1_clicked()
{
#if 1
   // 5a a5 09 8d 00 18 00 01 00 00 00 00
    char data[12];
    quint8 select = 0;

    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = (select << 4) | 0x2;//0x12;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
#endif
}

void Widget::on_checkBox_2_clicked()
{
#if 0
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = 0x22;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
#endif
}

void Widget::on_checkBox_3_clicked()
{
#if 0
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = 0x32;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
#endif
}

void Widget::on_checkBox_6_clicked()
{
#if 0
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = 0x42;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;

    port->write(data, 12);
#endif
}

void Widget::on_checkBox_5_clicked()
{
#if 0
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = 0x52;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;

    port->write(data, 12);
#endif
}

void Widget::on_checkBox_4_clicked()
{
#if 0
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = 0x62;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;

    port->write(data, 12);
#endif
}

void Widget::on_fastForwardButton_clicked()
{
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x0f;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 快进").arg(autoTestCount));
    }

}

void Widget::on_fastBackwardButton_clicked()
{

    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x1;
    data[6] = 0x10;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 慢放").arg(autoTestCount));
    }
}

void Widget::on_playLoopButton_clicked()
{

    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x11;
    data[6] = 0x00;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_pushButton_2_clicked()
{
    char data[12];

    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x3;
    data[6] = 0x2;//0x12;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);

}

void Widget::on_pushButton_3_clicked()
{

    char data[12];

    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x3;
    data[6] = 0x3;//0x12;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_pushButton_4_clicked()
{

    char data[12];

    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x0a;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_pushButton_7_clicked()
{
    char data[12];
    static quint8 t = 0;

    if (t == 0x1) {
        t = 0x2;
    } else {
        t = 0x1;
    }

    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = 0x42;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);

}

void Widget::on_pushButton_8_clicked()
{

    char data[12];


    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = 0x52;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_pushButton_9_clicked()
{

    char data[12];


    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = 0x62;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_pushButton_5_clicked()
{

}

void Widget::on_pushButton_6_clicked()
{

}

void Widget::on_DVRstateButton_clicked()
{
    char data[7];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x04;
    data[3] = 0x82;
    data[4] = 0x01;
    data[5] = 0x00;
    data[6] = 0x00;
    port->write(data, 12);

}

void Widget::on_DVRsErrorButton_clicked()
{
    char data[7];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x04;
    data[3] = 0x82;
    data[4] = 0x02;
    data[5] = 0x00;
    data[6] = 0x00;
    port->write(data, 12);

}

void Widget::on_DVRSetupButton_clicked()
{

    char data[7];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x04;
    data[3] = 0x82;
    data[4] = 0x03;
    data[5] = 0x00;
    data[6] = 0x00;
    port->write(data, 12);
}

void Widget::on_DVRSetupButton_2_clicked()
{

    char data[7];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x04;
    data[3] = 0x82;
    data[4] = 0x03;
    data[5] = 0x00;
    data[6] = 0x00;
    port->write(data, 12);
}

void Widget::on_NorSpaceButton_clicked()
{

    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x9;
    data[6] = 0x02;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_pushButton_11_clicked()
{

    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x9;
    data[6] = 0x03;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_pushButton_10_clicked()
{


    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x9;
    data[6] = 0x01;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_TotalSpaceButton_clicked()
{

    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x6;
    data[6] = 0x05;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_leftButton_clicked()
{

    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x0c;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_rightButton_clicked()
{

    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x0d;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_speedAddButton_clicked()
{
    char data[13];

    if (Speed < 255)
        Speed++;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear &0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5) | (HandBrake <<1) | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);
}

void Widget::on_speedClearButton_clicked()
{
    char data[13];

    Speed = 0;
    Gear = 0;
    TurnLeft = 0;
    TurnRight = 0;
    Acc = 0;
    Youmen = 0;
    Brake = 0;
    HandBrake = 0;
    Yuan = 0;
    Jin = 0;
    Fog = 0;
    Postion = 0;
    SeatBelt = 0;
    ARS = 0;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x0;
    data[6] = 0x0;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    data[12] = 0x00;
    port->write(data, 13);

}

void Widget::on_speedDecButton_clicked()
{
    char data[13];

    if (Speed > 0)
        Speed--;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear &0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5)  | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);
}

void Widget::on_pushButton_12_clicked()
{
    char data[13];

    Gear = Gear >> 2;
    if (Gear < 4)
        Gear++;
    else
        Gear = 0;
    Gear = Gear << 2 | 1;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear &0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5)  | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);

}

void Widget::on_pushButton_13_clicked()
{
    char data[13];

    TurnLeft ^= 0x1;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear&0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5)  | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;

    port->write(data, 13);
}

void Widget::on_pushButton_14_clicked()
{
    char data[13];

    TurnRight ^= 0x1;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear&0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5)  | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);

}

void Widget::on_pushButton_15_clicked()
{
    char data[13];

    if (Youmen <= 100)
    Youmen++;

    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear&0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5)  | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);

}

void Widget::on_pushButton_16_clicked()
{
    char data[13];

    Brake ^= 0x1;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear&0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5)  | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);

}

void Widget::on_pushButton_17_clicked()
{
    char data[13];

    HandBrake ^= 0x1;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear &0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5) | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);

}

void Widget::on_pushButton_18_clicked()
{

    char data[13];

    Yuan ^= 0x1;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear &0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5) | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);
}

void Widget::on_pushButton_19_clicked()
{

    char data[13];

    Jin ^= 0x1;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear &0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5)  | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);
}

void Widget::on_pushButton_20_clicked()
{

    char data[13];

    Fog ^= 0x1;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear &0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5) | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);
}

void Widget::on_pushButton_21_clicked()
{

    char data[13];

    Postion ^= 0x1;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear &0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5) | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);
}

void Widget::on_pushButton_22_clicked()
{

    char data[13];

    SeatBelt ^= 0x1;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear &0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5)  | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);
}

void Widget::on_pushButton_23_clicked()
{

    char data[13];

    ARS = 0x1;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear &0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5) | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);
}

void Widget::on_pushButton_24_clicked()
{
    char data[13];

    if (Youmen > 0)
        Youmen--;
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x0a;
    data[3] = 0x84;
    data[4] = 0x02;
    data[5] = 0x1;
    data[6] = Speed & 0xFF;
    data[7] = Gear &0xff;
    data[8] = 0x1;
    data[9] = Youmen;
    data[10] = (Jin << 5) | (TurnLeft << 2) | (TurnRight<<3) | (Yuan << 4) | (Postion << 1) | (Fog << 6);
    data[11] = (ARS << 1) | (SeatBelt << 2) | (Brake << 4) | (HandBrake << 6);
    data[12] = 0x00;
    port->write(data, 13);

}

void Widget::on_pushButton_27_clicked()
{
    connect(carInfoTimer, SIGNAL(timeout()), this, SLOT(SendCarInfo()));
    carInfoTimer->start(10);
}

void Widget::on_pushButton_28_clicked()
{
    carInfoTimer->stop();
}


void Widget::on_deleteAllButton_2_clicked()
{

    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x0a;
    data[6] = 0x02;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_deleteAllButton_3_clicked()
{

    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x0a;
    data[6] = 0x03;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_pushButton_25_clicked()
{
    char data[12];

    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = 0x22;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);

}

void Widget::autoReqDvrStatus()
{
    static int count = 0;

    switch (count) {
        case 0:
            on_DVRstateButton_clicked();
            break;
        case 1:
            on_DVRsErrorButton_clicked();
            break;
        case 2:
            on_DVRSetupButton_clicked();
            break;
        case 3:
            on_TotalSpaceButton_clicked();
            break;
        case 4:
            on_pushButton_10_clicked();
            break;
        case 5:
            on_NorSpaceButton_clicked();
            break;
        case 6:
            on_pushButton_11_clicked();
            break;
    }
    count++;
    if (count == 7)
        count = 0;

}

void Widget::on_pushButton_26_clicked()
{
    static int initFlag = 0;
    bool ok;

    if (initFlag == 0) {
        initFlag = 1;
        autoTestTimer = new QTimer(this);
        connect(autoTestTimer, SIGNAL(timeout()), this, SLOT(autoTest()));
    }
    if (autoTestTimer->isActive()) {
        ui->pushButton_26->setText(tr("开始"));
        autoTestTimer->stop();
    } else if (recordList.count()){
        ui->pushButton_26->setText(tr("停止"));
        autoTestTimer->start(ui->lineEdit->text().toInt(&ok, 10));
    }
}

void Widget::on_pushButton_29_clicked()
{
    if (recordFlag == 0) {
        recordFlag = 1;
        autoTestCount = 0;
        ui->pushButton_29->setText(tr("停止宏录制"));
    } else {
        recordFlag = 0;
        ui->pushButton_29->setText(tr("开启宏录制"));
    }
}

void Widget::autoTest()
{
    int num = recordList.count();
    static int i = 0;

    QByteArray array = recordList.at(i);
    port->write(array);
    i++;
    if (i == num) {
        i = 0;
    }




}

void Widget::setRecordData(char *data, int size)
{
    QByteArray array(data, size);
    recordList.insert(autoTestCount, array);
    autoTestCount++;

}


void Widget::on_pushButton_30_clicked()
{
    ui->textEdit_2->clear();
    autoTestCount = 0;
    recordList.clear();
}

void Widget::on_pushButton_ver_clicked()
{
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x6;
    data[6] = 0x03;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);

}

void Widget::on_pushButton_1_clicked()
{
    char data[12];

    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = (1 << 4) | 0x2;//0x12;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_pushButton_31_clicked()
{
    char data[12];

    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = 0x12;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_pushButton_32_clicked()
{
    char data[12];


    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x4;
    data[6] = 0x32;
    data[7] = 0x0;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}


void Widget::on_moveSelectButton_2_clicked()
{
    char data[13];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x018;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[12] = 0x00;

    port->write(data, 13);
}

void Widget::on_exitBrowseButton_2_clicked()
{

    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x19;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 进入编辑").arg(autoTestCount));
    }
}

void Widget::on_pushButton_33_clicked()
{
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x1B;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 全选").arg(autoTestCount));
    }

}

void Widget::on_pushButton_34_clicked()
{
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x1C;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
    if (recordFlag) {
        setRecordData(data, 12);
        ui->textEdit_2->append(tr("%1. 取消").arg(autoTestCount));
    }
}

void Widget::on_moveCurrentButton_2_clicked()
{
    char data[12];
    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x09;
    data[3] = 0x8d;
    data[4] = 0x00;
    data[5] = 0x01;
    data[6] = 0x17;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    port->write(data, 12);
}

void Widget::on_pushButton_35_clicked()
{
    char data[10];
    static int t = 0;
    t++;

    data[0] = 0x5a;
    data[1] = 0xa5;
    data[2] = 0x07;
    data[3] = 0x84;
    data[4] = 0x01;
    data[5] = 0x02;
    data[6] = 0x0c;
    data[7] = 0x01;
    data[8] = t;

    data[9] = 0x00;
    port->write(data, 10);
}
