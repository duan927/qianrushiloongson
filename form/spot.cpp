#include "spot.h"
#include "ui_spot.h"
#include<QDebug>
#include<QPainter>
#include<QPainterPath>
#include "qfiledialog.h"
#include "qmessagebox.h"
#include "sendemailthread.h"
spot::spot(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::spot)
{
    ui->setupUi(this);
    this->initForm();
    connection=false;
    name="";
}

spot::~spot()
{
    delete ui;
}

void spot::initForm()
{
    this->setMinimumSize(380, 300);
    this->setMaximumSize(380, 300);
    SendEmailThread::Instance()->start();
}

void spot::setid(QString ss)
{
    id=ss;
}

void spot::FreshForm()
{
    QSqlDatabase db;
    db = QSqlDatabase::addDatabase("QSQLITE","local");  //引入数据库的驱动字符串为SQLITE,相当于用这个来创建需要链接的数据库类型
    db.setDatabaseName("test.db");              //设置数据库,创建一个数据库文件
    db.open();                                  //打开数据库
    //QString ss="INSERT INTO spot VALUES("+rx.cap(1)+",\""+dateTime.toString("yyyy-MM-dd hh:mm:ss")+"\","+rx.cap(2)+")";
    QDateTime dateTime= QDateTime::currentDateTime();
    QString ss;
    QSqlQuery query(db);//创建一个QSqlQuery对象
    if(name=="")
    {
        ss="SELECT name from num where id ="+id;
        query.exec(ss);
        query.next();
        name=query.value(0).toString();
        ui->spotname->setText(name);
    }
    ss="SELECT * from spot where num ="+id+" ORDER BY id DESC";
    query.exec(ss);
    query.next();
    QDateTime time = QDateTime::fromString(query.value(1).toString(),"yyyy-MM-dd hh:mm:ss");
    if(dateTime.toTime_t()-time.toTime_t()<3)
   {
        tempurature=query.value(2).toFloat();
        wet=query.value(4).toInt();
        energy=query.value(5).toInt();
        light=query.value(6).toFloat();
        alcohol=query.value(7).toBool();
        smog=query.value(8).toBool();
        ui->temp->setText(QString("%1").arg(tempurature));
        ui->humi->setText(QString("%1").arg(wet));
        ui->elect->setText(QString("%1").arg(energy));
        ui->light->setText(QString("%1").arg(light));
        if(alcohol==1)
        {
            QString sr=name+"酒精浓度过高";
            QMessageBox::warning(this,"警报",sr);
            SendEmailThread::Instance()->append(sr.toHtmlEscaped());
        }
        if(!connection)
        {
            connection=true;
            ui->conection->setText("已连接");
            ss="INSERT INTO daily values("+id+",\""+dateTime.toString("yyyy-MM-dd hh:mm:ss")+"\",\"上线\")";
            query.exec(ss);
            qDebug()<<query.lastError();
        }
    }
    else
    {
        if(connection)
        {
            connection=false;
            QString sr=name+"已掉线";
            ss="INSERT INTO daily values("+id+",\""+dateTime.toString("yyyy-MM-dd hh:mm:ss")+"\",\"下线\")";
            query.exec(ss);
            QMessageBox::warning(this,"警报",sr);
            SendEmailThread::Instance()->append(sr.toHtmlEscaped());
        }
        ui->conection->setText("未连接");
        ui->temp->setText("");
        ui->humi->setText("");
        ui->elect->setText("");
        ui->light->setText("");
    }
    db.close();
    db = QSqlDatabase();
    db.QSqlDatabase::removeDatabase("local");
}

void spot::paintEvent(QPaintEvent *event)
{
    //绘制路径
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.setRenderHint(QPainter::Antialiasing);
    //设置边框为圆角12px
    QPainterPath path;
    path.addRoundedRect(rect(), 30, 30);
    painter.setClipPath(path);
    QBrush brush = QBrush(QColor("#ff0000"));
    painter.setBrush(brush);
    painter.drawRect(rect());
}
