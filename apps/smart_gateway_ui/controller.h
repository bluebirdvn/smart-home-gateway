#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QVariantList>
#include "ipc_dto.h"
#include "devicemodel.h"
#include "log_model.h"



class IPCEvent;

/**
 * @brief this class is bridge between qml and QT core (backend)
 * exposed to qml as a object for qml calling
 * receiving events from ipcevent and pushing them in to device model 
 * turn UI actions (Q_INVOKABLE) into signal to wake up ipcevent to send it out
 */
class Controller : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isGatewayReady READ isGatewayReady NOTIFY gatewayStatusChanged)
    Q_PROPERTY(bool isServerConnected READ isServerConnected NOTIFY serverStatusChanged)
    Q_PROPERTY(QString meshState READ meshStatus NOTIFY meshStatusChanged)
    Q_PROPERTY(QVariantList automationGroups READ getAutomationGroups NOTIFY groupsChanged)

public:
    explicit Controller(IPCEvent* ipcEvent, DeviceModel* model, LogModel* logModel, QObject *parent = nullptr);

    bool isGatewayReady() const { 
        return gatewayReady; 
    }
    bool isServerConnected() const { 
        return serverConnected; 
    }
    QString meshStatus() const { 
        return meshState; 
    }

    /**
     * @brief all this siganl below was called from qml to send it out to target applicaiton to process cmd
     * 
     */
    /**
     * @brief require backend to resend all data of local database (use at startup and after editting a)
     * 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void requestInitialData(); 
    
    /**
     * @brief send signal to ipcevent that provision a device 
     * 
     * @param uuid of device need to provision
     * @param bearer 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void provisionDevice(const QString &uuid, int32_t bearer);   

    /**
     * @brief Set the Actuator Manual object (switch between auto and manual)
     * 
     * @param nodeId 
     * @param deviceType 
     * @param state 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void setActuatorManual(const QString &nodeId, int deviceType, bool state);

    /**
     * @brief delete a node from network
     * 
     * @param nodeId 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void removeNode(const QString &nodeId);             

    /**
     * @brief Create a New Group object
     * 
     * @param groupName 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void createNewGroup(const QString &groupName);

    /**
     * @brief delete a group of node but not delete node
     * 
     * @param groupId 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void deleteGroup(int groupId);

    /**
     * @brief add a device (sensor, actuator) into a mesh group
     * 
     * @param groupId 
     * @param unicast 
     * @param isSensor 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void addDeviceToGroup(int groupId, int unicast, bool isSensor);

    /**
     * @brief unsub, unpub a device from group
     * 
     * @param groupId 
     * @param unicast 
     * @param isSensor 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void removeDeviceFromGroup(int groupId, int unicast, bool isSensor);

    /**
     * @brief Set the Group Mode object (auto or manual)
     * 
     * @param groupId 
     * @param isAuto 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void setGroupMode(int groupId, bool isAuto);

    /**
     * @brief save name, thesholds, sensor type, actuator type, members of group and send it out to backend
     * 
     * @param groupId 
     * @param name 
     * @param tOn 
     * @param tOff 
     * @param autoMode 
     * @param sensorType 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void applyMeshPubSubConfig(int groupId, const QString &name, float tOn, float tOff, bool autoMode, int sensorType);

    /**
     * @brief Set the Ac Manual object
     * 
     * @param nodeId 
     * @param power 
     * @param mode 
     * @param fan 
     * @param temp 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void setAcManual(const QString &nodeId, bool power, int mode, int fan, int temp);

    /**
     * @brief Set the Light Manual object
     * 
     * @param nodeId 
     * @param on 
     * @param brightness 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void setLightManual(const QString &nodeId, bool on, int brightness);

    /**
     * @brief Get the Automation Groups object: convert to a qvariantlist for qml
     * 
     * @return QVariantList 
     */
     
    QVariantList getAutomationGroups() const;

signals:
/**
 * @brief notify for qml that a property changed
 * 
 */
    void gatewayStatusChanged();
    void serverStatusChanged();
    void meshStatusChanged();
    void groupsChanged();

/**
 * @brief request forwards to IPCEvent
 * 
 * @param dto 
 */
    void reqSendActuator(ActuatorCmdDto dto);
    void reqDeleteNode(DeleteNodeDto dto);
    void reqWhitelistUuid(UuidWhitelistDto dto);
    void reqSendActautorMode(ModeAutoDto dto);
    void reqThresholdConfig(ThresholdCmdDto dto);

    void sigCreateGroup(GroupGetAddrDto dto);
    void sigUpdateGroupConfig(AutomationRuleDto dto);
    void sigDeleteGroup(int groupId);
    void sigRequestSyncGroups(); 
    void sigRequestSyncNode();

private slots:
/**
 * @brief data comming back to sync from backend
 * 
 * @param dto 
 */
    void onNodesSynced(const NodeInfoDto &dto);
    void onSensorsSynced(const SensorDto &dto);
    void onActuatorsSynced(const ActuatorStatusDto &dto);
    void onGroupsSynced(const GroupSyncListDto &dto);

private:
    DeviceModel* device_models;
    LogModel* log_model;
    
    QMap<int, AutomationRuleDto> groups_config; 

    bool gatewayReady = false;
    bool serverConnected = false;
    QString meshState = "Unknown";
};

#endif
