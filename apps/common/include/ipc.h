#ifndef IPC_H
#define IPC_H

#include <string>
#include <functional>
#include <vector>
#include "ipc_message.h"

enum class DataStatus {
    OK = 0,
    ERROR,
    NOT_READY
};

struct IpcEndpoint {
    std::string moduleName; 
    std::string interface;  
    std::string method;     
};


class IIpc {
public:
    virtual ~IIpc() = default;

    virtual bool init() = 0;
    virtual void deinit() = 0;

    virtual bool publish(const std::string& topic, const IpcMessage& msg) = 0;
    
    virtual void subscribe(const IpcEndpoint& endpoint, EventCallback callback) = 0;
    
    virtual IpcMessage call(const IpcEndpoint& endpoint, const IpcMessage& request) = 0;
    
    virtual void expose(const std::string& endpoint, RpcCallback callback) = 0;
};

#endif 