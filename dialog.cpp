#include <QFileDialog>
#include <QTime>
#include <QCryptographicHash>

#include "dialog.h"
#include "ui_dialog.h"

//============================================================================================================================================
Dialog::Dialog(QWidget *parent) : QDialog(parent), ui(new Ui::Dialog)
{
    ui->setupUi(this);

    frame = new QImage[8];
    for (int i = 0; i < 8; i++)
    {
        frame[i] = QImage(135, 240, QImage::Format_RGB32);
        frame[i].fill(Qt::black);
    }

    ui->label_SerialPort->setVisible(false);
    ui->comboBox_SerialPort->setVisible(false);
    ui->progressBar->setVisible(false);
    ui->pushButton_Flash->setVisible(false);
    ui->comboBox_page->setVisible(false);

    const auto infos = QSerialPortInfo::availablePorts();
    int portnum = 0;
    int i = 0;
    for (const QSerialPortInfo &info : infos)
    {
        i++;
        ui->comboBox_SerialPort->addItem(info.portName());
        if(info.vendorIdentifier() == 6790 && info.productIdentifier() == 29987 )
        {
            portnum = i-1;
        }
    }
    ui->comboBox_SerialPort->setCurrentIndex(portnum);

    QStringList ComboSimvols;
    ComboSimvols << "anim_Skin_1" << "anim_Skin_2" << "anim_Skin_3";
    ui->comboBox_page->addItems(ComboSimvols);


   connect(&clock_timer, SIGNAL(timeout()), SLOT(slotUpdateDateTime()));

   m_proc = new QProcess(this);
   connect(m_proc, SIGNAL(readyReadStandardOutput()), SLOT(slotDataOnStdout()));
}

//============================================================================================================================================
Dialog::~Dialog()
{
    delete frame;

    delete ui;
}

//============================================================================================================================================
void Dialog::paintEvent(QPaintEvent *event)
{
     ui->label_1->setPixmap(QPixmap::fromImage(frame[0],Qt::NoFormatConversion));
     ui->label_2->setPixmap(QPixmap::fromImage(frame[1],Qt::NoFormatConversion));
     ui->label_3->setPixmap(QPixmap::fromImage(frame[2],Qt::NoFormatConversion));
     ui->label_4->setPixmap(QPixmap::fromImage(frame[3],Qt::NoFormatConversion));
     ui->label_5->setPixmap(QPixmap::fromImage(frame[4],Qt::NoFormatConversion));
     ui->label_6->setPixmap(QPixmap::fromImage(frame[5],Qt::NoFormatConversion));
     ui->label_7->setPixmap(QPixmap::fromImage(frame[6],Qt::NoFormatConversion));
     ui->label_8->setPixmap(QPixmap::fromImage(frame[7],Qt::NoFormatConversion));
}

