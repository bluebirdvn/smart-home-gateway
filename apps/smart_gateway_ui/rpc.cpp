#include "rpc.h"
#include <QDBusError>
#include <QVariant>
#include <QDBusVariant>
#include <QDebug>
#include <QMutexLocker>
#include <QMutex>

QDbusRpc::QDbusRpc(QObject *parent)
    : QDBusVirtualObject(parent)
{
}

void QDbusRpc::registerMethod(const QString& method, RpcCallback callback)
{
    QMutexLocker locker(&lock);
    callbacks.insert(method, std::move(callback));
}

void QDbusRpc::unregisterMethod(const QString& method)
{
    QMutexLocker locker(&lock);
    callbacks.remove(method);
}

bool QDbusRpc::hasAnyMethod() const
{
    QMutexLocker locker(&lock);
    return !callbacks.isEmpty();
}

bool QDbusRpc::handleMessage(const QDBusMessage& message, const QDBusConnection& connection)
{
    if (message.type() != QDBusMessage::MethodCallMessage)
    {
        return false;
    }

    RpcCallback cb;
    {
        QMutexLocker locker(&lock);
        auto it = callbacks.find(message.member());
        
        if (it == callbacks.end())
        {
            return false;
        }
        cb = it.value();
    }

    if (!cb)
    {
        qWarning() << "[QDbusRpc] callback null for method:" << message.member();
        QDBusMessage err = message.createErrorReply(QDBusError::Failed, "RPC callback không hợp lệ");
        connection.send(err);
        return true;
    }

    IpcMessage request;
    request.payload = "{}";

    const QList<QVariant>& args = message.arguments();
    
    if (!args.isEmpty())
    {
        QVariant firstArg = args.at(0);
        
        if (firstArg.canConvert<QDBusVariant>())
        {
            request.payload = firstArg.value<QDBusVariant>().variant().toString();
        }
        else if (firstArg.canConvert<QString>())
        {
            request.payload = firstArg.toString();
        }
    }
    
    if (request.payload.isEmpty())
    {
        request.payload = "{}";
    }

    IpcMessage response;
    
    try
    {
        response = cb(request);
    }
    catch (const std::exception& e)
    {
        qWarning() << "[QDbusRpc] callback throw exception:" << e.what();
        QDBusMessage err = message.createErrorReply(QDBusError::Failed, e.what());
        connection.send(err);
        return true;
    }

    QString replyPayload = response.payload.isEmpty() ? QString("{}") : response.payload;
    QDBusMessage reply = message.createReply(QVariant::fromValue(QDBusVariant(replyPayload)));

    if (!connection.send(reply))
    {
        qWarning() << "[QDbusRpc] reply failed:" << message.member();
    }
    
    return true;
}

QString QDbusRpc::introspect(const QString& path) const
{
    Q_UNUSED(path);
    QMutexLocker locker(&lock);

    QString xml = QLatin1String(
        "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
        "    <method name=\"Introspect\">\n"
        "      <arg name=\"data\" type=\"s\" direction=\"out\"/>\n"
        "    </method>\n"
        "  </interface>\n");

    return xml;
}
