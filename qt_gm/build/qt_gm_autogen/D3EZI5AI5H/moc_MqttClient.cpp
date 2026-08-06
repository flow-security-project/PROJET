/****************************************************************************
** Meta object code from reading C++ file 'MqttClient.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../engine/mqtt/MqttClient.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MqttClient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_MqttClient_t {
    uint offsetsAndSizes[58];
    char stringdata0[11];
    char stringdata1[13];
    char stringdata2[1];
    char stringdata3[18];
    char stringdata4[6];
    char stringdata5[13];
    char stringdata6[18];
    char stringdata7[6];
    char stringdata8[16];
    char stringdata9[6];
    char stringdata10[8];
    char stringdata11[12];
    char stringdata12[12];
    char stringdata13[15];
    char stringdata14[14];
    char stringdata15[6];
    char stringdata16[13];
    char stringdata17[11];
    char stringdata18[10];
    char stringdata19[6];
    char stringdata20[8];
    char stringdata21[23];
    char stringdata22[11];
    char stringdata23[18];
    char stringdata24[22];
    char stringdata25[14];
    char stringdata26[17];
    char stringdata27[18];
    char stringdata28[13];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_MqttClient_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_MqttClient_t qt_meta_stringdata_MqttClient = {
    {
        QT_MOC_LITERAL(0, 10),  // "MqttClient"
        QT_MOC_LITERAL(11, 12),  // "stateChanged"
        QT_MOC_LITERAL(24, 0),  // ""
        QT_MOC_LITERAL(25, 17),  // "MqttClient::State"
        QT_MOC_LITERAL(43, 5),  // "state"
        QT_MOC_LITERAL(49, 12),  // "errorChanged"
        QT_MOC_LITERAL(62, 17),  // "MqttClient::Error"
        QT_MOC_LITERAL(80, 5),  // "error"
        QT_MOC_LITERAL(86, 15),  // "messageReceived"
        QT_MOC_LITERAL(102, 5),  // "topic"
        QT_MOC_LITERAL(108, 7),  // "payload"
        QT_MOC_LITERAL(116, 11),  // "onConnected"
        QT_MOC_LITERAL(128, 11),  // "onReadyRead"
        QT_MOC_LITERAL(140, 14),  // "onDisconnected"
        QT_MOC_LITERAL(155, 13),  // "onPingTimeout"
        QT_MOC_LITERAL(169, 5),  // "State"
        QT_MOC_LITERAL(175, 12),  // "Disconnected"
        QT_MOC_LITERAL(188, 10),  // "Connecting"
        QT_MOC_LITERAL(199, 9),  // "Connected"
        QT_MOC_LITERAL(209, 5),  // "Error"
        QT_MOC_LITERAL(215, 7),  // "NoError"
        QT_MOC_LITERAL(223, 22),  // "InvalidProtocolVersion"
        QT_MOC_LITERAL(246, 10),  // "IdRejected"
        QT_MOC_LITERAL(257, 17),  // "ServerUnavailable"
        QT_MOC_LITERAL(275, 21),  // "BadUsernameOrPassword"
        QT_MOC_LITERAL(297, 13),  // "NotAuthorized"
        QT_MOC_LITERAL(311, 16),  // "TransportInvalid"
        QT_MOC_LITERAL(328, 17),  // "ProtocolViolation"
        QT_MOC_LITERAL(346, 12)   // "UnknownError"
    },
    "MqttClient",
    "stateChanged",
    "",
    "MqttClient::State",
    "state",
    "errorChanged",
    "MqttClient::Error",
    "error",
    "messageReceived",
    "topic",
    "payload",
    "onConnected",
    "onReadyRead",
    "onDisconnected",
    "onPingTimeout",
    "State",
    "Disconnected",
    "Connecting",
    "Connected",
    "Error",
    "NoError",
    "InvalidProtocolVersion",
    "IdRejected",
    "ServerUnavailable",
    "BadUsernameOrPassword",
    "NotAuthorized",
    "TransportInvalid",
    "ProtocolViolation",
    "UnknownError"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_MqttClient[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       2,   71, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   56,    2, 0x06,    1 /* Public */,
       5,    1,   59,    2, 0x06,    3 /* Public */,
       8,    2,   62,    2, 0x06,    5 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      11,    0,   67,    2, 0x08,    8 /* Private */,
      12,    0,   68,    2, 0x08,    9 /* Private */,
      13,    0,   69,    2, 0x08,   10 /* Private */,
      14,    0,   70,    2, 0x08,   11 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, QMetaType::QString, QMetaType::QByteArray,    9,   10,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // enums: name, alias, flags, count, data
      15,   15, 0x0,    3,   81,
      19,   19, 0x0,    9,   87,

 // enum data: key, value
      16, uint(MqttClient::Disconnected),
      17, uint(MqttClient::Connecting),
      18, uint(MqttClient::Connected),
      20, uint(MqttClient::NoError),
      21, uint(MqttClient::InvalidProtocolVersion),
      22, uint(MqttClient::IdRejected),
      23, uint(MqttClient::ServerUnavailable),
      24, uint(MqttClient::BadUsernameOrPassword),
      25, uint(MqttClient::NotAuthorized),
      26, uint(MqttClient::TransportInvalid),
      27, uint(MqttClient::ProtocolViolation),
      28, uint(MqttClient::UnknownError),

       0        // eod
};

Q_CONSTINIT const QMetaObject MqttClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_MqttClient.offsetsAndSizes,
    qt_meta_data_MqttClient,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_MqttClient_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MqttClient, std::true_type>,
        // method 'stateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<MqttClient::State, std::false_type>,
        // method 'errorChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<MqttClient::Error, std::false_type>,
        // method 'messageReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>,
        // method 'onConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPingTimeout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MqttClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MqttClient *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->stateChanged((*reinterpret_cast< std::add_pointer_t<MqttClient::State>>(_a[1]))); break;
        case 1: _t->errorChanged((*reinterpret_cast< std::add_pointer_t<MqttClient::Error>>(_a[1]))); break;
        case 2: _t->messageReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[2]))); break;
        case 3: _t->onConnected(); break;
        case 4: _t->onReadyRead(); break;
        case 5: _t->onDisconnected(); break;
        case 6: _t->onPingTimeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MqttClient::*)(MqttClient::State );
            if (_t _q_method = &MqttClient::stateChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MqttClient::*)(MqttClient::Error );
            if (_t _q_method = &MqttClient::errorChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MqttClient::*)(const QString & , const QByteArray & );
            if (_t _q_method = &MqttClient::messageReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *MqttClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MqttClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MqttClient.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MqttClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void MqttClient::stateChanged(MqttClient::State _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MqttClient::errorChanged(MqttClient::Error _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void MqttClient::messageReceived(const QString & _t1, const QByteArray & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
