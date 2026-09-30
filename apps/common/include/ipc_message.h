#ifndef _IPC_MESSAGE_H
#define _IPC_MESSAGE_H

#include <string>
#include <functional>

/**
 * @brief payload of ipc message
 * 
 */
struct IpcMessage {
    std::string payload; 
};

/**
 * @brief definition for callback function for remote produce call/method call
 * @param request: param of function call
 * @return: IpcMessage
 */
using RpcCallback = std::function<IpcMessage(const IpcMessage& request)>;

/**
 * @brief definition for event callback (signal callback)
 * @param: event: struct ipc message
 * @return void
 */
using EventCallback = std::function<void(const IpcMessage& event)>;


#endif