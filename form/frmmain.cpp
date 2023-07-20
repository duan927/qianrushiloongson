#include "frmmain.h"
#include "ui_frmmain.h"
#include "iconhelper.h"
#include "quihelper.h"
#include "xlsxdocument.h"
#include "xlsxformat.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
#include "qfiledialog.h"
#include "qmessagebox.h"
#include "sendemailthread.h"
#include <QTime>
#include <QTimer>
#include <QSqlQueryModel>
frmMain::frmMain(QWidget *parent) : QDialog(parent), ui(new Ui::frmMain)
{
    ui->setupUi(this);
    this->initForm();
    this->initMain();
    this->initLine();
    QTimer *fTimer=new QTimer(this);  //创建定时器
    fTimer->stop();
    fTimer->setInterval(1000);//设置定时周期,单位：毫秒
    connect(fTimer,SIGNAL(timeout()),this,SLOT(on_timer_timeout()));
    fTimer->start();
    udpSocket=new QUdpSocket(this);//用于与连接的客户端通讯的QTcpSocket
    connect(udpSocket,SIGNAL(readyRead()),
            this,SLOT(onSocketReadyRead()));
    quint16 port =1200; //本机UDP端口
    spotshow=0;
    if (udpSocket->bind(QHostAddress::Any,port)) //绑定端口成功
    {
        qDebug()<<"成功绑定端口";
    }
}

frmMain::~frmMain()
{
    delete ui;
}

void frmMain::on_timer_timeout()
{
    QTime curTime=QTime::currentTime(); //获取当前时间
    QString str_datetime = curTime.toString("hh:mm:ss");
    ui -> lcd -> display(str_datetime);
    double x_Time = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
    //这里的8，是指横坐标时间宽度为8秒，如果想要横坐标显示更多的时间
    //就把8调整为比较大到值，比如要显示60秒，那就改成60。
    int i = 0;
    for(i = 0; i < lineSty.size(); i++)
    {
        lineSty[i]->xAxis->setRange(x_Time+0.25, 20, Qt::AlignRight);//设定x轴的范围
        lineSty[i]->replot();
    }
    QList<int> spotconnection;
    for(i = 0; i < numrpb; i ++)
    {
        rpb[i]->FreshForm();
        if(rpb[i]->connection) spotconnection.append(i);
    }
    if(rpb[spotshow]->connection)
    {
        ui->settemp->setValue(rpb[spotshow]->tempurature);
        ui->setwet->setValue(rpb[spotshow]->wet);
        ui->setlight->setValue(rpb[spotshow]->light);
        ui->battery->setValue(rpb[spotshow]->energy);
        lineSty[0]->graph(0)->addData(x_Time,rpb[spotshow]->tempurature);//添加数据1到曲线1
        lineSty[1]->graph(0)->addData(x_Time,rpb[spotshow]->wet);//添加数据1到曲线1
        lineSty[2]->graph(0)->addData(x_Time,rpb[spotshow]->light);//添加数据1到曲线1
        if(rpb[spotshow]->alcohol) setLED(ui->led1, 1, 120);
        else setLED(ui->led1, 2, 120);
        if(rpb[spotshow]->smog) setLED(ui->led2, 1, 120);
        else setLED(ui->led2, 2, 120);
    }
    else
    {
        ui->settemp->setValuenone();
        ui->setwet->setValuenone();
        ui->setlight->setValuenone();
        setLED(ui->led1, 0, 120);
        setLED(ui->led2, 0, 120);
    }
    if(spotconnection.size()>=3)
    {
        if((i=compare(spotconnection))!=spotconnection.size()){
            QString sr=spotconnection[i]+"数据异常";
            QMessageBox::warning(this,"警报",sr);
            SendEmailThread::Instance()->append(sr.toHtmlEscaped());
        }
    }
}

