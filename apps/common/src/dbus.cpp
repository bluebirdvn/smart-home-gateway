#include "dbus.h"
#include <iostream>

DBusGDBus::DBusGDBus(const DBusConfig& config, const std::string& xml_path) 
    : dbus_config(config), introspection_xml_path(xml_path) {
}

DBusGDBus::~DBusGDBus() {
    deinit();
}

IpcMessage DBusGDBus::read_payload(GVariant* var) {
    IpcMessage msg;
    msg.payload = "{}"; 
    if (var != nullptr) {
        GVariant* inner_variant = nullptr;
        g_variant_get(var, "(v)", &inner_variant);
        if (inner_variant != nullptr) {
            if (g_variant_is_of_type(inner_variant, G_VARIANT_TYPE_STRING)) {
                const gchar* str = g_variant_get_string(inner_variant, nullptr);
                if (str) {
                    msg.payload = str; 
                }
            }
            g_variant_unref(inner_variant);
        }
    }
    return msg;
}

GVariant* DBusGDBus::write_payload(const IpcMessage& message) {
    const char* str_payload = message.payload.empty() ? "{}" : message.payload.c_str();
    return g_variant_new("(v)", g_variant_new_string(str_payload));
}

bool DBusGDBus::publish(const std::string& topic, const IpcMessage& msg) {
    if (!connection) {
        return false;
    }
    GError* error = nullptr;
    GVariant* param = write_payload(msg); 
    
    gboolean success = g_dbus_connection_emit_signal(connection, NULL, dbus_config.objectPath.c_str(), dbus_config.interfaceName.c_str(), topic.c_str(), param, &error);
        
    if (!success) {
        g_printerr("Emit Signal Error: %s\n", error->message);
        g_error_free(error);
        return false;
    }
    return true;
}

void DBusGDBus::subscribe(const IpcEndpoint& endpoint, EventCallback callback) {
    if (!connection) {
        return;
    }
    std::string topic = endpoint.method;
    DBusConfig dbus_config = ConfigManager::getInstance().getConfig(endpoint.moduleName);

    std::string finalIface = endpoint.interface.empty() ? dbus_config.interfaceName : endpoint.interface;
    signal_handlers[topic] = std::move(callback);

    struct Context { 
        DBusGDBus* self; 
        std::string serviceName;
        std::string objectPath;
        std::string interfaceName;
        std::string topic; 
    };
    
    auto* a = new Context{ this, dbus_config.serviceName, dbus_config.objectPath, finalIface, topic};

    g_main_context_invoke_full(context, G_PRIORITY_DEFAULT, [](gpointer data) -> gboolean {
        auto* a = static_cast<Context*>(data);
        guint id = g_dbus_connection_signal_subscribe(a->self->connection, a->serviceName.empty() ? nullptr : a->serviceName.c_str(), a->interfaceName.empty() ? nullptr : a->interfaceName.c_str(), a->topic.c_str(), a->objectPath.empty() ? nullptr : a->objectPath.c_str(), nullptr, G_DBUS_SIGNAL_FLAGS_NONE, on_signal_cb, a->self, nullptr);
        a->self->signal_subscription_ids.push_back(id);
        delete a; 
        return G_SOURCE_REMOVE;
    }, a, nullptr);
    std::cout << "register event" << std::endl;

}

IpcMessage DBusGDBus::call(const IpcEndpoint& endpoint, const IpcMessage& request) {
    IpcMessage response;
    if (!connection) {
        return response;
    }
    
    DBusConfig targetConfig = ConfigManager::getInstance().getConfig(endpoint.moduleName);
    
    if (targetConfig.serviceName.empty()) {
        g_printerr("[DBus] Call Error: Target module '%s' not found in config.json\n", endpoint.moduleName.c_str());
        return response;
    }

    std::string finalIface = endpoint.interface.empty() ? targetConfig.interfaceName : endpoint.interface;

    GError* error = nullptr;
    GVariant* param = write_payload(request); 
    
    GVariant* result = g_dbus_connection_call_sync(connection, targetConfig.serviceName.c_str(), targetConfig.objectPath.c_str(), finalIface.c_str(), endpoint.method.c_str(), param, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);  
    
    if (error != nullptr) {
        g_printerr("[DBus] Call error on %s:%s - %s\n", endpoint.moduleName.c_str(), endpoint.method.c_str(), error->message);
        g_error_free(error);
        return response;
    }
    
    if (result != nullptr) {
        GVariantIter iter;
        GVariant* child = nullptr;
        g_variant_iter_init(&iter, result);
        if ((child = g_variant_iter_next_value(&iter)) != nullptr) {
            response = read_payload(child);
            g_variant_unref(child);
        }
        g_variant_unref(result);
    }
    return response;
}

void DBusGDBus::expose(const std::string& endpoint, RpcCallback callback) {
    method_handlers[endpoint] = std::move(callback);
}

void DBusGDBus::worker_thread_loop() {
    IpcTask task;
    while (task_queue.pop(task)) {
        if (task.type == TaskType::METHOD_CALL) {
            auto it = method_handlers.find(task.name);
            if (it != method_handlers.end()) {
                IpcMessage response = it->second(task.payload); 
                GVariant* reply_var = write_payload(response);
                g_dbus_method_invocation_return_value(task.invocation, reply_var);
            } else {
                g_dbus_method_invocation_return_error(task.invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "Method not implemented");
            }
        } else if (task.type == TaskType::SIGNAL_RECEIVED) {
            auto it = signal_handlers.find(task.name);
            if (it != signal_handlers.end()) {
                it->second(task.payload); 
            }
        }
    }
}

