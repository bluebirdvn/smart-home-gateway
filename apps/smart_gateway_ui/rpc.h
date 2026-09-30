#ifndef RPC_H
#define RPC_H

#include <QDBusVirtualObject>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QMutex>
#include <QMap>
#include <QString>
#include "ipc.h"

class QDbusRpc : public QDBusVirtualObject
{
    Q_OBJECT
public:
    explicit QDbusRpc(QObject *parent = nullptr);

    void registerMethod(const QString& method, RpcCallback callback);
    void unregisterMethod(const QString& method);
    bool hasAnyMethod() const;
    
    bool handleMessage(const QDBusMessage& message, const QDBusConnection& connection) override;
    QString introspect(const QString& path) const override;

private:
    mutable QMutex lock;
    QMap<QString, RpcCallback> callbacks;
};

#endif 