//============================================================================================================================================
void Dialog::on_pushButton_selectFile_clicked()
{
    QString FileName = QFileDialog::getOpenFileName(0, "Открыть файл", "", "*.bin");
    datafile.setFileName(FileName);

    if ( !FileName.isEmpty())
    {
        if (!datafile.open(QIODevice::ReadOnly))
        {
            ui->textEdit_Log->append(QTime::currentTime().toString()+" Ошибка открытия");
            return;
        }
        ui->lineEdit_selectFile->setText(FileName);

        clock_timer.stop();

        FramesData.clear();
        FramesData = datafile.readAll();

        datafile.close();

        Size = FramesData.size() / (64800 * 30); // [15/17] anim * 30 fps * 64800 bytes

        if (!((Size == 15) || (Size == 17)))
        {
            FramesData.clear();
            ui->textEdit_Log->append(QTime::currentTime().toString()+" Некорректный размер файла");
            
            return;
        }

        // Set initial time
        QTime local_time = QTime::currentTime();

        int hoursTen = local_time.hour() / 10;
        int hoursUnit = local_time.hour() % 10;

        int minTen = local_time.minute() / 10;
        int minUnit = local_time.minute() % 10;

        int secUnit = local_time.second() % 10;
        int secTen = local_time.second() / 10 ;

        LoadFrame(hoursTen * 30, 0);
        LoadFrame(hoursUnit * 30, 1);
        LoadFrame(13 * 30, 2);  //dots
        LoadFrame(minTen * 30, 3);
        LoadFrame(minUnit * 30, 4);
        if (Size == 15)
        {
            LoadFrame(14 * 30, 5);  //dots short
        }
        else
        {
            LoadFrame(15 * 30, 5);  //dots long
        }
        LoadFrame(secTen * 30, 6);
        LoadFrame(secUnit * 30, 7);

        ui->label_SerialPort->setVisible(true);
        ui->comboBox_SerialPort->setVisible(true);
        ui->pushButton_Flash->setVisible(true);
        ui->comboBox_page->setVisible(true);

        // Start animation
        clock_timer.start(15);
    }
    else
    {
        ui->textEdit_Log->append(QTime::currentTime().toString()+" Файл не выбран");
        
        ui->label_SerialPort->setVisible(false);
        ui->comboBox_SerialPort->setVisible(false);
        ui->progressBar->setVisible(false);
        ui->pushButton_Flash->setVisible(false);
        ui->comboBox_page->setVisible(false);

        ui->lineEdit_selectFile->clear();

        FramesData.clear();
        clock_timer.stop();
    }
}
//============================================================================================================================================
void  Dialog::LoadFrame(int index, int label)
{
    // 135 pix x 240 pix x 2 byte = 64800 bytes один кадр

    for(int j = 0; j < 240; j++)
    {
        for (int i = 0; i < 135; i++)
        {
            uchar r5 = (FramesData.at(index * 64800 + j * 135 * 2 + i * 2 + 0) & 0xF8) >> 3;
            uchar g6 = (FramesData.at(index * 64800 + j * 135 * 2 + i * 2 + 0) & 0x07) << 3 |
                       (FramesData.at(index * 64800 + j * 135 * 2 + i * 2 + 1) & 0xE0) >> 5;
            uchar b5 = (FramesData.at(index * 64800 + j * 135 * 2 + i * 2 + 1) & 0x1F);

            uchar r8 = (uchar)( (float) r5 * 255.0f / 31.0f + 0.5f );
            uchar g8 = (uchar)( (float) g6 * 255.0f / 63.0f + 0.5f );
            uchar b8 = (uchar)( (float) b5 * 255.0f / 31.0f + 0.5f );

            frame[label].setPixelColor(i, j, QColor(r8, g8, b8));
        }
    }
}

//============================================================================================================================================
void Dialog::slotUpdateDateTime()
{
    QTime local_time = QTime::currentTime();

    if (mstime != local_time.msec() / 33.33)
    {
        mstime = local_time.msec() / 33.33;

        int secUnit = local_time.second() % 10;
        LoadFrame(secUnit * 30 + mstime, 7); // Sec0

        if (Size == 15)
        {
            LoadFrame(13 * 30 + mstime, 2); // dots
            LoadFrame(14 * 30 + mstime, 5); // dots
        }
        else
        {
            LoadFrame(13 * 30 + (local_time.second() % 2) * 30 + mstime, 2); // dots
            LoadFrame(15 * 30 + (local_time.second() % 2) * 30 + mstime, 5); // dots
        }

        if(secUnit == 9)
        {
            int secTen = local_time.second() / 10 ;
            if (secTen == 5)
            {
                LoadFrame(12 * 30 + mstime, 6); // Sec1 5 -> 0

                int minUnit = local_time.minute() % 10;
                LoadFrame(minUnit * 30 + mstime, 4); // Min0

                if(minUnit == 9)
                {
                    int minTen = local_time.minute() / 10;
                    if(minTen == 5)
                    {
                        LoadFrame(12 * 30 + mstime, 3); // Min1 5 -> 0

                        int hoursUnit = local_time.hour() % 10;
                        LoadFrame(hoursUnit * 30 + mstime, 1); // Hour0

                        if(hoursUnit == 9)
                        {
                            int hoursTen = local_time.hour() / 10;
                            LoadFrame(hoursTen * 30 + mstime, 0); // Hour1
                        }
                        else
                        {
                            if(local_time.hour() == 23)
                            {
                                LoadFrame(10 * 30 + mstime, 0); // Hour1 2 -> 0
                                LoadFrame(11 * 30 + mstime, 1); // Hour0 3 -> 0
                            }
                        }
                    }
                    else
                    {
                        LoadFrame(minTen*30 + mstime, 3); // Min1
                    }
                }
            }
            else
            {
                LoadFrame(secTen * 30 + mstime, 6); // Sec1
            }
        }
    }
}

