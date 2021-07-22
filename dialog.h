#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QFile>
#include <QTimer>
#include <QTime>


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

private:
    Ui::Dialog *ui;
    QImage *frame;

    QTimer clock_timer;
    int mstime = 0;

    QFile datafile;
    QByteArray FramesData;

    void LoadFrame(int index, int label);

protected:
    virtual void paintEvent(QPaintEvent *event);


};
//============================================================================================================================================

#endif // DIALOG_H
