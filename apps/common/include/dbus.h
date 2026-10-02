#ifndef DBUS_H
#define DBUS_H

#include "ipc.h" 
#include "dbus_thread.h"
#include <gio/gio.h>
#include <string>
#include <map>
#include <thread>
#include <vector>
#include <semaphore>
#include <chrono>
#include "cjson_use.h"
#include <fstream>


/**
 * @brief configuration struct for dbus
 * @serviceName: name of service subscribe to dbus
 * @objectPath: path of dbus subscribeb with dbus
 * @interfaceName: include method and signal that another process can call as a proxy object
 */
struct DBusConfig {
    std::string serviceName;
    std::string objectPath;
    std::string interfaceName;
};


/**
 * @brief class get dbus configuration from file
 * 
 */
class ConfigManager {
public:
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    bool loadConfig(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        JsonUse::CJsonGuard root(content);
        if (!root.ptr) {
            return false;
        }

        std::vector<std::string> modules = {"Mesh", "Db", "Mqtt", "UI"};

        for (const auto& mod : modules) {
            cJSON* modObj = JsonUse::get_object(root.ptr, mod.c_str());
            if (modObj) {
                DBusConfig cfg;
                cfg.serviceName   = JsonUse::get_str(modObj, "serviceName");
                cfg.objectPath    = JsonUse::get_str(modObj, "objectPath");
                cfg.interfaceName = JsonUse::get_str(modObj, "interfaceName");
                configs[mod] = cfg;
            }
        }
        return true;
    }

    DBusConfig getConfig(const std::string& moduleName) {
        if (configs.find(moduleName) != configs.end()) {
            return configs[moduleName];
        }
        return {};
    }

private:
    std::map<std::string, DBusConfig> configs;
    ConfigManager() {}
};

/**
 * @brief define task is method or signal in dbus
 * 
 */
enum class TaskType {
    METHOD_CALL,
    SIGNAL_RECEIVED
};



/**
 * @brief dbus task will be created when a method called or receiving a signal
 * @type: METHOD_CALL or SIGNAL_RECEIVED
 * @name: name of method or signal that subcribed to dbus
 * @payload: payload of signal or method (get from dbus packet)
 * @invocation: context of the task processing (contains information about the caller and is used to return the result)
 */

struct IpcTask {
    TaskType type;
    std::string name;
    IpcMessage payload; 
    GDBusMethodInvocation* invocation = nullptr;
};


/**
 * @brief Gdbus implementation of IIpc interface
 * manage dbus connection, method exposure, signal subscriptions
 * and process callbacks asynchronously using a dedicated worker task thread
 * 
 */
class DBusGDBus : public IIpc {
public:
    DBusGDBus(const DBusConfig& config, const std::string& introspection_xml_path);
    ~DBusGDBus() override;

    bool init() override;
    void deinit() override;

    bool publish(const std::string& topic, const IpcMessage& msg) override;
    void subscribe(const IpcEndpoint& endpoint, EventCallback callback) override;

    IpcMessage call(const IpcEndpoint& endpoint, const IpcMessage& request) override;
    void expose(const std::string& endpoint, RpcCallback callback) override;

    const DBusConfig* getDbusConfig() const { 
        return &dbus_config; 
    }

    GDBusNodeInfo* getGDBusNodeInfo() const { 
        return introspection_data; 
    }

    std::vector<guint>& getObject_registration_ids() { 
        return object_registration_ids; 
    }

    GMainLoop* getGMainLoop() { 
        return loop; 
    }

    std::map<std::string, RpcCallback>& getMethodHandler() { 
        return method_handlers; 
    }
    std::map<std::string, EventCallback>& getSignalHandler() { 
        return signal_handlers; 
    }

private:
    DBusConfig dbus_config;
    std::string introspection_xml_path;
    GDBusNodeInfo* introspection_data = nullptr;
    GDBusConnection* connection = nullptr;
    GMainLoop* loop = nullptr;
    GMainContext* context = nullptr;
    guint owner_id = 0;

    std::vector<guint> object_registration_ids;
    std::vector<guint> signal_subscription_ids;

    std::map<std::string, RpcCallback> method_handlers;
    std::map<std::string, EventCallback> signal_handlers;

    ThreadSafeQueue<IpcTask> task_queue;
    std::thread message_processing;
    std::thread dbus_init_thread;
    std::binary_semaphore ready{0};

    IpcMessage read_payload(GVariant* var);
    GVariant* write_payload(const IpcMessage& message);

    void glib_thread();
    void worker_thread_loop();
    bool waitReady(int timeout_ms);

    
    static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data);
    static void on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data);
    static void on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data);
    static void on_method_call_cb(GDBusConnection* conn, const gchar* sender, const gchar* path, const gchar* iface, const gchar* method_name, GVariant* parameters, GDBusMethodInvocation* invocation, gpointer user_data);
    static GVariant* on_get_property_cb(GDBusConnection* conn, const gchar* sender, const gchar* path, const gchar* iface, const gchar* property_name, GError** error, gpointer user_data);
    static gboolean on_set_property_cb(GDBusConnection* conn, const gchar* sender, const gchar* path, const gchar* iface, const gchar* property_name, GVariant* value, GError** error, gpointer user_data);
    static void on_signal_cb(GDBusConnection* conn, const gchar* sender, const gchar* path, const gchar* iface, const gchar* signal_name, GVariant* parameters, gpointer user_data);
};

#endif