//============================================================================================================================================
void Dialog::on_pushButton_Flash_clicked()
{
    clock_timer.stop();
    ui->pushButton_Flash->setVisible(false);
    ui->pushButton_selectFile->setVisible(false);
    ui->comboBox_page->setVisible(false);
    ui->comboBox_SerialPort->setVisible(false);
    ui->label_SerialPort->setVisible(false);


    ui->textEdit_Log->append(QTime::currentTime().toString() + " Start upload sketch");
    ui->textEdit_Log->update();
    QApplication::processEvents();

    QString strCommand = "esptool -cp " + ui->comboBox_SerialPort->currentText() + " -bf 26 -cd nodemcu -cb 460800 -cf TheLCDclock_esp8285_loader.ino.esp8285.dat";
    m_proc->start(strCommand);
    m_proc->waitForStarted();
    while(m_proc->state() == QProcess::Running)
    {
        QApplication::processEvents();
        Sleep(500);
        QApplication::processEvents();
    }
    ui->textEdit_Log->append(QTime::currentTime().toString() + " Sketch uploaded. Wait 11 sec for reloading...");
    ui->textEdit_Log->update();
    QApplication::processEvents();

    Sleep(11000);

    if( OpenSerialPort(921600) )
    {
        FlashErase();
        FlashConfigData();
        FlashData();

        CloseHandle(hComm);//Closing the Serial Port
    }
    else
    {
        if (hComm)
        {
            CloseHandle(hComm); //Closing the Serial Port
        }
        ui->textEdit_Log->append(QTime::currentTime().toString()+" Flashing error");
        ui->textEdit_Log->update();
        QApplication::processEvents();
    }
    Sleep(500);

    ui->textEdit_Log->append(QTime::currentTime().toString()+" Start upload sketch");
    ui->textEdit_Log->update();
    strCommand = "esptool -cp " + ui->comboBox_SerialPort->currentText() + " -bf 26 -cd nodemcu -cb 460800 -cf TheLCDclock_esp8285.ino.esp8285.dat";
    m_proc->start(strCommand);
    m_proc->waitForStarted();
    while(m_proc->state() == QProcess::Running)
    {
        QApplication::processEvents();
        Sleep(500);
        QApplication::processEvents();
    }

    ui->pushButton_selectFile->setVisible(true);
}