int frmMain::compare(QList<int> spotconnection)
{
    float sum=0;
    float psum=0;
    for(int i = 0; i < spotconnection.size(); i ++)
    {
        sum=sum+rpb[spotconnection[i]]->tempurature;
        psum=psum+rpb[spotconnection[i]]->tempurature*rpb[spotconnection[i]]->tempurature;
    }
    sum=sum/spotconnection.size();
    float sigma=qSqrt( (psum/spotconnection.size() - sum*sum));
    qDebug()<<sigma;
    if(sigma<2)return spotconnection.size();
    else
    {
        for(int i = 0; i < spotconnection.size(); i ++)
        {
            if(rpb[spotconnection[i]]->tempurature>sum+sigma||rpb[spotconnection[i]]->tempurature<sum-sigma) return spotconnection[i];
        }
    }
    if(sigma<2)return spotconnection.size();
}

void frmMain::initLine()
{
    QVector<QString> line_name = {"温度","湿度","光照"};
    lineSty.clear();
    lineSty.append(ui->plot1);
    lineSty.append(ui->plot2);
    lineSty.append(ui->plot3);
    for(int i = 0; i < lineSty.size(); i ++)
    {
        QCPTextElement *textelement = new QCPTextElement(lineSty[i], line_name[i],QFont("sans", 17, QFont::Bold));
        textelement->setTextColor(QColor(15, 167, 234));
        titel.append(textelement);
    }
    lineSty[0]->yAxis->setLabel("温度/°C");
    lineSty[1]->yAxis->setLabel("湿度/%");
    lineSty[2]->yAxis->setLabel("光照/°C");
    for(int i = 0; i < lineSty.size(); i ++)
    {
        QSharedPointer<QCPAxisTickerDateTime> dateTick(new QCPAxisTickerDateTime);
        dateTick->setDateTimeFormat("HH:mm:ss");
        lineSty[i] ->xAxis->setTicker(dateTick);
        lineSty[i]->plotLayout()->insertRow(0);
        lineSty[i]->plotLayout()->addElement(0, 0, titel[i]);
        lineSty[i]->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                                    QCP::iSelectLegend | QCP::iSelectPlottables);//允许鼠标点击拖动
        lineSty[i]->setBackground(QColor(61, 85, 122));
        lineSty[i]->legend->setVisible(true);
        lineSty[i]->legend->setBrush(QColor(255,255,255,150));//legend背景色设为白色但背景透明，允许图像在legend区域可见

        lineSty[i]->xAxis->setLabel("时间/s");   //XY轴的名字
        lineSty[i]->xAxis->setLabelColor(QColor(255, 255, 255));   //XY轴的名字
        lineSty[i]->xAxis->setBasePen(QColor(255, 255, 255));//x轴颜色
        lineSty[i]->xAxis->setTickPen(QColor(255, 255, 255));//设置x坐标线上 大 分格线颜色
        lineSty[i]->xAxis->setSubTickPen(QColor(255, 255, 255));//设置x坐标线上小分格线颜色
        lineSty[i]->xAxis->setTickLabelColor(QColor(255, 255, 255));


        lineSty[i]->yAxis->setLabelColor(QColor(255, 255, 255));   //XY轴的名字
        lineSty[i]->yAxis->setBasePen(QColor(255, 255, 255));//设置y坐标线颜色
        lineSty[i]->yAxis->setTickPen(QColor(255, 255, 255));//设置x坐标线上 大 分格线颜色
        lineSty[i]->yAxis->setSubTickPen(QColor(255, 255, 255));//设置x坐标线上小分格线颜色
        lineSty[i]->yAxis->setTickLabelColor(QColor(255, 255, 255));

        lineSty[i]->addGraph();

        //数据线样式设计
        lineSty[i]->graph(0)->setPen(QColor(27, 254, 51));
        lineSty[i]->graph(0)->setName("节点1");

        lineSty[i]->xAxis->setRange(0,40);
        lineSty[i]->yAxis->setRange(-20,80);
    }
}

