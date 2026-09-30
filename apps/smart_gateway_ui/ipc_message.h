#ifndef IPC_MESSAGE_H
#define IPC_MESSAGE_H

#include <QString>
#include <QObject>

struct IpcMessage {
    QString payload;
};

using RpcCallback = std::function<IpcMessage(const IpcMessage& request)>;
using EventCallback = std::function<void(const IpcMessage& event)>;

#endif
