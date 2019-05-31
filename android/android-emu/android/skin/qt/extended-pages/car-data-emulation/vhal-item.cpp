#include "vhal-item.h"

#include "ui_vhal-item.h"

VhalItem::VhalItem(QWidget* parent, QString name, QString proID)
    : QWidget(parent), ui(new Ui::VhalItem) {
    ui->setupUi(this);
    ui->name->setText(name);
    ui->proid->setText(proID);
}

VhalItem::~VhalItem() {
    delete ui;
}
