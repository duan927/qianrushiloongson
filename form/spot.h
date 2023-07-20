#ifndef SPOT_H
#define SPOT_H
#include <QtSql>
#include <QSqlDatabase>
#include <QWidget>

namespace Ui {
class spot;
}

class spot : public QWidget
{
    Q_OBJECT

public:
    explicit spot(QWidget *parent = nullptr);
    ~spot();


private:
    Ui::spot *ui;

protected:
    void paintEvent(QPaintEvent *event) override;

public:
    void FreshForm();
    void setid(QString ss);
    bool connection;
    float tempurature;
    short wet;
    short energy;
    float light;
    bool alcohol;
    bool smog;


private:
    QString id;
    QString name;
    void initForm();
};

#endif // SPOT_H
