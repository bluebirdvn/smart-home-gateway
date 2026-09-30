#ifndef DBUS_EVENT_ADAPTER_H
#define DBUS_EVENT_ADAPTER_H

#include <QObject>
#include <QDBusMessage>
#include <QString>
#include "ipc.h"

/**
 * @brief receive one dbus signal and forwards it to event callback
 * one adapter is created per subscription, because qdbusconnection::connect() need a qobject with a slot to deliver the signal to
 */
class QDBusEventAdapter : public QObject
{
    Q_OBJECT
public:
    explicit QDBusEventAdapter(EventCallback cb, QObject *parent = nullptr);

    QString subService;
    QString subInterface;
    QString subMethod;

public slots:
    void handleSignal(const QDBusMessage& msg);

private:
    EventCallback callback;
};

#endif 