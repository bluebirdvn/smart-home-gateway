#ifndef IPCEVENT_H
#define IPCEVENT_H

#include <QObject>
#include <QString>
#include "ipc_dto.h"
#include "ipc.h"

/**
 * @brief translate between ipc layer and qt signal/slot
 * 
 */
class IPCEvent : public QObject
{
    Q_OBJECT
public:
    explicit IPCEvent(IIpc* ipc, QObject *parent = nullptr);

    /**
     * @brief subscribe to all events, call after ipc layer initialization
     * 
     */
    void ipc_init();

signals:

/**
 * @brief events received from backend
 * 
 * @param dto 
 */
    void unprovDeviceDiscovery(UnprovAdvDto dto);
    void heartbeatReceive(HeartbeatDto dto);
    void gatewayStatusReceive(ModuleStatusDto dto);
    void serverStatusReceive(ModuleStatusDto dto);
    void meshStatusReceive(ModuleStatusDto dto);
    
    void groupsSyncReceive(GroupSyncListDto dto);
    void nodeSyncReceive(NodeInfoDto dto);
    void sensorSyncReceive(SensorDto dto);
    void actuatorSyncReceive(ActuatorStatusDto dto);

public slots:

/**
 * @brief command sendto backend
 * 
 * @param dto 
 */
    void sendActuatorCmd(ActuatorCmdDto dto);
    void deleteNode(DeleteNodeDto dto);
    void sendUuidWhitelist(UuidWhitelistDto dto);
    void sendAutoMode(ModeAutoDto dto);
    void sendThresholdConfig(ThresholdCmdDto dto);
    
    void reqCreateGroup(GroupGetAddrDto dto);
    void reqUpdateGroupConfig(AutomationRuleDto dto);
    void reqDeleteGroupDb(int groupId);
    
    void reqSyncAllNodes();
    void reqSyncAllGroups(); 

private:
    IIpc* dbus;
};

#endif
