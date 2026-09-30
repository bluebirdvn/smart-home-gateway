#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QQuickWindow>
#include "config_manager.h"
#include "dbus.h"
#include "ipcevent.h"
#include "controller.h"
#include "devicemodel.h"
#include "log_model.h"

int main(int argc, char *argv[])
{

    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
    QApplication app(argc, argv);



    QString path = qEnvironmentVariable("GATEWAY_CONFIG_PATH");

    qRegisterMetaType<SensorDto>("SensorDto");
    qRegisterMetaType<UnprovAdvDto>("UnprovAdvDto");
    qRegisterMetaType<HeartbeatDto>("HeartbeatDto");
    qRegisterMetaType<ActuatorStatusDto>("ActuatorStatusDto");
    qRegisterMetaType<NodeInfoDto>("NodeInfoDto");
    qRegisterMetaType<ActuatorCmdDto>("ActuatorCmdDto");
    qRegisterMetaType<GroupOpDto>("GroupOpDto");
    qRegisterMetaType<DeleteNodeDto>("DeleteNodeDto");
    qRegisterMetaType<UuidWhitelistDto>("UuidWhitelistDto");
    qRegisterMetaType<ThresholdCmdDto>("ThresholdCmdDto");
    qRegisterMetaType<AutomationRuleDto>("AutomationRuleDto");
    
    if (!ConfigManager::getInstance().loadConfig((!path.isNull()) ? path : "/etc/gateway/config.json")) {
        qWarning() << "load config.json failed";
        ConfigManager::getInstance().loadConfig(QCoreApplication::applicationDirPath() + "/config.json");
    }

    ConfigManager& config = ConfigManager::getInstance();
    DBusConfig dbusCfg = config.getConfig("UI");
    QDbusImpl* ipc = new QDbusImpl(dbusCfg);
    QThread* ipcThread = new QThread();
    ipc->moveToThread(ipcThread);

    IPCEvent* ipcEvent = new IPCEvent(ipc); 
    ipcEvent->moveToThread(ipcThread);


    QObject::connect(ipcThread, &QThread::started, ipc, [ipc, ipcEvent](){
        if (ipc->init()) {
            ipcEvent->ipc_init();

        } else {
            qDebug() << "error init dbus";
        }
    });
    ipcThread->start();

    DeviceModel* model = new DeviceModel();
    LogModel* logModel = new LogModel();
    Controller controller(ipcEvent, model, logModel);


    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("MyDevice", model);
    engine.rootContext()->setContextProperty("GatewayController", &controller);
    engine.rootContext()->setContextProperty("LogModel", logModel);


    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &app, [&controller](QObject *obj, const QUrl &objUrl) {
            if (!obj) {
                QCoreApplication::exit(-1);
            } else {
                controller.requestInitialData();
            }
        },
        Qt::QueuedConnection);

    engine.loadFromModule("smart_gateway_ui", "Main");

    int ret = app.exec();
    QMetaObject::invokeMethod(ipc, &QDbusImpl::deinit, Qt::BlockingQueuedConnection);
    ipcThread->quit();
    ipcThread->wait();
    delete ipcThread;
    delete ipc;
    delete ipcEvent;
    delete model;
    delete logModel;

    return ret;
}
