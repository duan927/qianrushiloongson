#ifndef SPOT_H
#define SPOT_H

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
};

#endif // SPOT_H
