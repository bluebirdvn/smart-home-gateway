#include "dbus.h"
#include "dbus_event_adapter.h"
#include "rpc.h"
#include <QDBusConnection>
#include <QDBusError>
#include <QVariant>
#include <QDBusVariant>
#include <QDebug>

QDbusImpl::QDbusImpl(const DBusConfig& config, QObject *parent)
    : IIpc(parent), dbus_config(config)
{
}

QDbusImpl::~QDbusImpl()
{
    deinit();
}

bool QDbusImpl::init()
{
    if (dbus_config.serviceName.isEmpty())
    {
        qWarning() << "service name empty";
        return false;
    }
    
    if (dbus_config.objectPath.isEmpty())
    {
        qWarning() << "object path empty";
        return false;
    }
    
    if (dbus_config.interfaceName.isEmpty())
    {
        qWarning() << "interface name empty";
        return false;
    }

    QDBusConnection bus = QDBusConnection::systemBus();
    
    if (!bus.isConnected())
    {
        qWarning() << "can't connect to dbus";
        return false;
    }

    if (!bus.registerService(dbus_config.serviceName))
    {
        qWarning() << "register service failed" << bus.lastError().message();
        return false;
    }

    if (!ConfigManager::getInstance().isLoaded())
    {
        qWarning() << "config manager load failed";
    }

    qInfo() << "registered:" << dbus_config.serviceName;
    return true;
}

void QDbusImpl::deinit()
{
    QDBusConnection bus = QDBusConnection::systemBus();

    if (bus.isConnected())
    {
        for (QDBusEventAdapter* adapter : event_adapter)
        {
            bool ok = bus.disconnect(adapter->subService, QString(), adapter->subInterface, adapter->subMethod, adapter, SLOT(handleSignal(QDBusMessage)));
            if (!ok)
            {
                qWarning() << "disconnected failed";
            }
        }

        if (rpc)
        {
            bus.unregisterObject(dbus_config.objectPath);
            qInfo() << "unresgister:" << dbus_config.objectPath;
        }

        if (!dbus_config.serviceName.isEmpty())
        {
            bus.unregisterService(dbus_config.serviceName);
            qInfo() << "return sevice name:" << dbus_config.serviceName;
        }
    }

    qDeleteAll(event_adapter);
    event_adapter.clear();

    if (rpc)
    {
        delete rpc;
        rpc = nullptr;
    }

    qInfo() << "[IPC] Đã đóng IPC connection";
}

bool QDbusImpl::publish(const QString& topic, const IpcMessage& msg)
{
    if (topic.isEmpty())
    {
        qWarning() << "empty topic.";
        return false;
    }

    QDBusMessage dbus_msg = QDBusMessage::createSignal(dbus_config.objectPath, dbus_config.interfaceName, topic);

    QString jsonPayload = msg.payload.isEmpty() ? QString("{}") : msg.payload;
    dbus_msg << QVariant::fromValue(QDBusVariant(jsonPayload));

    bool ok = QDBusConnection::systemBus().send(dbus_msg);
    
    if (!ok)
    {
        qWarning() << "publish failed:" << topic;
    }
    
    return ok;
}

void QDbusImpl::subscribe(const IpcEndpoint& endpoint, EventCallback callback)
{
    if (!callback)
    {
        qWarning() << "NULL callback";
        return;
    }
    
    if (endpoint.method.isEmpty())
    {
        qWarning() << "empty method";
        return;
    }

    DBusConfig targetConfig = ConfigManager::getInstance().getConfig(endpoint.moduleName);
    
    if (targetConfig.serviceName.isEmpty())
    {
        qWarning() << "not found module" << endpoint.moduleName;
        QString objectPath;
        if (endpoint.moduleName == "Db") {
            objectPath = "/com/gateway/db";
        } else if (endpoint.moduleName == "Mesh") {
            objectPath = "/com/gateway/mesh";
        } else if (endpoint.moduleName == "Mqtt") {
            objectPath = "/com/gateway/mqtt";
        }

        qDebug() << "use alternative module";
        // return;
    }

    const QString& interfaceToUse = endpoint.interface.isEmpty() ? targetConfig.interfaceName : endpoint.interface;

    QDBusEventAdapter* adapter = new QDBusEventAdapter(std::move(callback), this);

    bool success = QDBusConnection::systemBus().connect(targetConfig.serviceName, targetConfig.objectPath, interfaceToUse, endpoint.method, adapter, SLOT(handleSignal(QDBusMessage)));

    if (!success)
    {
        qWarning() << "connected failed";
        delete adapter;
        return;
    }

    adapter->subService = targetConfig.serviceName;
    adapter->subInterface = interfaceToUse;
    adapter->subMethod = endpoint.method;

    event_adapter.append(adapter);

    qDebug() << "subscribe success";
}

IpcMessage QDbusImpl::call(const IpcEndpoint& ep, const IpcMessage& req)
{
    IpcMessage result;
    result.payload = "{}";

    QDBusConnection bus = QDBusConnection::systemBus();

    if (!bus.isConnected())
    {
        qWarning() << "calling connected failed";
        return result;
    }

    if (ep.moduleName.isEmpty() || ep.method.isEmpty())
    {
        qWarning() << "empty module";
        return result;
    }

    DBusConfig cfg = ConfigManager::getInstance().getConfig(ep.moduleName);
    
    if (cfg.serviceName.isEmpty())
    {
        qWarning() << "service empty:" << ep.moduleName;
        return result;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(cfg.serviceName, cfg.objectPath, cfg.interfaceName, ep.method);

    QString jsonPayload = req.payload.isEmpty() ? QString("{}") : req.payload;
    msg << QVariant::fromValue(QDBusVariant(jsonPayload));

    QDBusMessage reply = bus.call(msg, QDBus::Block, 5000);

    if (reply.type() == QDBusMessage::ErrorMessage)
    {
        qWarning() << "call failed.";
        return result;
    }

    if (reply.type() != QDBusMessage::ReplyMessage)
    {
        qWarning() << "Unexpected reply type:" << reply.type();
        return result;
    }

    if (!reply.arguments().isEmpty())
    {
        QVariant arg = reply.arguments().first();

        if (arg.canConvert<QDBusVariant>())
        {
            result.payload = arg.value<QDBusVariant>().variant().toString();
        }
        else if (arg.canConvert<QString>())
        {
            result.payload = arg.toString();
        }
        else
        {
            qWarning() << "can't process type reply";
        }
    }

    if (result.payload.isEmpty())
    {
        result.payload = "{}";
    }

    return result;
}

void QDbusImpl::expose(const QString& endpoint, RpcCallback callback)
{
    if (endpoint.isEmpty())
    {
        return;
    }
    
    if (!callback)
    {
        return;
    }
    
    if (dbus_config.objectPath.isEmpty())
    {
        return;
    }

    QDBusConnection bus = QDBusConnection::systemBus();
    
    if (!bus.isConnected())
    {
        qWarning() << "unconnected bus";
        return;
    }

    if (!rpc)
    {
        rpc = new QDbusRpc(this);

        bool registered = bus.registerVirtualObject(
            dbus_config.objectPath,
            rpc,
            QDBusConnection::SubPath
        );

        if (!registered)
        {
            qWarning() << "not registerd";
            delete rpc;
            rpc = nullptr;
            return;
        }

        qInfo() << "subscribe rpc success";
    }

    rpc->registerMethod(endpoint, std::move(callback));

    qInfo() << "expose success";
}