void DBusGDBus::on_method_call_cb(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar* method_name, GVariant* parameters, GDBusMethodInvocation* invocation, gpointer user_data) {
    auto* self = static_cast<DBusGDBus*>(user_data);
    std::string method_str(method_name);
    
    if (self->getMethodHandler().find(method_str) != self->getMethodHandler().end()) {
        IpcTask task;
        task.type = TaskType::METHOD_CALL;
        task.name = method_str;
        task.payload = self->read_payload(parameters); 
        task.invocation = invocation; 
        self->task_queue.push(task); 
    } else {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "Method '%s' not implemented", method_name);
    }
}

void DBusGDBus::on_signal_cb(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar* signal_name, GVariant* parameters, gpointer user_data) {
    auto* self = static_cast<DBusGDBus*>(user_data);
    std::string sig_str(signal_name);
    
    if (self->getSignalHandler().find(sig_str) != self->getSignalHandler().end()) {
        IpcTask task;
        task.type = TaskType::SIGNAL_RECEIVED;
        task.name = sig_str;
        task.payload = self->read_payload(parameters);
        self->task_queue.push(task); 
    }
}

bool DBusGDBus::init() {
    std::ifstream xml_file(introspection_xml_path);
    if (!xml_file.is_open()) {
        g_printerr("Failed to open XML file: %s\n", introspection_xml_path.c_str());
        return false;
    }
    std::string xml_content((std::istreambuf_iterator<char>(xml_file)), std::istreambuf_iterator<char>());

    GError* xml_err = nullptr;
    introspection_data = g_dbus_node_info_new_for_xml(xml_content.c_str(), &xml_err);
    if (introspection_data == nullptr) {
        g_printerr("XML Error: %s\n", xml_err->message);
        g_error_free(xml_err);
        return false;
    }

    message_processing = std::thread(&DBusGDBus::worker_thread_loop, this);
    dbus_init_thread = std::thread(&DBusGDBus::glib_thread, this);

    if (!waitReady(2000)) { 
        g_printerr("Failed to initialize DBus connection within timeout\n");
        return false;
    }
    return true;
}

void DBusGDBus::deinit() {
    task_queue.stop();
    if (message_processing.joinable()) {
        message_processing.join();
    }

    if (connection != nullptr) {
        for (guint id : object_registration_ids) {
            g_dbus_connection_unregister_object(connection, id);
        }
    }
    object_registration_ids.clear();
    
    for (guint id : signal_subscription_ids) {
        g_dbus_connection_signal_unsubscribe(connection, id);
    }
    signal_subscription_ids.clear();

    if (loop != nullptr && g_main_loop_is_running(loop)) { 
        g_main_loop_quit(loop);
    }
    
    if (dbus_init_thread.joinable()) {
        dbus_init_thread.join();
    }
    connection = nullptr;
}

void DBusGDBus::glib_thread() {
    context = g_main_context_new();
    g_main_context_push_thread_default(context);

    owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM, dbus_config.serviceName.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired, on_name_acquired, on_name_lost, this, NULL);
    loop = g_main_loop_new(context, FALSE);
    g_main_loop_run(loop);
    g_bus_unown_name(owner_id);

    if (introspection_data) {
        g_dbus_node_info_unref(introspection_data);
        introspection_data = nullptr;
    }
    g_main_loop_unref(loop);
    loop = nullptr;
    g_main_context_pop_thread_default(context);
    g_main_context_unref(context);
}

bool DBusGDBus::waitReady(int timeout_ms) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    if (ready.try_acquire_until(deadline)) {
        ready.release();
        return true;
    }
    g_printerr("waitReady timeout (%d ms)\n", timeout_ms);
    return false;
}

void DBusGDBus::on_bus_acquired(GDBusConnection *connection, const gchar *, gpointer user_data) {
    auto* self = static_cast<DBusGDBus*>(user_data);
    self->connection = connection; 

    const std::string& objPath = self->getDbusConfig()->objectPath;
    if (objPath.empty()) {
        g_printerr("objectPath is empty — cannot register object!\n");
        self->ready.release(); 
        return;
    }
    static const GDBusInterfaceVTable interface_vtable = [](){
        GDBusInterfaceVTable vtable = {};
        vtable.method_call = DBusGDBus::on_method_call_cb;   
        vtable.get_property = DBusGDBus::on_get_property_cb;  
        vtable.set_property = DBusGDBus::on_set_property_cb;
        return vtable;
    }();

    GError* error = nullptr;
    guint registration_id = g_dbus_connection_register_object(connection, objPath.c_str(), self->getGDBusNodeInfo()->interfaces[0], &interface_vtable, self, NULL, &error);
        
    if (registration_id == 0) {
        g_printerr("Failed to register object: %s\n", error->message);
        g_error_free(error);
    } else {
        self->getObject_registration_ids().push_back(registration_id);
    }

    self->ready.release();
}

void DBusGDBus::on_name_acquired(GDBusConnection *, const gchar *, gpointer) {
    std::cout << "Successfully got name for DBus" << std::endl;
}

void DBusGDBus::on_name_lost(GDBusConnection *, const gchar *name, gpointer user_data) {
    g_printerr("Lost D-Bus name: %s\n", name);
    auto* self = static_cast<DBusGDBus*>(user_data);
    if (self->getGMainLoop() != nullptr && g_main_loop_is_running(self->getGMainLoop())) {
        g_main_loop_quit(self->getGMainLoop());
    }
}

GVariant* DBusGDBus::on_get_property_cb(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar* property_name, GError** error, gpointer) {
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Property '%s' not supported", property_name);
    return NULL;
}

gboolean DBusGDBus::on_set_property_cb(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar* property_name, GVariant*, GError** error, gpointer) {
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY, "Cannot set property '%s'", property_name);
    return FALSE;
}