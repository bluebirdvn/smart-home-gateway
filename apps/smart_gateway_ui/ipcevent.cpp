#include "ipcevent.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

IPCEvent::IPCEvent(IIpc* ipc, QObject *parent)
    : QObject(parent), dbus(ipc)
{

}

void IPCEvent::ipc_init()
{
    if (!dbus) {
        qWarning() << "IPC pointer is nullptr";
        return;
    }

    dbus->subscribe(IpcEndpoint{"Mesh", "com.gateway.mesh.events", "GatewayStatus", ""}, [this](const IpcMessage& msg) {
        qDebug() << "get GatewayStatus" << "\n";
        emit gatewayStatusReceive(ModuleStatusDto::from_json(msg.payload));
    });

    dbus->subscribe(IpcEndpoint{"Mqtt", "com.gateway.mqtt.events", "ServerStatus", ""}, [this](const IpcMessage& msg) {
        qDebug() << "get ServerStatus" << "\n";
        emit serverStatusReceive(ModuleStatusDto::from_json(msg.payload));
    });

    dbus->subscribe(IpcEndpoint{"Mesh", "com.gateway.mesh.events", "MeshStatus", ""}, [this](const IpcMessage& msg) {
        qDebug() << "get MeshStatus" << "\n";
        emit meshStatusReceive(ModuleStatusDto::from_json(msg.payload));
    });


    dbus->subscribe(IpcEndpoint{"Mesh", "com.gateway.mesh.events", "UnprovDeviceEvent", ""}, [this](const IpcMessage& msg) {
        qDebug() << "get UnprovDeviceEvent" << "\n";
        emit unprovDeviceDiscovery(UnprovAdvDto::from_json(msg.payload));
    });

    dbus->subscribe(IpcEndpoint{"Db", "com.gateway.db.events", "HeartbeatEvent", ""}, [this](const IpcMessage& msg) {
        qDebug() << "get HeartbeatEvent" << "\n";
        emit heartbeatReceive(HeartbeatDto::from_json(msg.payload));
    });

    dbus->subscribe(IpcEndpoint{"Db", "com.gateway.db.events", "NodeSyncEvent", ""}, [this](const IpcMessage& msg) {
        qDebug() << "get NodeSyncEvent" << "\n";
        emit nodeSyncReceive(NodeInfoDto::from_ipc(msg));
    });

    dbus->subscribe(IpcEndpoint{"Db", "com.gateway.db.events", "SensorSyncEvent", ""}, [this](const IpcMessage& msg) {
        qDebug() << "get SensorSyncEvent" << "\n";
        emit sensorSyncReceive(SensorDto::from_ipc(msg));
    });

    dbus->subscribe(IpcEndpoint{"Db", "com.gateway.db.events", "ActuatorSyncEvent", ""}, [this](const IpcMessage& msg) {
        qDebug() << "get ActuatorSyncEvent" << "\n";

        emit actuatorSyncReceive(ActuatorStatusDto::from_ipc(msg));
    });

    dbus->subscribe(IpcEndpoint{"Db", "com.gateway.db.events", "GroupSyncEvent", ""}, [this](const IpcMessage& msg) {
        qDebug() << "get GroupSyncEvent" << "\n";
        emit groupsSyncReceive(GroupSyncListDto::from_json(msg.payload));
    });
}

void IPCEvent::sendActuatorCmd(ActuatorCmdDto dto) {
    if (dbus) {
        qDebug() << "send SendActuatorCmd" << "\n";
        dbus->publish("UiActuatorCmd", dto.to_ipc());
    }
}

void IPCEvent::deleteNode(DeleteNodeDto dto) {
    if (dbus) {
        qDebug() << "send DeleteNodeCmd" << "\n";
        dbus->publish("UiDeleteNodeCmd", dto.to_ipc());
    }
}

void IPCEvent::sendUuidWhitelist(UuidWhitelistDto dto) {
    if (dbus) {
        qDebug() << "send UuidWhitelistCmd" << "\n";
        dbus->publish("UuidWhitelistCmd", dto.to_ipc());
    }
}

void IPCEvent::sendAutoMode(ModeAutoDto dto) {
    if (dbus) {
        qDebug() << "send ModeAuto" << "\n";
        dbus->publish("UiAutoModeCmd", dto.to_ipc());
    }
}

void IPCEvent::sendThresholdConfig(ThresholdCmdDto dto) {
    if (dbus) {
        qDebug() << "send ThresholdConfigCmd" << "\n";
        dbus->publish("UiThresholdCmd", dto.to_ipc());
    }
}

void IPCEvent::reqCreateGroup(GroupGetAddrDto dto) {
    if (dbus) {
        qDebug() << "send CreateGroupCmd" << "\n";
        dbus->publish("CreateGroupCmd", dto.to_ipc());
    }
}

void IPCEvent::reqUpdateGroupConfig(AutomationRuleDto dto) {
    if (dbus) {
        qDebug() << "send UpdateGroupCmd" << "\n";
        dbus->publish("UpdateGroupCmd", dto.to_ipc());
    }
}

void IPCEvent::reqDeleteGroupDb(int groupId) {
    if (dbus) {
        QJsonObject obj; 
        obj["groupId"] = groupId;
        IpcMessage msg; 
        msg.payload =  QJsonDocument(obj).toJson(QJsonDocument::Compact);
        qDebug() << "send DeleteGroupCmd" << "\n";
        dbus->publish("DeleteGroupCmd", msg);
    }
}

void IPCEvent::reqSyncAllGroups() {
    if (dbus) {
        IpcMessage msg; 
        msg.payload = "{}";
        qDebug() << "send SyncAllGroupsCmd" << "\n";
        dbus->publish("SyncAllGroupsCmd", msg);
    }
}

void IPCEvent::reqSyncAllNodes()
{
    if (dbus) {
        IpcMessage msg; 
        msg.payload = "{}";
        qDebug() << "send reqSyncAllNodes" << "\n";
        dbus->publish("SyncAllNodesCmd", msg);
    }
}