void frmMain::onSocketReadyRead()
{
    QSqlDatabase db;
    db = QSqlDatabase::addDatabase("QSQLITE","local");  //引入数据库的驱动字符串为SQLITE,相当于用这个来创建需要链接的数据库类型
    db.setDatabaseName("test.db");              //设置数据库,创建一个数据库文件其
    db.open();                                  //打开数据库
    QSqlDatabase dbcloud = QSqlDatabase::addDatabase("QMYSQL","cloud");
    dbcloud.setHostName("82.157.177.198");  //连接本地主机
    dbcloud.setPort(3306);
    dbcloud.setDatabaseName("wzr");
    dbcloud.setUserName("wzr");
    dbcloud.setPassword("12345678");
    dbcloud.open();
    int pos=0;
    //QRegExp rx("^BUPT([0-9]{2})T([0-9]{1,2}.[0-9]{2})W([0-9]{1,2}.[0-9]{2})E([0-9]{1,2})$"); //BUPT11T12.11W22.11E11
    QRegExp rx("^BUPT([0-9]{2})T([0-9]{1,2}.[0-9]{2})W([0-9]{1,2})E([0-9]{1,2})L([0-9]{1,2}.[0-9]{2})A([0-1]{1})D([0-1]{1})$"); //BUPT11T12.11W22E11L22.22A0D0
    QRegExpValidator v(rx, 0);
    while(udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());  //数据格式统一

        QHostAddress peerAddr;
        quint16 peerPort;
        udpSocket->readDatagram(datagram.data(),
                                datagram.size(),&peerAddr,&peerPort);   //接收数据
        QString str=datagram.data();    //数据转换为QT的ui界面使用的QString类型

        int s=v.validate(str,pos);

        if(s==2)
        {
            rx.indexIn(str);
            qDebug()<<rx.cap(2);
            QDateTime dateTime= QDateTime::currentDateTime();
            QString ss="INSERT INTO spot(\"num\",\"time\",\"temp\",\"wet\",\"energy\",\"light\",\"alcohol\",\"smog\") VALUES("+rx.cap(1)+",\""+dateTime.toString("yyyy-MM-dd hh:mm:ss")+"\","+rx.cap(2)+","+rx.cap(3)+","+rx.cap(4)+","+rx.cap(5)+","+rx.cap(6)+","+rx.cap(7)+")";
            qDebug()<<ss;
            QSqlQuery query(db);	//创建一个QSqlQuery对象
            query.exec(ss);
            qDebug()<<query.lastError();
            ss="INSERT INTO spot(`num`,`time`,`temp`) VALUES("+rx.cap(1)+",\""+dateTime.toString("yyyy-MM-dd hh:mm:ss")+"\","+rx.cap(2)+")";
            QSqlQuery querycloud(dbcloud);	//创建一个QSqlQuery对象
            querycloud.exec(ss);
            qDebug()<<querycloud.lastError();
        }
    }
    db.close();
    db = QSqlDatabase();
    dbcloud.close();
    dbcloud = QSqlDatabase();
    db.QSqlDatabase::removeDatabase("local");
    dbcloud.QSqlDatabase::removeDatabase("cloud");
}

