#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QFile>
#include <QTimer>
#include <QTime>
#include <QProcess>

#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>

#include<windows.h>

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

//============================================================================================================================================
class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_pushButton_selectFile_clicked();
    void slotUpdateDateTime();

    void on_pushButton_Flash_clicked();

    void on_comboBox_page_currentIndexChanged(int index);

    void slotDataOnStdout();

private:
    Ui::Dialog *ui;
    QImage *frame;

    QTimer clock_timer;
    int mstime = 0;

    QFile datafile;
    QByteArray FramesData;
    int Size;

    HANDLE hComm;
    bool OpenSerialPort(int baud);
    int m_waitTimeout = 30;
    int m_readTimeout = 10;
    int m_wtiteTimeout = 10;
    char *buf_out;
    char *buf_in;
    DWORD         bc;
    unsigned char fault_cnt = 0;

    void LoadFrame(int index, int label);

    void FlashErase();
    void FlashConfigData();
    void FlashData();
    int anim_page = 0;

    QByteArray Ok_str{"\x4F\x6B"};
    QByteArray ID_str{"\x21\xAA\xEF"};

    QProcess* m_proc;

protected:
    virtual void paintEvent(QPaintEvent *event);


};
//============================================================================================================================================

#endif // DIALOG_H
