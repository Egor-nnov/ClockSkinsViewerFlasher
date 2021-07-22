#include <QFileDialog>

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

   connect(&clock_timer, SIGNAL(timeout()), SLOT(slotUpdateDateTime()));
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

    if (!datafile.open(QIODevice::ReadOnly))
    {
            //qDebug () << "Ошибка открытия дпя записи";
    }
    ui->lineEdit_selectFile->setText(FileName);

    FramesData.clear();
    FramesData = datafile.readAll();

    datafile.close();

    //135 x 240 x 2 = 64800 один кадр
    // 64800 = 8100 x 8 частей
    int Size = FramesData.size() / 64800 - 1;

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
    LoadFrame(14 * 30, 2);
    LoadFrame(minTen * 30, 3);
    LoadFrame(minUnit * 30, 4);
    LoadFrame(13 * 30, 5);
    LoadFrame(secTen * 30, 6);
    LoadFrame(secUnit * 30, 7);

    // Start animation
    clock_timer.start(15);
}
//============================================================================================================================================
void  Dialog::LoadFrame(int index, int label)
{
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

        LoadFrame(14 * 30 + mstime, 2); // dots1
        LoadFrame(13 * 30 + mstime, 5); // dots2

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
