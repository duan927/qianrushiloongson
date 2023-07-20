#include "spot.h"
#include "ui_spot.h"

spot::spot(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::spot)
{
    ui->setupUi(this);
}

spot::~spot()
{
    delete ui;
}
