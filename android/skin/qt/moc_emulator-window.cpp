/****************************************************************************
** Meta object code from reading C++ file 'emulator-window.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "emulator-window.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'emulator-window.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_MainLoopThread_t {
    QByteArrayData data[1];
    char stringdata[15];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainLoopThread_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainLoopThread_t qt_meta_stringdata_MainLoopThread = {
    {
QT_MOC_LITERAL(0, 0, 14) // "MainLoopThread"

    },
    "MainLoopThread"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainLoopThread[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void MainLoopThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject MainLoopThread::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_MainLoopThread.data,
      qt_meta_data_MainLoopThread,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *MainLoopThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainLoopThread::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_MainLoopThread.stringdata))
        return static_cast<void*>(const_cast< MainLoopThread*>(this));
    return QThread::qt_metacast(_clname);
}

int MainLoopThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
struct qt_meta_stringdata_EmulatorWindow_t {
    QByteArrayData data[85];
    char stringdata[996];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EmulatorWindow_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EmulatorWindow_t qt_meta_stringdata_EmulatorWindow = {
    {
QT_MOC_LITERAL(0, 0, 14), // "EmulatorWindow"
QT_MOC_LITERAL(1, 15, 4), // "blit"
QT_MOC_LITERAL(2, 20, 0), // ""
QT_MOC_LITERAL(3, 21, 7), // "QImage*"
QT_MOC_LITERAL(4, 29, 3), // "src"
QT_MOC_LITERAL(5, 33, 6), // "QRect*"
QT_MOC_LITERAL(6, 40, 7), // "srcRect"
QT_MOC_LITERAL(7, 48, 3), // "dst"
QT_MOC_LITERAL(8, 52, 7), // "QPoint*"
QT_MOC_LITERAL(9, 60, 6), // "dstPos"
QT_MOC_LITERAL(10, 67, 26), // "QPainter::CompositionMode*"
QT_MOC_LITERAL(11, 94, 2), // "op"
QT_MOC_LITERAL(12, 97, 11), // "QSemaphore*"
QT_MOC_LITERAL(13, 109, 9), // "semaphore"
QT_MOC_LITERAL(14, 119, 12), // "createBitmap"
QT_MOC_LITERAL(15, 132, 12), // "SkinSurface*"
QT_MOC_LITERAL(16, 145, 1), // "s"
QT_MOC_LITERAL(17, 147, 1), // "w"
QT_MOC_LITERAL(18, 149, 1), // "h"
QT_MOC_LITERAL(19, 151, 4), // "fill"
QT_MOC_LITERAL(20, 156, 12), // "const QRect*"
QT_MOC_LITERAL(21, 169, 4), // "rect"
QT_MOC_LITERAL(22, 174, 13), // "const QColor*"
QT_MOC_LITERAL(23, 188, 5), // "color"
QT_MOC_LITERAL(24, 194, 13), // "getBitmapInfo"
QT_MOC_LITERAL(25, 208, 18), // "SkinSurfacePixels*"
QT_MOC_LITERAL(26, 227, 3), // "pix"
QT_MOC_LITERAL(27, 231, 13), // "getMonitorDpi"
QT_MOC_LITERAL(28, 245, 4), // "int*"
QT_MOC_LITERAL(29, 250, 7), // "out_dpi"
QT_MOC_LITERAL(30, 258, 19), // "getScreenDimensions"
QT_MOC_LITERAL(31, 278, 8), // "out_rect"
QT_MOC_LITERAL(32, 287, 11), // "getWindowId"
QT_MOC_LITERAL(33, 299, 4), // "WId*"
QT_MOC_LITERAL(34, 304, 6), // "out_id"
QT_MOC_LITERAL(35, 311, 12), // "getWindowPos"
QT_MOC_LITERAL(36, 324, 1), // "x"
QT_MOC_LITERAL(37, 326, 1), // "y"
QT_MOC_LITERAL(38, 328, 20), // "isWindowFullyVisible"
QT_MOC_LITERAL(39, 349, 5), // "bool*"
QT_MOC_LITERAL(40, 355, 9), // "out_value"
QT_MOC_LITERAL(41, 365, 9), // "pollEvent"
QT_MOC_LITERAL(42, 375, 10), // "SkinEvent*"
QT_MOC_LITERAL(43, 386, 5), // "event"
QT_MOC_LITERAL(44, 392, 8), // "hasEvent"
QT_MOC_LITERAL(45, 401, 10), // "queueEvent"
QT_MOC_LITERAL(46, 412, 13), // "releaseBitmap"
QT_MOC_LITERAL(47, 426, 9), // "sempahore"
QT_MOC_LITERAL(48, 436, 12), // "requestClose"
QT_MOC_LITERAL(49, 449, 13), // "requestUpdate"
QT_MOC_LITERAL(50, 463, 13), // "setWindowIcon"
QT_MOC_LITERAL(51, 477, 20), // "const unsigned char*"
QT_MOC_LITERAL(52, 498, 4), // "data"
QT_MOC_LITERAL(53, 503, 4), // "size"
QT_MOC_LITERAL(54, 508, 12), // "setWindowPos"
QT_MOC_LITERAL(55, 521, 8), // "setTitle"
QT_MOC_LITERAL(56, 530, 14), // "const QString*"
QT_MOC_LITERAL(57, 545, 5), // "title"
QT_MOC_LITERAL(58, 551, 10), // "showWindow"
QT_MOC_LITERAL(59, 562, 13), // "is_fullscreen"
QT_MOC_LITERAL(60, 576, 9), // "slot_blit"
QT_MOC_LITERAL(61, 586, 18), // "slot_clearInstance"
QT_MOC_LITERAL(62, 605, 17), // "slot_createBitmap"
QT_MOC_LITERAL(63, 623, 9), // "slot_fill"
QT_MOC_LITERAL(64, 633, 18), // "slot_getBitmapInfo"
QT_MOC_LITERAL(65, 652, 18), // "slot_getMonitorDpi"
QT_MOC_LITERAL(66, 671, 24), // "slot_getScreenDimensions"
QT_MOC_LITERAL(67, 696, 16), // "slot_getWindowId"
QT_MOC_LITERAL(68, 713, 17), // "slot_getWindowPos"
QT_MOC_LITERAL(69, 731, 25), // "slot_isWindowFullyVisible"
QT_MOC_LITERAL(70, 757, 14), // "slot_pollEvent"
QT_MOC_LITERAL(71, 772, 15), // "slot_queueEvent"
QT_MOC_LITERAL(72, 788, 18), // "slot_releaseBitmap"
QT_MOC_LITERAL(73, 807, 17), // "slot_requestClose"
QT_MOC_LITERAL(74, 825, 18), // "slot_requestUpdate"
QT_MOC_LITERAL(75, 844, 18), // "slot_setWindowIcon"
QT_MOC_LITERAL(76, 863, 17), // "slot_setWindowPos"
QT_MOC_LITERAL(77, 881, 19), // "slot_setWindowTitle"
QT_MOC_LITERAL(78, 901, 15), // "slot_showWindow"
QT_MOC_LITERAL(79, 917, 10), // "slot_power"
QT_MOC_LITERAL(80, 928, 11), // "slot_rotate"
QT_MOC_LITERAL(81, 940, 13), // "slot_volumeUp"
QT_MOC_LITERAL(82, 954, 15), // "slot_volumeDown"
QT_MOC_LITERAL(83, 970, 9), // "slot_zoom"
QT_MOC_LITERAL(84, 980, 15) // "slot_fullscreen"

    },
    "EmulatorWindow\0blit\0\0QImage*\0src\0"
    "QRect*\0srcRect\0dst\0QPoint*\0dstPos\0"
    "QPainter::CompositionMode*\0op\0QSemaphore*\0"
    "semaphore\0createBitmap\0SkinSurface*\0"
    "s\0w\0h\0fill\0const QRect*\0rect\0const QColor*\0"
    "color\0getBitmapInfo\0SkinSurfacePixels*\0"
    "pix\0getMonitorDpi\0int*\0out_dpi\0"
    "getScreenDimensions\0out_rect\0getWindowId\0"
    "WId*\0out_id\0getWindowPos\0x\0y\0"
    "isWindowFullyVisible\0bool*\0out_value\0"
    "pollEvent\0SkinEvent*\0event\0hasEvent\0"
    "queueEvent\0releaseBitmap\0sempahore\0"
    "requestClose\0requestUpdate\0setWindowIcon\0"
    "const unsigned char*\0data\0size\0"
    "setWindowPos\0setTitle\0const QString*\0"
    "title\0showWindow\0is_fullscreen\0slot_blit\0"
    "slot_clearInstance\0slot_createBitmap\0"
    "slot_fill\0slot_getBitmapInfo\0"
    "slot_getMonitorDpi\0slot_getScreenDimensions\0"
    "slot_getWindowId\0slot_getWindowPos\0"
    "slot_isWindowFullyVisible\0slot_pollEvent\0"
    "slot_queueEvent\0slot_releaseBitmap\0"
    "slot_requestClose\0slot_requestUpdate\0"
    "slot_setWindowIcon\0slot_setWindowPos\0"
    "slot_setWindowTitle\0slot_showWindow\0"
    "slot_power\0slot_rotate\0slot_volumeUp\0"
    "slot_volumeDown\0slot_zoom\0slot_fullscreen"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EmulatorWindow[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      79,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      36,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    6,  409,    2, 0x06 /* Public */,
       1,    5,  422,    2, 0x26 /* Public | MethodCloned */,
      14,    4,  433,    2, 0x06 /* Public */,
      14,    3,  442,    2, 0x26 /* Public | MethodCloned */,
      19,    4,  449,    2, 0x06 /* Public */,
      19,    3,  458,    2, 0x26 /* Public | MethodCloned */,
      24,    3,  465,    2, 0x06 /* Public */,
      24,    2,  472,    2, 0x26 /* Public | MethodCloned */,
      27,    2,  477,    2, 0x06 /* Public */,
      27,    1,  482,    2, 0x26 /* Public | MethodCloned */,
      30,    2,  485,    2, 0x06 /* Public */,
      30,    1,  490,    2, 0x26 /* Public | MethodCloned */,
      32,    2,  493,    2, 0x06 /* Public */,
      32,    1,  498,    2, 0x26 /* Public | MethodCloned */,
      35,    3,  501,    2, 0x06 /* Public */,
      35,    2,  508,    2, 0x26 /* Public | MethodCloned */,
      38,    2,  513,    2, 0x06 /* Public */,
      38,    1,  518,    2, 0x26 /* Public | MethodCloned */,
      41,    3,  521,    2, 0x06 /* Public */,
      41,    2,  528,    2, 0x26 /* Public | MethodCloned */,
      45,    2,  533,    2, 0x06 /* Public */,
      45,    1,  538,    2, 0x26 /* Public | MethodCloned */,
      46,    2,  541,    2, 0x06 /* Public */,
      46,    1,  546,    2, 0x26 /* Public | MethodCloned */,
      48,    1,  549,    2, 0x06 /* Public */,
      48,    0,  552,    2, 0x26 /* Public | MethodCloned */,
      49,    2,  553,    2, 0x06 /* Public */,
      49,    1,  558,    2, 0x26 /* Public | MethodCloned */,
      50,    3,  561,    2, 0x06 /* Public */,
      50,    2,  568,    2, 0x26 /* Public | MethodCloned */,
      54,    3,  573,    2, 0x06 /* Public */,
      54,    2,  580,    2, 0x26 /* Public | MethodCloned */,
      55,    2,  585,    2, 0x06 /* Public */,
      55,    1,  590,    2, 0x26 /* Public | MethodCloned */,
      58,    6,  593,    2, 0x06 /* Public */,
      58,    5,  606,    2, 0x26 /* Public | MethodCloned */,

 // slots: name, argc, parameters, tag, flags
      60,    6,  617,    2, 0x08 /* Private */,
      60,    5,  630,    2, 0x28 /* Private | MethodCloned */,
      61,    0,  641,    2, 0x08 /* Private */,
      62,    4,  642,    2, 0x08 /* Private */,
      62,    3,  651,    2, 0x28 /* Private | MethodCloned */,
      63,    4,  658,    2, 0x08 /* Private */,
      63,    3,  667,    2, 0x28 /* Private | MethodCloned */,
      64,    3,  674,    2, 0x08 /* Private */,
      64,    2,  681,    2, 0x28 /* Private | MethodCloned */,
      65,    2,  686,    2, 0x08 /* Private */,
      65,    1,  691,    2, 0x28 /* Private | MethodCloned */,
      66,    2,  694,    2, 0x08 /* Private */,
      66,    1,  699,    2, 0x28 /* Private | MethodCloned */,
      67,    2,  702,    2, 0x08 /* Private */,
      67,    1,  707,    2, 0x28 /* Private | MethodCloned */,
      68,    3,  710,    2, 0x08 /* Private */,
      68,    2,  717,    2, 0x28 /* Private | MethodCloned */,
      69,    2,  722,    2, 0x08 /* Private */,
      69,    1,  727,    2, 0x28 /* Private | MethodCloned */,
      70,    3,  730,    2, 0x08 /* Private */,
      70,    2,  737,    2, 0x28 /* Private | MethodCloned */,
      71,    2,  742,    2, 0x08 /* Private */,
      71,    1,  747,    2, 0x28 /* Private | MethodCloned */,
      72,    2,  750,    2, 0x08 /* Private */,
      72,    1,  755,    2, 0x28 /* Private | MethodCloned */,
      73,    1,  758,    2, 0x08 /* Private */,
      73,    0,  761,    2, 0x28 /* Private | MethodCloned */,
      74,    2,  762,    2, 0x08 /* Private */,
      74,    1,  767,    2, 0x28 /* Private | MethodCloned */,
      75,    3,  770,    2, 0x08 /* Private */,
      75,    2,  777,    2, 0x28 /* Private | MethodCloned */,
      76,    3,  782,    2, 0x08 /* Private */,
      76,    2,  789,    2, 0x28 /* Private | MethodCloned */,
      77,    2,  794,    2, 0x08 /* Private */,
      77,    1,  799,    2, 0x28 /* Private | MethodCloned */,
      78,    6,  802,    2, 0x08 /* Private */,
      78,    5,  815,    2, 0x28 /* Private | MethodCloned */,
      79,    0,  826,    2, 0x0a /* Public */,
      80,    0,  827,    2, 0x0a /* Public */,
      81,    0,  828,    2, 0x0a /* Public */,
      82,    0,  829,    2, 0x0a /* Public */,
      83,    0,  830,    2, 0x0a /* Public */,
      84,    0,  831,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5, 0x80000000 | 3, 0x80000000 | 8, 0x80000000 | 10, 0x80000000 | 12,    4,    6,    7,    9,   11,   13,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5, 0x80000000 | 3, 0x80000000 | 8, 0x80000000 | 10,    4,    6,    7,    9,   11,
    QMetaType::Void, 0x80000000 | 15, QMetaType::Int, QMetaType::Int, 0x80000000 | 12,   16,   17,   18,   13,
    QMetaType::Void, 0x80000000 | 15, QMetaType::Int, QMetaType::Int,   16,   17,   18,
    QMetaType::Void, 0x80000000 | 15, 0x80000000 | 20, 0x80000000 | 22, 0x80000000 | 12,   16,   21,   23,   13,
    QMetaType::Void, 0x80000000 | 15, 0x80000000 | 20, 0x80000000 | 22,   16,   21,   23,
    QMetaType::Void, 0x80000000 | 15, 0x80000000 | 25, 0x80000000 | 12,   16,   26,   13,
    QMetaType::Void, 0x80000000 | 15, 0x80000000 | 25,   16,   26,
    QMetaType::Void, 0x80000000 | 28, 0x80000000 | 12,   29,   13,
    QMetaType::Void, 0x80000000 | 28,   29,
    QMetaType::Void, 0x80000000 | 5, 0x80000000 | 12,   31,   13,
    QMetaType::Void, 0x80000000 | 5,   31,
    QMetaType::Void, 0x80000000 | 33, 0x80000000 | 12,   34,   13,
    QMetaType::Void, 0x80000000 | 33,   34,
    QMetaType::Void, 0x80000000 | 28, 0x80000000 | 28, 0x80000000 | 12,   36,   37,   13,
    QMetaType::Void, 0x80000000 | 28, 0x80000000 | 28,   36,   37,
    QMetaType::Void, 0x80000000 | 39, 0x80000000 | 12,   40,   13,
    QMetaType::Void, 0x80000000 | 39,   40,
    QMetaType::Void, 0x80000000 | 42, 0x80000000 | 39, 0x80000000 | 12,   43,   44,   13,
    QMetaType::Void, 0x80000000 | 42, 0x80000000 | 39,   43,   44,
    QMetaType::Void, 0x80000000 | 42, 0x80000000 | 12,   43,   13,
    QMetaType::Void, 0x80000000 | 42,   43,
    QMetaType::Void, 0x80000000 | 15, 0x80000000 | 12,   16,   47,
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void, 0x80000000 | 12,   13,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 20, 0x80000000 | 12,   21,   13,
    QMetaType::Void, 0x80000000 | 20,   21,
    QMetaType::Void, 0x80000000 | 51, QMetaType::Int, 0x80000000 | 12,   52,   53,   13,
    QMetaType::Void, 0x80000000 | 51, QMetaType::Int,   52,   53,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, 0x80000000 | 12,   36,   37,   13,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   36,   37,
    QMetaType::Void, 0x80000000 | 56, 0x80000000 | 12,   57,   13,
    QMetaType::Void, 0x80000000 | 56,   57,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, 0x80000000 | 12,   36,   37,   17,   18,   59,   13,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,   36,   37,   17,   18,   59,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5, 0x80000000 | 3, 0x80000000 | 8, 0x80000000 | 10, 0x80000000 | 12,    4,    6,    7,    9,   11,   13,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5, 0x80000000 | 3, 0x80000000 | 8, 0x80000000 | 10,    4,    6,    7,    9,   11,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 15, QMetaType::Int, QMetaType::Int, 0x80000000 | 12,   16,   17,   18,   13,
    QMetaType::Void, 0x80000000 | 15, QMetaType::Int, QMetaType::Int,   16,   17,   18,
    QMetaType::Void, 0x80000000 | 15, 0x80000000 | 20, 0x80000000 | 22, 0x80000000 | 12,   16,   21,   23,   13,
    QMetaType::Void, 0x80000000 | 15, 0x80000000 | 20, 0x80000000 | 22,   16,   21,   23,
    QMetaType::Void, 0x80000000 | 15, 0x80000000 | 25, 0x80000000 | 12,   16,   26,   13,
    QMetaType::Void, 0x80000000 | 15, 0x80000000 | 25,   16,   26,
    QMetaType::Void, 0x80000000 | 28, 0x80000000 | 12,   29,   13,
    QMetaType::Void, 0x80000000 | 28,   29,
    QMetaType::Void, 0x80000000 | 5, 0x80000000 | 12,   31,   13,
    QMetaType::Void, 0x80000000 | 5,   31,
    QMetaType::Void, 0x80000000 | 33, 0x80000000 | 12,   34,   13,
    QMetaType::Void, 0x80000000 | 33,   34,
    QMetaType::Void, 0x80000000 | 28, 0x80000000 | 28, 0x80000000 | 12,   36,   37,   13,
    QMetaType::Void, 0x80000000 | 28, 0x80000000 | 28,   36,   37,
    QMetaType::Void, 0x80000000 | 39, 0x80000000 | 12,   40,   13,
    QMetaType::Void, 0x80000000 | 39,   40,
    QMetaType::Void, 0x80000000 | 42, 0x80000000 | 39, 0x80000000 | 12,   43,   44,   13,
    QMetaType::Void, 0x80000000 | 42, 0x80000000 | 39,   43,   44,
    QMetaType::Void, 0x80000000 | 42, 0x80000000 | 12,   43,   13,
    QMetaType::Void, 0x80000000 | 42,   43,
    QMetaType::Void, 0x80000000 | 15, 0x80000000 | 12,   16,   47,
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void, 0x80000000 | 12,   13,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 20, 0x80000000 | 12,   21,   13,
    QMetaType::Void, 0x80000000 | 20,   21,
    QMetaType::Void, 0x80000000 | 51, QMetaType::Int, 0x80000000 | 12,   52,   53,   13,
    QMetaType::Void, 0x80000000 | 51, QMetaType::Int,   52,   53,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, 0x80000000 | 12,   36,   37,   13,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   36,   37,
    QMetaType::Void, 0x80000000 | 56, 0x80000000 | 12,   57,   13,
    QMetaType::Void, 0x80000000 | 56,   57,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, 0x80000000 | 12,   36,   37,   17,   18,   59,   13,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,   36,   37,   17,   18,   59,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void EmulatorWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EmulatorWindow *_t = static_cast<EmulatorWindow *>(_o);
        switch (_id) {
        case 0: _t->blit((*reinterpret_cast< QImage*(*)>(_a[1])),(*reinterpret_cast< QRect*(*)>(_a[2])),(*reinterpret_cast< QImage*(*)>(_a[3])),(*reinterpret_cast< QPoint*(*)>(_a[4])),(*reinterpret_cast< QPainter::CompositionMode*(*)>(_a[5])),(*reinterpret_cast< QSemaphore*(*)>(_a[6]))); break;
        case 1: _t->blit((*reinterpret_cast< QImage*(*)>(_a[1])),(*reinterpret_cast< QRect*(*)>(_a[2])),(*reinterpret_cast< QImage*(*)>(_a[3])),(*reinterpret_cast< QPoint*(*)>(_a[4])),(*reinterpret_cast< QPainter::CompositionMode*(*)>(_a[5]))); break;
        case 2: _t->createBitmap((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< QSemaphore*(*)>(_a[4]))); break;
        case 3: _t->createBitmap((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 4: _t->fill((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< const QRect*(*)>(_a[2])),(*reinterpret_cast< const QColor*(*)>(_a[3])),(*reinterpret_cast< QSemaphore*(*)>(_a[4]))); break;
        case 5: _t->fill((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< const QRect*(*)>(_a[2])),(*reinterpret_cast< const QColor*(*)>(_a[3]))); break;
        case 6: _t->getBitmapInfo((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< SkinSurfacePixels*(*)>(_a[2])),(*reinterpret_cast< QSemaphore*(*)>(_a[3]))); break;
        case 7: _t->getBitmapInfo((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< SkinSurfacePixels*(*)>(_a[2]))); break;
        case 8: _t->getMonitorDpi((*reinterpret_cast< int*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 9: _t->getMonitorDpi((*reinterpret_cast< int*(*)>(_a[1]))); break;
        case 10: _t->getScreenDimensions((*reinterpret_cast< QRect*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 11: _t->getScreenDimensions((*reinterpret_cast< QRect*(*)>(_a[1]))); break;
        case 12: _t->getWindowId((*reinterpret_cast< WId*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 13: _t->getWindowId((*reinterpret_cast< WId*(*)>(_a[1]))); break;
        case 14: _t->getWindowPos((*reinterpret_cast< int*(*)>(_a[1])),(*reinterpret_cast< int*(*)>(_a[2])),(*reinterpret_cast< QSemaphore*(*)>(_a[3]))); break;
        case 15: _t->getWindowPos((*reinterpret_cast< int*(*)>(_a[1])),(*reinterpret_cast< int*(*)>(_a[2]))); break;
        case 16: _t->isWindowFullyVisible((*reinterpret_cast< bool*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 17: _t->isWindowFullyVisible((*reinterpret_cast< bool*(*)>(_a[1]))); break;
        case 18: _t->pollEvent((*reinterpret_cast< SkinEvent*(*)>(_a[1])),(*reinterpret_cast< bool*(*)>(_a[2])),(*reinterpret_cast< QSemaphore*(*)>(_a[3]))); break;
        case 19: _t->pollEvent((*reinterpret_cast< SkinEvent*(*)>(_a[1])),(*reinterpret_cast< bool*(*)>(_a[2]))); break;
        case 20: _t->queueEvent((*reinterpret_cast< SkinEvent*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 21: _t->queueEvent((*reinterpret_cast< SkinEvent*(*)>(_a[1]))); break;
        case 22: _t->releaseBitmap((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 23: _t->releaseBitmap((*reinterpret_cast< SkinSurface*(*)>(_a[1]))); break;
        case 24: _t->requestClose((*reinterpret_cast< QSemaphore*(*)>(_a[1]))); break;
        case 25: _t->requestClose(); break;
        case 26: _t->requestUpdate((*reinterpret_cast< const QRect*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 27: _t->requestUpdate((*reinterpret_cast< const QRect*(*)>(_a[1]))); break;
        case 28: _t->setWindowIcon((*reinterpret_cast< const unsigned char*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< QSemaphore*(*)>(_a[3]))); break;
        case 29: _t->setWindowIcon((*reinterpret_cast< const unsigned char*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 30: _t->setWindowPos((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< QSemaphore*(*)>(_a[3]))); break;
        case 31: _t->setWindowPos((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 32: _t->setTitle((*reinterpret_cast< const QString*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 33: _t->setTitle((*reinterpret_cast< const QString*(*)>(_a[1]))); break;
        case 34: _t->showWindow((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5])),(*reinterpret_cast< QSemaphore*(*)>(_a[6]))); break;
        case 35: _t->showWindow((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5]))); break;
        case 36: _t->slot_blit((*reinterpret_cast< QImage*(*)>(_a[1])),(*reinterpret_cast< QRect*(*)>(_a[2])),(*reinterpret_cast< QImage*(*)>(_a[3])),(*reinterpret_cast< QPoint*(*)>(_a[4])),(*reinterpret_cast< QPainter::CompositionMode*(*)>(_a[5])),(*reinterpret_cast< QSemaphore*(*)>(_a[6]))); break;
        case 37: _t->slot_blit((*reinterpret_cast< QImage*(*)>(_a[1])),(*reinterpret_cast< QRect*(*)>(_a[2])),(*reinterpret_cast< QImage*(*)>(_a[3])),(*reinterpret_cast< QPoint*(*)>(_a[4])),(*reinterpret_cast< QPainter::CompositionMode*(*)>(_a[5]))); break;
        case 38: _t->slot_clearInstance(); break;
        case 39: _t->slot_createBitmap((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< QSemaphore*(*)>(_a[4]))); break;
        case 40: _t->slot_createBitmap((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 41: _t->slot_fill((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< const QRect*(*)>(_a[2])),(*reinterpret_cast< const QColor*(*)>(_a[3])),(*reinterpret_cast< QSemaphore*(*)>(_a[4]))); break;
        case 42: _t->slot_fill((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< const QRect*(*)>(_a[2])),(*reinterpret_cast< const QColor*(*)>(_a[3]))); break;
        case 43: _t->slot_getBitmapInfo((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< SkinSurfacePixels*(*)>(_a[2])),(*reinterpret_cast< QSemaphore*(*)>(_a[3]))); break;
        case 44: _t->slot_getBitmapInfo((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< SkinSurfacePixels*(*)>(_a[2]))); break;
        case 45: _t->slot_getMonitorDpi((*reinterpret_cast< int*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 46: _t->slot_getMonitorDpi((*reinterpret_cast< int*(*)>(_a[1]))); break;
        case 47: _t->slot_getScreenDimensions((*reinterpret_cast< QRect*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 48: _t->slot_getScreenDimensions((*reinterpret_cast< QRect*(*)>(_a[1]))); break;
        case 49: _t->slot_getWindowId((*reinterpret_cast< WId*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 50: _t->slot_getWindowId((*reinterpret_cast< WId*(*)>(_a[1]))); break;
        case 51: _t->slot_getWindowPos((*reinterpret_cast< int*(*)>(_a[1])),(*reinterpret_cast< int*(*)>(_a[2])),(*reinterpret_cast< QSemaphore*(*)>(_a[3]))); break;
        case 52: _t->slot_getWindowPos((*reinterpret_cast< int*(*)>(_a[1])),(*reinterpret_cast< int*(*)>(_a[2]))); break;
        case 53: _t->slot_isWindowFullyVisible((*reinterpret_cast< bool*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 54: _t->slot_isWindowFullyVisible((*reinterpret_cast< bool*(*)>(_a[1]))); break;
        case 55: _t->slot_pollEvent((*reinterpret_cast< SkinEvent*(*)>(_a[1])),(*reinterpret_cast< bool*(*)>(_a[2])),(*reinterpret_cast< QSemaphore*(*)>(_a[3]))); break;
        case 56: _t->slot_pollEvent((*reinterpret_cast< SkinEvent*(*)>(_a[1])),(*reinterpret_cast< bool*(*)>(_a[2]))); break;
        case 57: _t->slot_queueEvent((*reinterpret_cast< SkinEvent*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 58: _t->slot_queueEvent((*reinterpret_cast< SkinEvent*(*)>(_a[1]))); break;
        case 59: _t->slot_releaseBitmap((*reinterpret_cast< SkinSurface*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 60: _t->slot_releaseBitmap((*reinterpret_cast< SkinSurface*(*)>(_a[1]))); break;
        case 61: _t->slot_requestClose((*reinterpret_cast< QSemaphore*(*)>(_a[1]))); break;
        case 62: _t->slot_requestClose(); break;
        case 63: _t->slot_requestUpdate((*reinterpret_cast< const QRect*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 64: _t->slot_requestUpdate((*reinterpret_cast< const QRect*(*)>(_a[1]))); break;
        case 65: _t->slot_setWindowIcon((*reinterpret_cast< const unsigned char*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< QSemaphore*(*)>(_a[3]))); break;
        case 66: _t->slot_setWindowIcon((*reinterpret_cast< const unsigned char*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 67: _t->slot_setWindowPos((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< QSemaphore*(*)>(_a[3]))); break;
        case 68: _t->slot_setWindowPos((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 69: _t->slot_setWindowTitle((*reinterpret_cast< const QString*(*)>(_a[1])),(*reinterpret_cast< QSemaphore*(*)>(_a[2]))); break;
        case 70: _t->slot_setWindowTitle((*reinterpret_cast< const QString*(*)>(_a[1]))); break;
        case 71: _t->slot_showWindow((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5])),(*reinterpret_cast< QSemaphore*(*)>(_a[6]))); break;
        case 72: _t->slot_showWindow((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5]))); break;
        case 73: _t->slot_power(); break;
        case 74: _t->slot_rotate(); break;
        case 75: _t->slot_volumeUp(); break;
        case 76: _t->slot_volumeDown(); break;
        case 77: _t->slot_zoom(); break;
        case 78: _t->slot_fullscreen(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (EmulatorWindow::*_t)(QImage * , QRect * , QImage * , QPoint * , QPainter::CompositionMode * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::blit)) {
                *result = 0;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(SkinSurface * , int , int , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::createBitmap)) {
                *result = 2;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(SkinSurface * , const QRect * , const QColor * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::fill)) {
                *result = 4;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(SkinSurface * , SkinSurfacePixels * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::getBitmapInfo)) {
                *result = 6;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(int * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::getMonitorDpi)) {
                *result = 8;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(QRect * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::getScreenDimensions)) {
                *result = 10;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(WId * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::getWindowId)) {
                *result = 12;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(int * , int * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::getWindowPos)) {
                *result = 14;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(bool * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::isWindowFullyVisible)) {
                *result = 16;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(SkinEvent * , bool * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::pollEvent)) {
                *result = 18;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(SkinEvent * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::queueEvent)) {
                *result = 20;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(SkinSurface * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::releaseBitmap)) {
                *result = 22;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::requestClose)) {
                *result = 24;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(const QRect * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::requestUpdate)) {
                *result = 26;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(const unsigned char * , int , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::setWindowIcon)) {
                *result = 28;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(int , int , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::setWindowPos)) {
                *result = 30;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(const QString * , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::setTitle)) {
                *result = 32;
            }
        }
        {
            typedef void (EmulatorWindow::*_t)(int , int , int , int , int , QSemaphore * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EmulatorWindow::showWindow)) {
                *result = 34;
            }
        }
    }
}

const QMetaObject EmulatorWindow::staticMetaObject = {
    { &QFrame::staticMetaObject, qt_meta_stringdata_EmulatorWindow.data,
      qt_meta_data_EmulatorWindow,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EmulatorWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EmulatorWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EmulatorWindow.stringdata))
        return static_cast<void*>(const_cast< EmulatorWindow*>(this));
    return QFrame::qt_metacast(_clname);
}

int EmulatorWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 79)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 79;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 79)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 79;
    }
    return _id;
}

// SIGNAL 0
void EmulatorWindow::blit(QImage * _t1, QRect * _t2, QImage * _t3, QPoint * _t4, QPainter::CompositionMode * _t5, QSemaphore * _t6)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)), const_cast<void*>(reinterpret_cast<const void*>(&_t6)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 2
void EmulatorWindow::createBitmap(SkinSurface * _t1, int _t2, int _t3, QSemaphore * _t4)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 4
void EmulatorWindow::fill(SkinSurface * _t1, const QRect * _t2, const QColor * _t3, QSemaphore * _t4)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 6
void EmulatorWindow::getBitmapInfo(SkinSurface * _t1, SkinSurfacePixels * _t2, QSemaphore * _t3)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 8
void EmulatorWindow::getMonitorDpi(int * _t1, QSemaphore * _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 10
void EmulatorWindow::getScreenDimensions(QRect * _t1, QSemaphore * _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 12
void EmulatorWindow::getWindowId(WId * _t1, QSemaphore * _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 14
void EmulatorWindow::getWindowPos(int * _t1, int * _t2, QSemaphore * _t3)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 16
void EmulatorWindow::isWindowFullyVisible(bool * _t1, QSemaphore * _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 18
void EmulatorWindow::pollEvent(SkinEvent * _t1, bool * _t2, QSemaphore * _t3)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 18, _a);
}

// SIGNAL 20
void EmulatorWindow::queueEvent(SkinEvent * _t1, QSemaphore * _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 20, _a);
}

// SIGNAL 22
void EmulatorWindow::releaseBitmap(SkinSurface * _t1, QSemaphore * _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 22, _a);
}

// SIGNAL 24
void EmulatorWindow::requestClose(QSemaphore * _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 24, _a);
}

// SIGNAL 26
void EmulatorWindow::requestUpdate(const QRect * _t1, QSemaphore * _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 26, _a);
}

// SIGNAL 28
void EmulatorWindow::setWindowIcon(const unsigned char * _t1, int _t2, QSemaphore * _t3)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 28, _a);
}

// SIGNAL 30
void EmulatorWindow::setWindowPos(int _t1, int _t2, QSemaphore * _t3)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 30, _a);
}

// SIGNAL 32
void EmulatorWindow::setTitle(const QString * _t1, QSemaphore * _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 32, _a);
}

// SIGNAL 34
void EmulatorWindow::showWindow(int _t1, int _t2, int _t3, int _t4, int _t5, QSemaphore * _t6)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)), const_cast<void*>(reinterpret_cast<const void*>(&_t6)) };
    QMetaObject::activate(this, &staticMetaObject, 34, _a);
}
QT_END_MOC_NAMESPACE
