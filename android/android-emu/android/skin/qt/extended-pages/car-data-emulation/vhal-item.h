#ifndef VHALITEM_H
#define VHALITEM_H

#include <QWidget>

namespace Ui {
class VhalItem;
}

class VhalItem : public QWidget {
    Q_OBJECT

public:
    explicit VhalItem(QWidget* parent = nullptr,
                      QString name = "null",
                      QString proId = "0");
    ~VhalItem();

private:
    Ui::VhalItem* ui;
};

#endif  // VHALITEM_H
