#include <stdio.h>
#ifdef _WIN32
#include <rpc.h>
#else
#include <uuid/uuid.h>
#endif

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include "Object.h"


int main(int argc, char** argv) {
#ifdef _WIN32
    UUID uu;
    UuidCreate(&uu);
#else
    uuid_t uu;
    uuid_generate(uu);
#endif


    QApplication app(argc, argv);
	QMessageBox msgBox;
	msgBox.setText("Foo");
	msgBox.setInformativeText("Bar");
	msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Save);
	return msgBox.exec();
}
