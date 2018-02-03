#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QFile>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QContextMenuEvent>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    void printDebugInfo(QString info);
    char charTohex(char ch);
    int stringTohex(const char *str, char *buf);
    quint16 crc16_check(char *data, int len);
    void writeButtonData(char *buttonName);
    char DecToHex(int num);
    void parseRecvData(QByteArray array);

private slots:
    void SendCarInfo();
    void on_pushButton_clicked();
    void on_connectButton_clicked();

    void on_loadButton_clicked();

    void on_saveButton_clicked();
    void displayCurrentTime();

    void on_serialNameBox_activated(int index);

    void on_clearButton_clicked();

    void on_sendButton_clicked();
    void readPortData();
    void clearScreen();
    void textEditSelectAll();
    void textEditCopy();

    void on_selectAllButton_clicked();

    void on_clearAllButton_clicked();

    void on_moveSelectButton_clicked();

    void on_deleteSelectButton_clicked();

    void on_eventRecordButton_clicked();

    void on_startRecordButton_clicked();

    void autoStartRecord();
    void on_stopRecordButton_clicked();

    void on_captureButton_clicked();

    void on_oneMinButton_clicked();

    void on_threeMinButton_clicked();

    void on_fiveMinButton_clicked();

    void on_micOnButton_clicked();

    void on_micOffButton_clicked();

    void on_res1080PButton_clicked();

    void on_res1080PHDRButton_clicked();

    void on_res720PButton_clicked();

    void on_res720PHDRButton_clicked();

    void on_setSystemTimeButton_clicked();

    void on_formatButton_clicked();

    void on_loadDefButton_clicked();

    void on_rebootButton_clicked();

    void on_powerOffButton_clicked();

    void on_normalButton_clicked();

    void on_eventButton_clicked();

    void on_photoButton_clicked();

    void on_exitBrowseButton_clicked();

    void on_prevPageButton_clicked();

    void on_nextPageButton_clicked();

    void on_upButton_clicked();

    void on_downButton_clicked();

    void on_enterPlayButton_clicked();

    void on_moveCurrentButton_clicked();

    void on_deleteCurrentButton_clicked();

    void on_deleteAllButton_clicked();

    void on_item_1_Button_clicked();

    void on_item_2_Button_clicked();

    void on_item_3_Button_clicked();

    void on_item_4_Button_clicked();

    void on_item_5_Button_clicked();

    void on_item_6_Button_clicked();

    void on_item_7_Button_clicked();

    void on_item_8_Button_clicked();

    void on_item_9_Button_clicked();

    void on_playPrevButton_clicked();

    void on_playNextButton_clicked();

    void on_pauseButton_clicked();

    void on_playButton_clicked();

    void on_exitPlayButton_clicked();

    void on_dvrVersionButton_clicked();

    void on_initButton_clicked();

    void on_checkBox_1_clicked();

    void on_checkBox_2_clicked();

    void on_checkBox_3_clicked();

    void on_checkBox_6_clicked();

    void on_checkBox_5_clicked();

    void on_checkBox_4_clicked();

    void on_fastForwardButton_clicked();

    void on_fastBackwardButton_clicked();

    void on_playLoopButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_7_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_9_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_6_clicked();

    void on_DVRstateButton_clicked();

    void on_DVRsErrorButton_clicked();

    void on_DVRSetupButton_clicked();

    void on_DVRSetupButton_2_clicked();

    void on_NorSpaceButton_clicked();

    void on_pushButton_11_clicked();

    void on_pushButton_10_clicked();

    void on_TotalSpaceButton_clicked();

    void on_leftButton_clicked();

    void on_rightButton_clicked();

    void on_speedAddButton_clicked();

    void on_speedClearButton_clicked();

    void on_speedDecButton_clicked();

    void on_pushButton_12_clicked();

    void on_pushButton_13_clicked();

    void on_pushButton_14_clicked();

    void on_pushButton_15_clicked();

    void on_pushButton_16_clicked();

    void on_pushButton_17_clicked();

    void on_pushButton_18_clicked();

    void on_pushButton_19_clicked();

    void on_pushButton_20_clicked();

    void on_pushButton_21_clicked();

    void on_pushButton_22_clicked();

    void on_pushButton_23_clicked();

    void on_pushButton_24_clicked();

    void on_pushButton_27_clicked();

    void on_pushButton_28_clicked();



    void on_deleteAllButton_2_clicked();

    void on_deleteAllButton_3_clicked();

    void on_pushButton_25_clicked();
    void autoReqDvrStatus();

    void on_pushButton_26_clicked();

    void on_pushButton_29_clicked();
    void autoTest();
    void setRecordData(char *data, int size);

    void on_pushButton_30_clicked();

    void on_pushButton_ver_clicked();

    void on_pushButton_1_clicked();

    void on_pushButton_31_clicked();

    void on_pushButton_32_clicked();

    void on_moveSelectButton_2_clicked();

    void on_exitBrowseButton_2_clicked();

    void on_pushButton_33_clicked();

    void on_pushButton_34_clicked();

    void on_moveCurrentButton_2_clicked();

    void on_pushButton_35_clicked();

private:
    Ui::Widget *ui;
    QFile *configFile;
    int isConnected;
    QSerialPort *port;
    QSerialPortInfo selectedPortInfo;
    QList<QSerialPortInfo> portInfoList;
    quint16 Speed;
    quint8 Gear;
    quint8 TurnLeft;
    quint8 TurnRight;
    quint8 Acc;
    quint8 Youmen;
    quint8 Brake;
    quint8 HandBrake;
    quint8 Yuan;
    quint8 Jin;
    quint8 Fog;
    quint8 Postion;
    quint8 SeatBelt;
    quint8 ARS;
    QTimer *carInfoTimer;
    int recordFlag;
    int autoTestCount;
    QTimer *autoTestTimer;
    QTimer *dvrStatusTimer;
    QList<QObject> objectList;
    QList<QByteArray> recordList;
};

#endif // WIDGET_H