bool frmMain::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick) {
        if (watched == ui->widgetTitle) {
            on_btnMenu_Max_clicked();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void frmMain::initForm()
{
    //设置无边框
    QUIHelper::setFramelessForm(this);
    //设置图标
    IconHelper::setIcon(ui->labIco, 0xf099, 35);
    IconHelper::setIcon(ui->btnMenu_Min, 0xf068);
    IconHelper::setIcon(ui->btnMenu_Max, 0xf067);
    IconHelper::setIcon(ui->btnMenu_Close, 0xf00d);

    //ui->widgetMenu->setVisible(false);
    ui->widgetTitle->installEventFilter(this);
    ui->widgetTitle->setProperty("form", "title");
    ui->widgetTop->setProperty("nav", "top");

    QFont font;
    font.setPixelSize(25);
    ui->labTitle->setFont(font);
    ui->labTitle->setText("传感器监控管理系统");
    this->setWindowTitle(ui->labTitle->text());


    //单独设置指示器大小
    int addWidth = 20;
    int addHeight = 10;
    int rbtnWidth = 15;
    int ckWidth = 13;
    int scrWidth = 12;
    int borderWidth = 3;

    QStringList qss;
    qss << QString("QComboBox::drop-down,QDateEdit::drop-down,QTimeEdit::drop-down,QDateTimeEdit::drop-down{width:%1px;}").arg(addWidth);
    qss << QString("QComboBox::down-arrow,QDateEdit[calendarPopup=\"true\"]::down-arrow,QTimeEdit[calendarPopup=\"true\"]::down-arrow,"
                   "QDateTimeEdit[calendarPopup=\"true\"]::down-arrow{width:%1px;height:%1px;right:2px;}").arg(addHeight);
    qss << QString("QRadioButton::indicator{width:%1px;height:%1px;}").arg(rbtnWidth);
    qss << QString("QCheckBox::indicator,QGroupBox::indicator,QTreeWidget::indicator,QListWidget::indicator{width:%1px;height:%1px;}").arg(ckWidth);
    qss << QString("QScrollBar:horizontal{min-height:%1px;border-radius:%2px;}QScrollBar::handle:horizontal{border-radius:%2px;}"
                   "QScrollBar:vertical{min-width:%1px;border-radius:%2px;}QScrollBar::handle:vertical{border-radius:%2px;}").arg(scrWidth).arg(scrWidth / 2);
    qss << QString("QWidget#widget_top>QToolButton:pressed,QWidget#widget_top>QToolButton:hover,"
                   "QWidget#widget_top>QToolButton:checked,QWidget#widget_top>QLabel:hover{"
                   "border-width:0px 0px %1px 0px;}").arg(borderWidth);
    qss << QString("QWidget#widgetleft>QPushButton:checked,QWidget#widgetleft>QToolButton:checked,"
                   "QWidget#widgetleft>QPushButton:pressed,QWidget#widgetleft>QToolButton:pressed{"
                   "border-width:0px 0px 0px %1px;}").arg(borderWidth);
    this->setStyleSheet(qss.join(""));

    QSize icoSize(32, 32);
    int icoWidth = 85;

    //设置顶部导航按钮
    QList<QToolButton *> tbtns = ui->widgetTop->findChildren<QToolButton *>();
    foreach (QToolButton *btn, tbtns) {
        btn->setIconSize(icoSize);
        btn->setMinimumWidth(icoWidth);
        btn->setCheckable(true);
        connect(btn, SIGNAL(clicked()), this, SLOT(buttonClick()));
    }

    ui->btnMain->click();
    this->initTable();
    //this->initTableDaily();
}

// 该函数将label控件变成一个圆形指示灯，需要指定颜色color以及直径size
// color 0:grey 1:red 2:green 3:yellow
// size  单位是像素
void frmMain::setLED(QLabel* label, int color, int size)
{
    // 将label中的文字清空
    label->setText("");
    // 先设置矩形大小
    // 如果ui界面设置的label大小比最小宽度和高度小，矩形将被设置为最小宽度和最小高度；
    // 如果ui界面设置的label大小比最小宽度和高度大，矩形将被设置为最大宽度和最大高度；
    QString min_width = QString("min-width: %1px;").arg(size);              // 最小宽度：size
    QString min_height = QString("min-height: %1px;").arg(size);            // 最小高度：size
    QString max_width = QString("max-width: %1px;").arg(size);              // 最小宽度：size
    QString max_height = QString("max-height: %1px;").arg(size);            // 最小高度：size
    // 再设置边界形状及边框
    QString border_radius = QString("border-radius: %1px;").arg(size/2);    // 边框是圆角，半径为size/2
    QString border = QString("border:1px solid black;");                    // 边框为1px黑色
    // 最后设置背景颜色
    QString background = "background-color:";
    switch (color) {
    case 0:
        // 灰色
        background += "rgb(190,190,190)";
        break;
    case 1:
        // 红色
        background += "rgb(255,0,0)";
        break;
    case 2:
        // 绿色
        background += "rgb(0,255,0)";
        break;
    case 3:
        // 黄色
        background += "rgb(255,255,0)";
        break;
    default:
        break;
    }

    const QString SheetStyle = min_width + min_height + max_width + max_height + border_radius + border + background;
    label->setStyleSheet(SheetStyle);
}

void frmMain::buttonClick()
{
    QToolButton *b = (QToolButton *)sender();
    QString name = b->text();

    QList<QToolButton *> tbtns = ui->widgetTop->findChildren<QToolButton *>();
    foreach (QToolButton *btn, tbtns) {
        btn->setChecked(btn == b);
    }

    if (name == "主界面") {
        ui->stackedWidget->setCurrentIndex(0);
    } else if (name == "系统设置") {
        ui->stackedWidget->setCurrentIndex(1);
    } else if (name == "数据查看") {
        this->initTable();
        ui->stackedWidget->setCurrentIndex(2);
    } else if (name == "调试帮助") {
        ui->stackedWidget->setCurrentIndex(3);
    } else if (name == "节点信息") {
        ui->stackedWidget->setCurrentIndex(4);
    } else if (name == "节点日志") {
        this->initTableDaily();
        ui->stackedWidget->setCurrentIndex(5);
    }else if (name == "用户退出") {
        exit(0);
    }
}

void frmMain::on_btnMenu_Min_clicked()
{
    showMinimized();
}

void frmMain::initTable()
{
    QSqlDatabase db;
    db = QSqlDatabase::addDatabase("QSQLITE");  //引入数据库的驱动字符串为SQLITE,相当于用这个来创建需要链接的数据库类型
    db.setDatabaseName("test.db");              //设置数据库,创建一个数据库文件
    db.open();                                  //打开数据库
    QSqlQueryModel* qmodel = new QSqlQueryModel();
    qmodel->setQuery("select name,time,temp,wet,light,alcohol,smog from spot INNER JOIN num on spot.num=num.id ORDER BY spot.id DESC");
    db.close();
    db = QSqlDatabase();
    db.QSqlDatabase::removeDatabase("qt_sql_default_connection");
    ui->table1->setModel(qmodel);
}

void frmMain::initTableDaily()
{
    QSqlDatabase db;
    db = QSqlDatabase::addDatabase("QSQLITE");  //引入数据库的驱动字符串为SQLITE,相当于用这个来创建需要链接的数据库类型
    db.setDatabaseName("test.db");              //设置数据库,创建一个数据库文件
    db.open();                                  //打开数据库
    QSqlQueryModel* qmodel = new QSqlQueryModel();
    qmodel->setQuery("select name,time,updown from daily INNER JOIN num on daily.num=num.id");
    ui->table2->setModel(qmodel);
    db.close();
    db = QSqlDatabase();
    db.QSqlDatabase::removeDatabase("qt_sql_default_connection");
}

void frmMain::initMain()
{
    QSqlDatabase db;
    db = QSqlDatabase::addDatabase("QSQLITE");  //引入数据库的驱动字符串为SQLITE,相当于用这个来创建需要链接的数据库类型
    db.setDatabaseName("test.db");              //设置数据库,创建一个数据库文件
    db.open();                                  //打开数据库
    QString ss="select id from num";
    QSqlQuery query(db);	//创建一个QSqlQuery对象
    query.exec(ss);
    ui->gridLayout->setSpacing(10);//设置间距
    int i = 0;
    for(; i < 6&&query.next(); i ++)
    {
        ss=query.value(0).toString();
        rpb[i]=new spot();
        rpb[i]->setid(ss);
        rpb[i]->FreshForm();
        ui->gridLayout->addWidget(rpb[i],i/3,i%3);
    }
    numrpb=i;
    qDebug()<<numrpb;
    db.close();
    db = QSqlDatabase();
    db.QSqlDatabase::removeDatabase("qt_sql_default_connection");
}

void frmMain::on_btnMenu_Max_clicked()
{
    static bool max = false;
    static QRect location = this->geometry();

    if (max) {
        this->setGeometry(location);
    } else {
        location = this->geometry();
        this->setGeometry(QUIHelper::getScreenRect());
    }

    this->setProperty("canMove", max);
    max = !max;
}

void frmMain::on_btnMenu_Close_clicked()
{
    close();
}

void frmMain::on_comboBox_currentIndexChanged(int index)
{
    spotshow=index;
    lineSty[0]->clearGraphs();
    lineSty[0]->addGraph();

    //数据线样式设计
    lineSty[0]->graph(0)->setPen(QColor(27, 254, 51));
    lineSty[0]->graph(0)->setName(ui->comboBox->currentText());
    lineSty[1]->clearGraphs();
    lineSty[1]->addGraph();

    //数据线样式设计
    lineSty[1]->graph(0)->setPen(QColor(27, 254, 51));
    lineSty[1]->graph(0)->setName(ui->comboBox->currentText());
    lineSty[2]->clearGraphs();
    lineSty[2]->addGraph();

    //数据线样式设计
    lineSty[2]->graph(0)->setPen(QColor(27, 254, 51));
    lineSty[2]->graph(0)->setName(ui->comboBox->currentText());
}


void frmMain::on_toexcel_clicked()
{
    QSqlDatabase db;
    db = QSqlDatabase::addDatabase("QSQLITE");  //引入数据库的驱动字符串为SQLITE,相当于用这个来创建需要链接的数据库类型
    db.setDatabaseName("test.db");              //设置数据库,创建一个数据库文件
    db.open();                                  //打开数据库
    QXlsx::Document xlsx;
    QXlsx::Format title_format; /*设置标题的样式*/
    QXlsx::Format format2;/*小标题样式*/
    QXlsx::Format format3;/*数据内容样式*/
    title_format.setBorderStyle(QXlsx::Format::BorderThin);//外边框
    format2.setBorderStyle(QXlsx::Format::BorderThin);//外边框
    format3.setBorderStyle(QXlsx::Format::BorderThin);//外边框
    xlsx.setRowHeight(1,1,25);/*设置标题行高*/
    xlsx.setColumnWidth(1,7,20);/*设置列宽，一共7列参数*/

    title_format.setFontSize(11);
    title_format.setFontColor(QColor(Qt::red));
    title_format.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    title_format.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    xlsx.mergeCells("A1:G1",title_format);//合并1~7列写入标题
    xlsx.write("A1","节点数据");

    format2.setFontColor(QColor(Qt::blue));
    format2.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    xlsx.write("A2", "节点", format2);/*写入文字，应该刚才设置的样式*/
    xlsx.write("B2", "时间", format2);
    xlsx.write("C2", "温度", format2);
    xlsx.write("D2", "湿度", format2);
    xlsx.write("E2", "光照", format2);
    xlsx.write("F2", "酒精", format2);
    xlsx.write("G2", "烟雾", format2);

    format3.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    QSqlQuery query;
    QString selectSql="select name,time,temp,wet,light,alcohol,smog from spot INNER JOIN num on spot.num=num.id ORDER BY spot.id DESC";
    query.exec(selectSql);
    int i=3;
    while(query.next())//一行一行遍历
    {
        xlsx.write(i,1,query.value(0).toString(),format3);
        qDebug()<<query.value(0).toString();
        xlsx.write(i,2,query.value(1).toString(),format3);
        xlsx.write(i,3,query.value(2).toFloat(),format3);
        xlsx.write(i,4,query.value(3).toInt(),format3);
        xlsx.write(i,5,query.value(4).toFloat(),format3);
        xlsx.write(i,6,query.value(5).toBool(),format3);
        xlsx.write(i,7,query.value(6).toBool(),format3);
        i++;
    }
    //设置excel表格的默认文件名为"Student Information-当前时间"
    QString current_date =QDateTime::currentDateTime().toString(Qt::ISODate);
    QString fileName=tr("Student Information-")+current_date;
    QString dir=QString("../%1").arg(fileName);
    QString dir1=dir.replace(QRegExp(":"),"-");
    /*??QFSFileEngine::open: No file name specified*/
    QString path = QFileDialog::getSaveFileName(this, tr("save"), dir1, "XLSX(*.xlsx)");
    xlsx.saveAs(path+".xlsx");/*保存*/
    db.close();
    db = QSqlDatabase();
    db.QSqlDatabase::removeDatabase("qt_sql_default_connection");
}