//============================================================================================================================================
// Открытие порта
//============================================================================================================================================
bool Dialog::OpenSerialPort(int baud)
{
    QString portname = "\\\\.\\" + ui->comboBox_SerialPort->currentText();
    hComm = CreateFileA(portname.toStdString().c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

   if (hComm == INVALID_HANDLE_VALUE)
   {
       ui->textEdit_Log->append(QTime::currentTime().toString() + " Error open serial port");
       return false;
   }
   else
   {
       ui->textEdit_Log->append(QTime::currentTime().toString() + " OK! open serial port");
   }

   DCB ComDCM;

   memset(&ComDCM,0,sizeof(ComDCM));
   ComDCM.DCBlength = sizeof(DCB);
   GetCommState(hComm, &ComDCM);

   ComDCM.BaudRate = DWORD(baud);
   ComDCM.ByteSize = 8;
   ComDCM.Parity = NOPARITY;
   ComDCM.StopBits = ONESTOPBIT;
   ComDCM.fAbortOnError = TRUE;
   ComDCM.fDtrControl = DTR_CONTROL_DISABLE;
   ComDCM.fRtsControl = RTS_CONTROL_DISABLE;
   ComDCM.fBinary = TRUE;
   ComDCM.fParity = FALSE;
   ComDCM.fInX = FALSE;
   ComDCM.fOutX = FALSE;
   ComDCM.XonChar = 0;
   ComDCM.XoffChar = (unsigned char)0xFF;
   ComDCM.fErrorChar = FALSE;
   ComDCM.fNull = FALSE;
   ComDCM.fOutxCtsFlow = FALSE;
   ComDCM.fOutxDsrFlow = FALSE;
   ComDCM.XonLim = 128;
   ComDCM.XoffLim = 128;

   if(!SetCommState(hComm, &ComDCM))
   {
       CloseHandle(hComm);
       hComm = INVALID_HANDLE_VALUE;
       ui->textEdit_Log->append(QTime::currentTime().toString() + " Error SETUP serial port");
       return false;
   }

   COMMTIMEOUTS CommTimeOuts;
   CommTimeOuts.ReadIntervalTimeout = m_waitTimeout;
   CommTimeOuts.ReadTotalTimeoutMultiplier = 1;
   CommTimeOuts.ReadTotalTimeoutConstant = m_readTimeout;
   CommTimeOuts.WriteTotalTimeoutMultiplier = 1;
   CommTimeOuts.WriteTotalTimeoutConstant = m_wtiteTimeout;

   SetCommTimeouts(hComm, &CommTimeOuts);

   //--------------------------------
   QString val = "";
   QByteArray requestData;
   QByteArray responseData;

   requestData.resize(1);
   requestData[0] = 0x9F;

   buf_out = requestData.data();

   WriteFile(hComm, buf_out, requestData.size(), &bc, NULL);

   responseData.resize(30);
   buf_in = responseData.data();
   ReadFile(hComm, buf_in, responseData.size(), &bc, NULL);

   val = "";
   for (uint j = 0; j < bc; j++)
   {
       val +=  " " + QString("%1").arg(QString::number(uchar(responseData[j]),16).rightJustified(2,'0'));
   }
   ui->textEdit_Log->append(QTime::currentTime().toString() + "  check ID " +  val.toUpper());

   //--------------------------------
   if ((bc > 2) && (responseData.contains(ID_str)))
   {
        return true;
   }
   else
   {
       return false;
   }
}
//============================================================================================================================================
// Запись
//============================================================================================================================================
void Dialog::FlashData()
{
    QString val = "";
    QByteArray requestData;
    QByteArray responseData;

    if (!datafile.open(QIODevice::ReadOnly))
    {
        ui->textEdit_Log->append(QTime::currentTime().toString() + " Ошибка открытия файла");
        return;
    }

    char rr = 0xFF;
    ui->progressBar->setVisible(true);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(17 * 30 * 8 - 1);
    ui->progressBar->setValue(0);
    ui->textEdit_Log->append(QTime::currentTime().toString() + " Старт прошивки скина");

    for (int frame = 0; frame < 17 * 30 * 8; frame++)
    {
        FramesData.clear();
        FramesData = datafile.read(8100);
        FramesData.append(76, rr);

        QByteArray HashData = QCryptographicHash::hash(FramesData, QCryptographicHash::Md5); //16 bytes
        FramesData.append(HashData);

        ui->progressBar->setValue(frame);
        QApplication::processEvents();

        for (int page = 0; page < 4; page++)
        {
            requestData.resize(131);
            buf_out = requestData.data();

            for (int pj = 0; pj < 16; pj++)
            {
                requestData[0] = 0x02;
                requestData[1] = ((pj*128)>>8) & 0xFF;
                requestData[2] = (pj*128) & 0xFF;
                for (int i = 3; i < requestData.size(); i++)
                {
                   requestData[i] = FramesData[page * 2048 + pj * 128 + i - 3];
                }

                WriteFile(hComm, buf_out, requestData.size(), &bc, NULL);

                if (bc == 131)
                {
                    responseData.resize(2);
                    buf_in = responseData.data();

                    ReadFile(hComm, buf_in, responseData.size(), &bc, NULL);

                    if ((bc != 2) || (Ok_str != responseData))
                    {
                        if (fault_cnt < 5)
                        {
                            pj--;
                            fault_cnt++;
                            continue;
                        }
                        else
                        {
                            ui->textEdit_Log->append(QTime::currentTime().toString() + " Ошибка подтверждения загрузки данных");
                            fault_cnt = 0;
                        }
                    }
                    else
                    {
                        fault_cnt = 0;
                    }
                }
                else
                {
                    ui->textEdit_Log->append(QTime::currentTime().toString() + " Таймаут загрузки данных");
                }
            }

            requestData.resize(3);
            requestData[0] = 0x10;
            requestData[1] = (((frame * 4 + page + 1) >> 8) & 0xFF) + anim_page * 0x40;
            requestData[2] = (frame * 4 + page + 1) & 0xFF;
            buf_out = requestData.data();

            WriteFile(hComm, buf_out, requestData.size(), &bc, NULL);

            if (bc == 3)
            {
                responseData.resize(2);
                buf_in = responseData.data();

                ReadFile(hComm, buf_in, responseData.size(), &bc, NULL);

                if ((bc != 2) || (Ok_str != responseData))
                {
                    if (fault_cnt < 5)
                    {
                        page--;
                        fault_cnt++;
                        continue;
                    }
                    else
                    {
                        ui->textEdit_Log->append(QTime::currentTime().toString() + " Ошибка подтверждения команды записи");
                        fault_cnt = 0;
                    }
                }
                else
                {
                    fault_cnt = 0;
                }
            }
            else
            {
                ui->textEdit_Log->append(QTime::currentTime().toString() + " Таймаут команды записи");
            }
        }
    }
    ui->textEdit_Log->append(QTime::currentTime().toString() + " Прошивка скина завершена");

    QApplication::processEvents();

    ui->progressBar->setVisible(false);
    datafile.close();

}

//============================================================================================================================================
// Запись конфигурационных данных (0-page)
//============================================================================================================================================
void Dialog::FlashConfigData()
{
   const uchar header[] = {0x04, 0xCF, 0x00, 0x83, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x05, 0xED, 0x64, 0x03, 0x12, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x04, 0xE8, 0x85, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x06, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x02, 0xF7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x03, 0xEA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x02, 0xC0, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x02, 0xC1, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

                           0x03, 0xC5, 0x35, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x02, 0xC7, 0xBE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x02, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x02, 0x3A, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x01, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x03, 0xB1, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x02, 0xF2, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x02, 0x26, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,


                           0x0F, 0xE0, 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17,
                           0x0F, 0xE1, 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x23, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E,
                           0x05, 0x2A, 0x00, 0x00, 0x00, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x05, 0x2B, 0x00, 0x00, 0x01, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x01, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x02, 0xB7, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x05, 0xB6, 0x0A, 0x82, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x81, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

                           0x81, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
   QByteArray header_data = QByteArray::fromRawData((const char *)&header[0], sizeof(header));

   QString val = "";
   QByteArray requestData;
   QByteArray responseData;

   requestData.resize(131);
   buf_out = requestData.data();

   ui->textEdit_Log->append(QTime::currentTime().toString() + " Старт прошивки конфигурационных данных");

   for (int pj = 0; pj < 4; pj++)
   {
       requestData[0] = 0x02;
       requestData[1] = ((pj*128)>>8) & 0xFF;
       requestData[2] = (pj*128) & 0xFF;

       for (int i = 3; i < requestData.size(); i++)
       {
           requestData[i] = header_data[pj * 128 + i - 3];
       }

       WriteFile(hComm, buf_out, requestData.size(), &bc, NULL);

       if (bc == 131)
       {
           responseData.resize(2);
           buf_in = responseData.data();

           ReadFile(hComm, buf_in, responseData.size(), &bc, NULL);

           if ((bc != 2) || (Ok_str != responseData))
           {
               if (fault_cnt < 5)
               {
                   pj--;
                   fault_cnt++;
                   continue;
               }
               else
               {
                   ui->textEdit_Log->append(QTime::currentTime().toString() + " Ошибка подтверждения загрузки конфигурационных данных");
                   fault_cnt = 0;
               }
           }
           else
           {
               fault_cnt = 0;
           }
       }
       else
       {
            ui->textEdit_Log->append(QTime::currentTime().toString() + " Таймаут загрузки конфигурационных данных");
       }
   }

   //--------------------------------
   requestData.resize(3);
   buf_out = requestData.data();

   requestData[0] = 0x10;
   requestData[1] = 0x00 + anim_page * 0x40;
   requestData[2] = 0x00;

   WriteFile(hComm, buf_out, requestData.size(), &bc, NULL);

   if (bc == 3)
   {
       responseData.resize(2);
       buf_in = responseData.data();

       ReadFile(hComm, buf_in, responseData.size(), &bc, NULL);

       if ((bc != 2) || (Ok_str != responseData))
       {
           ui->textEdit_Log->append(QTime::currentTime().toString() + " Ошибка подтверждения команды записи");
       }
   }
   else
   {
       ui->textEdit_Log->append(QTime::currentTime().toString() + " Таймаут команды записи");
   }

   ui->textEdit_Log->append(QTime::currentTime().toString() + " Прошивка конфигурационных данных завершена");
}

//============================================================================================================================================
//============================================================================================================================================
void Dialog::FlashErase()
{
    QByteArray requestData;
    QByteArray responseData;
    //--------------------------------

    ui->textEdit_Log->append(QTime::currentTime().toString() + " Старт очистки");

    {
        requestData.resize(3);
        buf_out = requestData.data();

        ui->progressBar->setVisible(true);
        ui->progressBar->setMinimum(0);
        ui->progressBar->setMaximum(256);
        ui->progressBar->setValue(0);

        for (int pj = 0; pj < 256; pj++)
        {
            requestData[0] = 0xD8;
            requestData[1] = (((pj*64)>>8) & 0xFF) + anim_page * 0x40;
            requestData[2] = (pj*64) & 0xFF;

            ui->progressBar->setValue(pj);
            QApplication::processEvents();

            WriteFile(hComm, buf_out, requestData.size(), &bc, NULL);

            if (bc == 3)
            {
                responseData.resize(2);
                buf_in = responseData.data();

                ReadFile(hComm, buf_in, responseData.size(), &bc, NULL);

                if ((bc != 2) || (Ok_str != responseData))
                {
                    if (fault_cnt < 3)
                    {
                        pj--;
                        fault_cnt++;
                        continue;
                    }
                    else
                    {
                        ui->textEdit_Log->append(QTime::currentTime().toString() + " Ошибка подтверждения команды очистки");
                        fault_cnt = 0;
                    }
                }
                else
                {
                    fault_cnt = 0;
                }
            }
            else
            {
                ui->textEdit_Log->append(QTime::currentTime().toString() + " Таймаут команды очистки");
            }
        }

        ui->progressBar->setVisible(false);
    }

    ui->textEdit_Log->append(QTime::currentTime().toString() + " Очистка завершена");
}

//============================================================================================================================================
void Dialog::on_comboBox_page_currentIndexChanged(int index)
{
    anim_page = index;
}

//============================================================================================================================================
void  Dialog::slotDataOnStdout()
{
    ui->textEdit_Log->append(QTime::currentTime().toString() + " " + m_proc->readAllStandardOutput());
}

//============================================================================================================================================
