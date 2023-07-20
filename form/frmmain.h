#ifndef FRMMAIN_H
#define FRMMAIN_H
#include <QUdpSocket>
#include <QtSql>
#include <QSqlDatabase>
#include <QDialog>
#include"spot.h"
#include "qcustomplot.h"
#include <QtCharts>
namespace Ui {
class frmMain;
}

class frmMain : public QDialog
{
    Q_OBJECT

public:
    explicit frmMain(QWidget *parent = 0);
    ~frmMain();

protected:
    bool eventFilter(QObject *watched, QEvent *event);

private:
    Ui::frmMain *ui;
    QUdpSocket *udpSocket;//
    spot* rpb[6];
    int numrpb;
    int spotshow;
    QVector<QCustomPlot*> lineSty;
    QVector<QCPTextElement *> titel;

private slots:
    void onSocketReadyRead();//读取socket传入的数据
    void on_timer_timeout();
    void initForm();
    void buttonClick();
    int compare(QList<int> spotconnection);
    void setLED(QLabel* label, int color, int size);

private slots:
    void on_btnMenu_Min_clicked();
    void on_btnMenu_Max_clicked();
    void on_btnMenu_Close_clicked();
    void initTable();
    void initTableDaily();
    void initMain();
    void initLine();
    void on_comboBox_currentIndexChanged(int index);
    void on_toexcel_clicked();
};

#endif // UIDEMO01_H
