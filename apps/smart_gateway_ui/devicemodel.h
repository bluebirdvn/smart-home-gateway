#ifndef DEVICEMODEL_H
#define DEVICEMODEL_H

#include <QString>
#include <QObject>
#include <QAbstractListModel>
#include <QList>
#include <QMap>
#include <QVariantList>
#include <cstdint>
#include "ipc_dto.h"
#include "mesh_vendor_id.h"

/**
 * @brief lastest reading reported by a sensor node
 * 
 */
struct sensorData
{
    float humidity = 0.0f;
    float temperature = 0.0f;
    float lux = 0.0f;
    float soil_moisture = 0.0f;
    float motion = 0.0f;
    int battery = 0;
};

/**
 * @brief lastest reading reported by a actuator node
 * 
 */
struct actuatorData
{
    float currentSetpoint = 0.0f;
    float controlSetpoint = 0.0f;
    int setState = 0;
    int actuatorType = 0;
};

/**
 * @brief data of either actuator or sensor node
 * The active memeber is decided by kind
 */
union deviceData
{
    struct sensorData sensor;
    struct actuatorData actuator;
    deviceData() : sensor{} {}
};


/**
 * @brief a device that has been seen advertising but not provisioned yet
 * 
 */
struct unProvisionDevice
{
    QString uuid;
    int32_t rssi = 0;
    int32_t bearer = 0;
    int32_t oob_info = 0;
    int status = 0;
};


/**
 * @brief details data of a node provisioned
 * 
 */
struct deviceInfo
{
    int      id = 0;
    uint16_t unicast = 0; 
    uint16_t addr = 0;
    QString  uuid;
    QString  name;
    NodeKind kind = NodeKind::Unknown;   
    int      status = 0;
    deviceData data;
    quint64  lastSeen = 0;
    uint16_t model_id = 0;
    uint16_t company_id = 0xFFFF;
    int      features = 0;
    bool    pending = false;
    quint64 pendingSince = 0;
};


/**
 * @brief List model exposed to qml
 * 
 */
class DeviceModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int totalDevices READ totalDevices NOTIFY deviceCountChanged)
    Q_PROPERTY(int activeDevices READ activeDevices NOTIFY deviceCountChanged)
    Q_PROPERTY(QVariantList unprovList READ unprovList NOTIFY unprovListChanged)
    Q_PROPERTY(float avgTemp READ avgTemp NOTIFY averagesChanged)
    Q_PROPERTY(float avgHumi READ avgHumi NOTIFY averagesChanged)
    Q_PROPERTY(float avgLux READ avgLux NOTIFY averagesChanged)
    Q_PROPERTY(float avgSoil READ avgSoil NOTIFY averagesChanged)

public:

    /**
     * @brief roles available to qml delegates
     * 
     */
    enum DeviceRoles {
        AddrRole = Qt::UserRole + 1,
        AddrNumRole,
        NameRole, 
        NodeTypeRole, 
        StatusRole,
        HumidityRole, 
        TemperatureRole, 
        SoilMoistureRole,
        LuxRole, 
        MotionRole,
        CurrentSetpointRole, 
        ActuatorStateRole, 
        BatteryRole, 
        UuidRole,
        ModelIdRole, 
        CompanyIdRole, 
        FeaturesRole,
        IsAcRole,
        IsLightRole,
        PendingRole,
    };

    /**
     * @brief Construct a new Device Model object
     * 
     * @param parent 
     */
    explicit DeviceModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    QString nameByAddr(int addr) const;


    /**
     * @brief get average temperature of mesh
     * 
     * @return float 
     */
    float avgTemp() const { 
        return average_temp; 
    }

    /**
     * @brief get average humidity of mesh
     * 
     * @return float 
     */
    float avgHumi() const { 
        return average_humi; 
    }

    float avgLux() const { 
        return average_lux; 
    }

    
    int totalDevices() const { 
        return devices.count(); 
    }
    float avgSoil() const { 
        return average_soil; 
    }
    

    int activeDevices() const;

    uint16_t getUnicastByAddr(const uint16_t &addr) const;
    uint16_t getNodeAddr(const uint16_t &addr) const;
    uint16_t getNodeModelId(const uint16_t &addr) const;
    uint16_t getNodeCompanyId(const uint16_t &addr) const;
    QString  getNodeUuid(const uint16_t &addr) const;
    int32_t  getDeviceType(const uint16_t &addr) const;

    void setPending(uint16_t addr, bool pending);


    Q_INVOKABLE bool isPending(int addr) const;

    /**
     * @brief UPDATE data comming, called by controller
     * 
     */
    /**
     * @brief store a new sensor reading and mark the node online
     * 
     * @param addr 
     * @param temp 
     * @param humi 
     * @param soil 
     * @param lux 
     * @param motion 
     * @param battery 
     * @param lastSeen 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void updateSensorData(const uint16_t &addr, float temp, float humi, float soil, float lux, float motion, int battery, quint64 lastSeen);

    /**
     * @brief add a newly provisioned node, or refresh it if it existed
     * 
     * @param addr 
     * @param name 
     * @param uuid 
     * @param unicast 
     * @param element_addr 
     * @param modelId 
     * @param companyId 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void onProvisionSuccess(const uint16_t &addr, const QString& name, const QString &uuid, uint16_t unicast, uint16_t modelId, uint16_t companyId);

    /**
     * @brief stored lastest actuator data and mark it online
     * 
     * @param addr 
     * @param actuatorId 
     * @param setpoint 
     * @param status 
     * @param lastSeen 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void updateActuatorStatus(const uint16_t &addr, int actuatorId, float setpoint, int status, quint64 lastSeen);

    /**
     * @brief mark node online
     * 
     * @param addr 
     * @param features 
     * @param lastSeen 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void markOnline(const uint16_t &addr, int features, quint64 lastSeen);

    /**
     * @brief mark node offline
     * 
     * @param addr 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void markOffline(const uint16_t &addr);

    /**
     * @brief delete device from list
     * 
     * @param addr 
     * @return Q_INVOKABLE 
     */
    Q_INVOKABLE void deleteDeviceByUnicast(const uint16_t &addr);

    /**
     * @brief clear all node and reset average values
     * 
     */
    void clearAllDevices();

    /**
     * @brief add a unprovisioned device 
     * 
     * @param device 
     */
    void addUnprovDev(const unProvisionDevice& device);

    /**
     * @brief delete a unprovisioned device from list when it was provisioned
     * 
     * @param uuid 
     */
    void deleteUnprovDevice(const QString &uuid);
    QVariantList unprovList() const;

    const QList<deviceInfo>& getDevices() const { 
        return devices; 
    }

signals:
    void deviceCountChanged();
    void unprovListChanged();
    void averagesChanged();
    void pendingChanged(int addr, bool pending);
    void pendingTimedOut(int addr);

private:
    QList<deviceInfo> devices;
    QList<unProvisionDevice> unprovDevs;
    float average_soil = 0.0f;
    float average_temp = 0.0f;
    float average_humi = 0.0f;
    float average_lux = 0.0f;
    void calculateAverages();
};

#endif
