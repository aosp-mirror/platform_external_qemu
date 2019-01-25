android {
    TEMPLATE = subdirs
    SUBDIRS += androidextras jar
} else {
    TEMPLATE = aux
    QMAKE_DOCS = $$PWD/androidextras/doc/qtandroidextras.qdocconf
}
