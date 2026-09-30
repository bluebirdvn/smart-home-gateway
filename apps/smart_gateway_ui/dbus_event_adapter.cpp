#include "dbus_event_adapter.h"
#include <QVariant>
#include <QDBusVariant>
#include <QDebug>

QDBusEventAdapter::QDBusEventAdapter(EventCallback cb, QObject *parent)
    : QObject(parent), callback(std::move(cb))
{
}

void QDBusEventAdapter::handleSignal(const QDBusMessage& msg)
{
    if (callback == nullptr)
    {
        qWarning() << "null callback.";
        return;
    }

    IpcMessage ipcMsg;
    ipcMsg.payload = "{}";

    const QList<QVariant>& args = msg.arguments();
    
    if (args.isEmpty())
    {
        qWarning() << "[QDBusEventAdapter] null arg in signal";
        callback(ipcMsg);
        return;
    }

    QVariant firstArg = args.at(0);

    if (firstArg.canConvert<QDBusVariant>())
    {
        QDBusVariant dbusVar = firstArg.value<QDBusVariant>();
        ipcMsg.payload = dbusVar.variant().toString();
    }

    if (ipcMsg.payload.isEmpty())
    {
        ipcMsg.payload = "{}";
        qWarning() << "can't parse payload, defaulted to '{}'";
    }

    qDebug().noquote() << "raw payload: " << ipcMsg.payload;
    
    callback(ipcMsg);
}