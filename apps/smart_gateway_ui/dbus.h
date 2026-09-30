#ifndef DBUS_H
#define DBUS_H

#include <QObject>
#include <QList>
#include "ipc.h"
#include "ipc_message.h"
#include "config_manager.h"

class QDBusEventAdapter;
class QDbusRpc;

class QDbusImpl : public IIpc
{
    Q_OBJECT

public:
    explicit QDbusImpl(const DBusConfig& config, QObject *parent = nullptr);
    ~QDbusImpl() override;

    bool init() override;
    void deinit() override;
    bool publish(const QString& topic, const IpcMessage& msg) override;
    void subscribe(const IpcEndpoint& endpoint, EventCallback callback) override;
    IpcMessage call(const IpcEndpoint& endpoint, const IpcMessage& request) override;
    void expose(const QString& endpoint, RpcCallback callback) override;

private:
    DBusConfig dbus_config;
    QList<QDBusEventAdapter*> event_adapter;
    QDbusRpc* rpc = nullptr;
};

#endif 
