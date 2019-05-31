#include "vhal-item.h"

#include "ui_vhal-item.h"

VhalItem::VhalItem(QWidget* parent, QString name, QString proID)
    : QWidget(parent), ui(new Ui::VhalItem) {
    ui->setupUi(this);
    ui->name->setText(name);
    ui->proid->setText(proID);
}

void VhalItem::vhalItemSelected(bool state) {
    if (state) {
        ui->name->setStyleSheet("color: white");
        ui->proid->setStyleSheet("color: white");
    } else {
        ui->name->setStyleSheet("");
        ui->proid->setStyleSheet("");
    }
}

VhalItem::~VhalItem() {
    delete ui;
}
