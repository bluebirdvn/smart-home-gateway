#ifndef IPC_H
#define IPC_H

#include <QString>
#include "ipc_message.h"
#include <QObject>

enum class DataStatus {
    OK = 0,
    ERROR,
    NOT_READY
};

struct IpcEndpoint {
    QString moduleName;
    QString interface;
    QString method;
    QString objectPath;
};

class IIpc : public QObject {
    Q_OBJECT
public:
    explicit IIpc(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IIpc() = default;

    virtual bool init() = 0;
    virtual void deinit() = 0;
    virtual bool publish(const QString& topic, const IpcMessage& msg) = 0;
    virtual void subscribe(const IpcEndpoint& endpoint, EventCallback callback) = 0;
    virtual IpcMessage call(const IpcEndpoint& endpoint, const IpcMessage& request) = 0;
    virtual void expose(const QString& endpoint, RpcCallback callback) = 0;
};

#endif
