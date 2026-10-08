# Smart Home IoT Mesh Ecosystem

An embedded Linux (Yocto) gateway that bridges a **BLE Mesh** sensor/actuator network to a local **Qt/QML touch UI** and to **HiveMQ Cloud** over MQTT.
 
This document has two goals:

1. Explain **what the project is and how to build/run it**.
2. Teach the **background knowledge** you need to understand, debug and extend it (BLE Mesh, ESP-IDF, UART framing, D-Bus, SQLite, MQTT, Qt, systemd, Yocto, device tree).


## Table of Contents

1. [Project Overview](#1-project-overview)
2. [System Architecture](#2-system-architecture)
3. [Repository Layout](#3-repository-layout)
4. [Background Knowledge](#4-background-knowledge)
   - [4.1 BLE Mesh](#41-ble-mesh)
   - [4.2 ESP-IDF and ESP BLE Mesh](#42-esp-idf-and-esp-ble-mesh)
   - [4.3 UART framing, CRC16 and ARQ](#43-uart-framing-crc16-and-arq)
   - [4.4 D-Bus](#44-d-bus)
   - [4.5 Event-driven design and the DB as orchestrator](#45-event-driven-design-and-the-db-as-orchestrator)
   - [4.6 SQLite](#46-sqlite)
   - [4.7 MQTT and HiveMQ Cloud](#47-mqtt-and-hivemq-cloud)
   - [4.8 Qt / QML / QtCharts](#48-qt--qml--qtcharts)
   - [4.9 systemd](#49-systemd)
   - [4.10 Yocto Project](#410-yocto-project)
   - [4.11 Device tree overlays](#411-device-tree-overlays)
   - [4.12 Wi-Fi and TLS on the target](#412-wi-fi-and-tls-on-the-target)
   - [4.13 Concurrency patterns used in the daemon](#413-concurrency-patterns-used-in-the-daemon)
5. [Module Reference](#5-module-reference)
6. [Message Flows](#6-message-flows)
7. [Configuration, Build, and Flash Guide](#7-configuration-build-and-flash-guide)
   - [7.1 Clone the Repository](#71-clone-the-repository)
   - [7.2 Configuration Steps](#72-configuration-steps)
   - [7.3 Build the Yocto Image](#73-build-the-yocto-image)
   - [7.4 Flash the SD Card](#74-flash-the-sd-card)
   - [7.5 Display & Touch Screen Setup (ILI9341)](#75-display--touch-screen-setup-ili9341)
   - [7.6 Optimizing UI Refresh Rate (SPI & Core Clock)](#76-optimizing-ui-refresh-rate-spi-and-core-clock)
   - [7.7 First Boot Checklist](#77-first-boot-checklist)
   - [7.8 Fast Application Update (For Developers)](#78-fast-application-update-for-developers)
8. [Firmware Guide (ESP32)](#8-firmware-guide-esp32)
9. [Further Reading](#9-further-reading)
10. [License](#10-license)

---
 
## 1. Project Overview

| Capability | How it is done |
|---|---|
| Collect sensor data | ESP32 Provisioner receives BLE Mesh sensor messages and forwards them over UART |
| Control actuators | UI or cloud command goes through the DB, daemon and UART to the mesh |
| Edge automation | Groups with thresholds and auto mode are pushed into the mesh nodes |
| Local HMI | Qt/QML app with live charts |
| Cloud | MQTT bridge to HiveMQ Cloud, with remote control |
| Persistence | SQLite (nodes, actuators, sensor history, groups) |
| Deployment | Yocto image for Raspberry Pi with systemd services |

## 2. System Architecture
 
![System Architecture](images/system_architecture.png)

### System Architecture Overview

The system is built upon a **Host-NCP (Network Co-Processor)** model, combining a multi-process Linux Host (Raspberry Pi Zero 2 W) with a Real-Time Edge Network (ESP32).

**1. Application Host Layer (Yocto Linux on Raspberry Pi)**
The software stack is decoupled into four independent microservices communicating exclusively via **System D-Bus (Pub/Sub)**. If one process (e.g., UI) crashes, the core daemon and database continue to operate autonomously.
*   **`local-database`**: The SQLite-based orchestrator and **Single Source of Truth**. It persists all local and remote configurations before re-broadcasting instructions.
*   **`gateway-daemon`**: The C++ core application. It translates D-Bus DTOs into a custom binary protocol (Opcode + CRC16 + ARQ) and manages a reliable UART link to the physical layer.
*   **`smart_gateway_ui`**: The Qt/QML frontend rendering the HMI on an ILI9341 SPI display, operating strictly on an event-driven basis (`*SyncEvent`).
*   **`mqtt-connect`**: A TLS-secured bridge to HiveMQ Cloud. Using a "Virtual UI" pipeline, it translates downstream cloud commands into internal D-Bus signals, achieving 100% logic reuse.

**2. Edge Network Layer (ESP-IDF on ESP32 Series)**
Real-time radio operations are offloaded to dedicated microcontrollers to prevent Linux scheduling latencies.
*   **ESP32 Provisioner (NCP)**: Acts as the physical bridge. It processes UART frames from the gateway daemon, executes BLE Mesh provisioning, and routes Generic/Vendor Model messages into the wireless network.
*   **BLE Mesh Nodes (ESP32-H2/C6)**: The end devices (sensors, relays, AC controllers). They support **Edge Automation**: sensors publish telemetry directly to group addresses, allowing actuators to evaluate thresholds and trigger actions locally, even if the main gateway goes offline.


### Design rules
 
1. **No direct calls between processes.** Everything is a D-Bus signal (publish/subscribe).
2. **The DB is the single source of truth and the orchestrator.** Control and configuration commands go to the DB first. It saves them, then re-publishes (rebroadcasts) them to the daemon.
3. **`mqtt-connect` is a Virtual UI.** A cloud command is translated into the same D-Bus signal the UI would emit, so the whole downstream logic is reused.
4. **Status flows upward:** ESP32, daemon, DB (persist), then `*SyncEvent` to UI (and MQTT).

## 3. Repository Layout
 
```text
smart-home-gateway/
├── apps/                  # Contains: gateway-daemon, local-database, mqtt-connect, smart_gateway_ui
├── firmware/
│   ├── esp32-mesh/
│   │   ├── provisioner/                   # BLE Mesh Provisioner + UART bridge
│   │   ├── node_ble_mesh_actuator_ac_r/   # Actuator node (AC / relay)
│   │   ├── rpr_server/                    # Remote Provisioning server
│   │   └── unprov_dev/                    # Unprovisioned device
│   └── nrf52-mesh/                        # Reserved
├── yocto/
│   └── meta-ble-mesh/                     # Custom Yocto layer
├── poky/                  # Yocto reference distribution
├── build/                 # Yocto build dir (generated)
├── downloads/             # Yocto DL_DIR (generated)
├── sstate-cache/          # Yocto shared-state cache (generated)
├── images/                # Output images
├── Dockerfile             # Reproducible build environment
├── run_docker.sh          # Start the build container
├── build.sh               # Build the image
├── autoflash.sh           # Flash the image
└── README.md
```
 
`yocto/meta-ble-mesh` in detail:
 
```text
meta-ble-mesh/
├── conf/layer.conf
├── recipes-app/                 gateway-app, local-database-app, mqtt-connect, ui-app
├── recipes-apps-config/         apps-config (runtime configuration files)
├── recipes-connectivity/
│   ├── openssl/                 openssl_%.bbappend
│   └── wifi-config/             wifi.conf, wifi-connect.sh, wpa_supplicant units
├── recipes-kernel/
│   ├── linux/                   linux-raspberrypi_%.bbappend + fragment.cfg
│   └── rapi-dts-overlays/       screen_overlayer.dts
└── recipes-service/app-service/ daemon-uart, local-database, mqtt-connect, ui-qt (.service)
```
 
---

## 4. Background Knowledge
 
Each topic below follows the same pattern: **what it is**, **why the project uses it**, **where it appears**, and **useful commands**.


### 4.1 BLE Mesh

**What it is:**
Bluetooth Mesh is a many-to-many, multi-hop networking topology built on Bluetooth Low Energy (BLE). It uses BLE advertising channels for communication and operates on a Publish-Subscribe messaging pattern. Key concepts include:
*   **Node Features:** Relay (extends range), Proxy (allows smartphones to connect via GATT), Friend (caching), and Low Power (sleeps and polls data).
*   **Addressing:** Unicast (unique per element), Virtual (Label UUID hash), and Group (dynamic or fixed, e.g., all-relays).
*   **Models:** Standardized behaviors defining states and messages. Includes Server (holds states), Client (requests/changes states), and Control models. Messages are defined by unique Opcodes (GET, SET, STATUS).
*   **Provisioning:** A secure 5-step process (Beaconing, Invitation, ECDH Public Key Exchange, OOB Authentication, and Data Distribution) to securely add unprovisioned devices to the network. It also supports Remote Provisioning (PB-Remote) to add devices outside direct radio range.

**Why the project uses it (vs. Zigbee / LoRa):**
*   **Direct Smartphone Access:** Unlike Zigbee, BLE is native to smartphones/PCs. Devices can be controlled directly via the GATT Proxy feature without mandating a network gateway.
*   **Higher Bandwidth:** BLE 5.x offers data rates up to 1 Mbps, significantly faster than Zigbee's 250 Kbps limit (based on 802.15.4).
*   **Cost & Ecosystem:** The massive smartphone-driven scale of BLE makes its chipsets generally more cost-effective than Zigbee ICs.
*   **Practicality for Smart Homes:** While LoRa is excellent for WAN/long-range, operating in unlicensed ISM bands requires strict compliance with transmission power and duty cycle regulations, making it less ideal for high-density, real-time home automation. BLE Mesh is purpose-built for building automation and smart lighting.

**Where it appears in the project:**
*   **`firmware/esp32-mesh/provisioner/`**: The ESP32 acting as the network manager. It handles the 5-step provisioning process, securely assigning Unicast Addresses, NetKeys, and AppKeys to new nodes.
*   **`firmware/esp32-mesh/node_ble_mesh_actuator_ac_r/`**: The edge nodes implementing Server/Client Models. They subscribe to Group Addresses to receive `CMD_ACTUATOR_SET` messages (Opcode `0x32`) and publish `EVT_SENSOR_STATUS` (Opcode `0xB1`) back to the gateway.


### 4.2 ESP-IDF and ESP BLE Mesh
 
**What it is.** ESP-IDF is Espressif's SDK. It includes an ESP BLE Mesh stack. All `firmware/esp32-mesh/*` projects are ESP-IDF projects.
 
**Files you will see in each project**
 
| File / folder | Purpose |
|---|---|
| `CMakeLists.txt` | Top-level build file |
| `main/` | Application code (`app_main`) |
| `components/` | Local reusable components |
| `managed_components/`, `dependencies.lock` | Components downloaded by the IDF component manager. Do not edit by hand |
| `sdkconfig` | Generated config. Created from `sdkconfig.defaults*` and `menuconfig` |
| `sdkconfig.defaults`, `sdkconfig.defaults.<chip>` | Default options, applied per target chip |
| `partitions.csv` | Flash partition layout (app, NVS, ...). The BLE Mesh stack stores keys in NVS |
| `build/` | Build output (generated) |
 
**Typical workflow**
 
```bash
. $HOME/esp/esp-idf/export.sh            # load the toolchain (path may differ)
cd firmware/esp32-mesh/provisioner
idf.py set-target esp32                  # choose chip (esp32, esp32c3, esp32s3, ...)
idf.py menuconfig                        # Component config > Bluetooth > ESP BLE Mesh Support
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor     # Ctrl+] to exit the monitor
idf.py -p /dev/ttyUSB0 erase-flash       # wipe NVS (forget all provisioning data)
```
 
**Notes**
 
- Changing the target chip wipes `sdkconfig`. Keep your options in `sdkconfig.defaults`.
- If a node was provisioned and you change its role, run `erase-flash` so old keys are removed.
- Pick **Bluedroid** or **NimBLE** host in menuconfig. The `sdkconfig.ci.*` files show tested combinations.

 
### 4.3 UART framing, CRC16 and ARQ
UART is a raw byte stream with no message boundaries, no integrity check and no delivery guarantee. The project adds those three things.
 
**Frame format**
 
```text
START | opcode | addr (LE, 2B) | seq | type | len | payload (len bytes) | CRC16 (LE, 2B)
```
 
| Field | Role |
|---|---|
| `START` | Marks the beginning of a frame so the parser can resynchronise after garbage |
| `opcode` | What the frame means (command or event). See table below |
| `addr` | Target or source `element_addr` (little-endian) |
| `seq` | Sequence number used for ACK matching |
| `type` | Frame type (data, reliable data, ACK, ...) |
| `len` | Payload length |
| `CRC16` | Integrity check over the frame. |
 
**Opcodes**
 
| Opcode | Name | Direction |
|---|---|---|
| `0x0A` | `CMD_GROUP_ADD` | Gateway to ESP32 |
| `0x0B` | `CMD_GROUP_DELETE` | Gateway to ESP32 |
| `0x0C` | `CMD_MODEL_PUB_SET` | Gateway to ESP32 |
| `0x32` | `CMD_ACTUATOR_SET` | Gateway to ESP32 |
| `0x34` | `CMD_THRESHOLD_CONFIG` | Gateway to ESP32 |
| `0x40` | `CMD_ACTUATOR_AUTO` | Gateway to ESP32 |
| `0xB1` | `EVT_SENSOR_STATUS` | ESP32 to Gateway |
| `0xB2` | `EVT_ACTUATOR_STATUS` | ESP32 to Gateway |
| `0xD0` | `EVT_GROUP_STATUS` | ESP32 to Gateway |
 
**Receive path (daemon)**
 
1. `ReliableTransport::rx_loop` reads bytes from the UART.
2. `FrameParser::feed` finds `START`, collects `len` bytes and checks the CRC. A bad CRC means the frame is dropped.
3. For reliable frames, `AckBuilder::build_ack` sends an ACK with the same `seq`.
4. `FrameCodec::decode` turns bytes into a frame structure.
5. `MeshEventDispatcher::dispatch` routes by opcode to `Telemetry::onSensorStatus`, `onActuatorStatus` or `onGroupStatus`, which publish D-Bus events.

**How Stop-and-Wait ARQ works:**
To ensure reliable UART communication between the Linux Host and the ESP32 Network Co-Processor, the `ArqController` implements a Stop-and-Wait ARQ (Automatic Repeat reQuest) mechanism. 
*   **Success:** When the Gateway transmits a frame (`seq=N`), it halts the transmission queue and waits. If a matching ACK arrives within `ARQ_TIMEOUT_MS`, the next frame is allowed to proceed.
*   **Retry & Failure:** If the timeout expires without an ACK (due to noise or MCU busy state), the Gateway retransmits the exact same frame. This retry loop repeats up to `ARQ_MAX_RETRY` times, after which the system reports a communication failure to prevent silent packet drops.


On a Raspberry Pi Zero 2W, make sure the serial console on that UART is disabled and the UART is enabled (`enable_uart=1` in `config.txt`, no `console=serial0` in `cmdline.txt`).
### 4.4 D-Bus

**What it is.** A message bus for inter-process communication on Linux. There are two buses: the **system bus** (one per machine, system-wide, used here) and the **session bus** (one per user login). In this project, System D-Bus acts as an internal message broker enabling a decoupled, multi-process microservice architecture.

**Vocabulary**

| Term | Role | Project Mapping & Concrete Values |
|---|---|---|
| **Bus name** | Well-known connection name owned by each process | `com.gateway.mesh`, `com.gateway.db`, `com.gateway.mqtt`, `com.gateway.ui` |
| **Object path** | Hierarchical identifier of the exported object instance | `/com/gateway/mesh`, `/com/gateway/db`, `/com/gateway/mqtt`, `/com/gateway/ui` |
| **Interface** | Namespace grouping signals and methods | `com.gateway.mesh.events`, `com.gateway.db.events`, `com.gateway.ui.events`, `com.gateway.mqtt.events` |
| **Signal / member** | Broadcast event name emitted without waiting for replies | `UiActuatorCmd`, `ActuatorStatus`, `ActuatorSyncEvent`, `SensorDataStatus`, `SensorSyncEvent`, `GroupSyncEvent` |
| **Match rule** | Low-level bus filter configured by subscribers | Matches by `sender` / `interface` / `member` via `g_dbus_connection_signal_subscribe` or `QDBusConnection::connect` |

**Why signals (Pub/Sub) instead of point-to-point IPC (Unix Domain Sockets / Shared Memory)?**
* **Native 1-to-Many Fan-Out:** Hardware events such as `SensorDataStatus` or `ActuatorStatus` must be delivered concurrently to the local database (persistence), UI (real-time graphs), and MQTT bridge (cloud sync). D-Bus eliminates the need for `gateway-daemon` to maintain custom routing tables, connection tracking, or individual peer sockets.
* **Process Lifecycle Decoupling:** Modules start and stop independently. If `smart_gateway_ui` crashes or restarts, `gateway-daemon` and `local-database` continue running without socket broken-pipe errors (`SIGPIPE`). When the UI recovers, it requests full state resynchronization (`SyncAllNodesCmd`, `SyncAllGroupsCmd`).
* **Clean Event Boundaries:** Payloads are serialized using lightweight DTO JSON wrappers (`ipc_dto.h`) encapsulated inside GVariant/QDBusVariant strings (`(v)`), keeping transport parsing isolated from business logic.

**Rules that matter in this project**

- **Publisher Interface Ownership:** A module emits signals on **its own interface**. A listener must subscribe to the **publisher's interface**, not its own.
  - `gateway-daemon` emits on `com.gateway.mesh.events`.
  - `local-database` emits on `com.gateway.db.events`.
  - `smart_gateway_ui` emits on `com.gateway.ui.events`.
  - `mqtt-connect` emits on `com.gateway.mqtt.events`.
- **No Native In-Bus Queuing:** Unhandled signals are dropped if no subscriber is active. To prevent state loss, `local-database` acts as the single source of truth, committing data before notifying consumers.
- **Implementation Split:**
  - Daemons (`gateway-daemon`, `local-database`, `mqtt-connect`) use **GDBus / GLib** wrapped by `DBusGDBus` (`common/src/dbus.cpp`), running an event loop on a dedicated thread (`dbus_init_thread`) and dispatching tasks via `ThreadSafeQueue<IpcTask>`.
  - The UI uses **Qt D-Bus (`QDbusImpl`)** integrated with Qt's event loop via `QDBusEventAdapter`.

**System bus policy.**
On Linux, unprivileged processes cannot own well-known names or broadcast signals on the system bus without an explicit configuration policy. 
- **Policy Path:** `/etc/dbus-1/system.d/com.gateway.conf` (deployed via the Yocto recipe `recipes-apps-config/`).
- **Permissions:** Grants default user permissions to own names (`com.gateway.*`), send method calls, and receive broadcast signals across all four gateway interfaces.

**Useful Diagnostic Commands**

```bash
# Monitor all signals across all gateway interfaces
dbus-monitor --system "type='signal',interface='com.gateway.db.events'" \
                      "type='signal',interface='com.gateway.mesh.events'" \
                      "type='signal',interface='com.gateway.ui.events'" \
                      "type='signal',interface='com.gateway.mqtt.events'"

# Check active gateway service names currently registered on the bus
busctl --system list | grep com.gateway

# Introspect an active module object interface
busctl --system introspect com.gateway.mesh /com/gateway/m  esh com.gateway.mesh.events

# Manually trigger a database synchronization request from terminal
busctl --system emit /com/gateway/ui com.gateway.ui.events SyncAllNodesCmd "v" "{}"
```

### 4.5 Event-driven design and the DB as orchestrator
 
**Pattern.** The DB receives intent ("set group X with these members"), records it, then **orchestrates** the lower layer by emitting a series of commands. It later learns the result from events and updates its state.
 
**`mesh_applied` flag.** A group member has two states:
 
| `mesh_applied` | Meaning |
|---|---|
| `0` | Stored in the DB, but the mesh has not confirmed it |
| `1` | The ESP32 confirmed the publish/subscribe change via `GroupStatus` |
 
This is **eventual consistency**: the DB and the mesh are briefly different and converge when the status arrives. The UI sees both steps because `GroupSyncEvent` is emitted twice (once right after the commands are queued, once after confirmation).
 
**Idempotency.** Because ARQ may deliver a command twice, and the DB may re-send commands when a group is edited again, every command must be safe to repeat.
 
**Why re-publish instead of letting the UI talk to the daemon?** The DB keeps one authoritative history. It also gives one place to add validation, ordering and retries.


### 4.6 SQLite
 
**What it is.** An embedded, file-based SQL database. No server process.
 
**Why here.** Small footprint, ACID transactions, good enough for a gateway. The DB module is the only writer, so there are no cross-process locking problems.
 
**Data model**
 
| Table | Content |
|---|---|
| node | Provisioned devices and their `element_addr` |
| actuator | Target and present setpoint, on/off, status |
| sensor_reading | Time series (temperature, humidity, soil, lux, motion, battery) |
| mesh_group | Automation rules (name, auto mode, sensor type, thresholds) |
| group_member | Membership with role (sensor / actuator) and `mesh_applied` |

### 4.7 MQTT and HiveMQ Cloud
 
**What it is.** A lightweight publish/subscribe protocol over TCP. Clients connect to a **broker** (HiveMQ Cloud here).
 
| Concept | Notes |
|---|---|
| Topic | Hierarchical string, for example `gateway/nodes/0x0005/sensors` |
| Wildcards | `+` matches one level, `#` matches the rest. `gateway/nodes/+/command` matches every node |
| QoS 0 | At most once. Used for frequent sensor data |
| QoS 1 | At least once. Used for commands and actuator status (duplicates possible) |
| QoS 2 | Exactly once (not used here) |
| Retain | Broker stores the last message and gives it to new subscribers. Used for `sync_groups` |
| TLS | HiveMQ Cloud requires TLS on port **8883** with username and password |
| Last Will | Optional message published if the client drops. Useful for "gateway offline" |
 
**Topics in this project**
 
| Direction | Topic | QoS | Handler |
|---|---|---|---|
| Cloud to Gateway | `gateway/nodes/+/command` | 1 | `MqttTranslator::handle_actuator_command` |
| Gateway to Cloud | `gateway/nodes/<addr>/sensors` | 0 | `MqttTranslator::handle_sensor` |
| Gateway to Cloud | `gateway/nodes/<addr>/actuator` | 1 | `MqttTranslator::handle_actuator_status` |
| Gateway to Cloud | `gateway/telemetry/sync_groups` | 1, retain | `MqttTranslator::handle_group_sync_status` |
 
Command payload:
 
```json
{ "actuator_type": 1, "device_type": 1, "onoff": 1, "setpoint": 24 }
```
### 4.8 Qt / QML / QtCharts
 
**What it is.** Qt is a C++ UI framework. QML is its declarative UI language. C++ objects (for example `Controller`) are exposed to QML.
 
**Concepts used**
 
- **Signals and slots.** The UI calls a C++ slot or emits a signal such as `reqSendActuator`. The IPC layer (`IPCEvent`) connects it to a D-Bus publish. Incoming D-Bus events are re-emitted as Qt signals (`actuatorSyncReceive`, `sensorSyncReceive`, ...) and handled by `Controller::onActuatorsSynced`, `onSensorsSynced`, `onGroupsSynced`.
- **Models.** Lists shown in QML come from `QAbstractListModel` or similar C++ models.
- **QtCharts.** `Chart.qml` appends points to line series to draw sensor curves.
- **Thread rule.** UI objects live in the main thread. D-Bus callbacks must hop into it (queued signals).
**On the target (no desktop).** The UI runs full screen without X11/Wayland. Since the ILI9341 operates over SPI and is mapped as a secondary framebuffer (`/dev/fb1`), the UI strictly uses the `linuxfb` platform plugin with software rendering to ensure maximum stability on the Pi Zero 2 W:
 
```bash
Environment=QT_QPA_PLATFORM=linuxfb:fb=/dev/fb1
Environment=QT_QUICK_BACKEND=software
Environment=QT_QPA_GENERIC_PLUGINS=evdevtouch:/dev/input/event0
```
 
### 4.9 systemd
 
**What it is.** The init system. It starts services at boot in dependency order and restarts them if they crash.
 
**Units in this project** (`recipes-service/app-service/files/`)
 
| Unit | Starts |
|---|---|
| `local-database.service` | DB module |
| `daemon-uart.service` | `gateway-daemon` |
| `mqtt-connect.service` | MQTT bridge |
| `ui-qt.service` | Qt UI |
 
Typical unit anatomy:
 
```ini
[Unit]
Description=Gateway daemon
After=dbus.service local-database.service     
Requires=dbus.service
 
[Service]
ExecStart=/usr/bin/gateway-daemon            
Restart=on-failure
 
[Install]
WantedBy=multi-user.target
```
 
**Commands**
 
```bash
systemctl status daemon-uart.service
systemctl restart local-database.service
journalctl -u daemon-uart.service -f          
journalctl -b -p err                        
systemctl list-dependencies multi-user.target
```
 
Start order matters: D-Bus first, then the DB, then daemon / MQTT / UI. If the DB starts late, signals sent earlier are lost. Use `After=` and consider a full sync request at start-up.



### 4.10 Yocto Project
 
**What it is.** A framework to build a custom embedded Linux distribution from source.
 
| Term | Meaning |
|---|---|
| Poky | Yocto's reference distribution (the `poky/` folder) |
| BitBake | The build engine |
| Layer | A folder of recipes and config (`meta-ble-mesh` is yours) |
| Recipe (`.bb`) | How to fetch, build and install one package |
| `.bbappend` | Modify an existing recipe without copying it (`openssl_%.bbappend`, `linux-raspberrypi_%.bbappend`) |
| Image | A list of packages that forms the root filesystem |
| `MACHINE` | Target hardware (a Raspberry Pi variant) |
| `DISTRO` | Distribution policy |
| `local.conf` / `bblayers.conf` | Build configuration / list of layers (in `build/conf/`) |
| `DL_DIR`, `SSTATE_DIR` | Source download cache / build-result cache (`downloads/`, `sstate-cache/`) |
 
 
**What each of your recipes does**
 
| Recipe | Purpose |
|---|---|
| `gateway-app`, `local-database-app`, `mqtt-connect`, `ui-app` | Build and install the four modules |
| `apps-config` | Install runtime config files |
| `app-service` | Install the systemd units |
| `wifi-config` | Install Wi-Fi scripts and `wpa_supplicant` config |
| `openssl_%.bbappend` | Adjust OpenSSL (TLS for MQTT) |
| `linux-raspberrypi_%.bbappend` + `fragment.cfg` | Add kernel options |
| `screen-overlayer` | Install the display device tree overlay |
 
The `%` in `openssl_%.bbappend` matches any version of the recipe.
 
**Everyday commands**
 
```bash
source poky/oe-init-build-env build              # enter the build environment
bitbake-layers show-layers                       # confirm meta-ble-mesh is listed
bitbake-layers add-layer ../yocto/meta-ble-mesh  # add the layer if missing
bitbake <your-image-name>                        
bitbake -c cleansstate gateway-app               # force rebuild of one recipe
bitbake -e gateway-app | grep ^SRC_URI           # inspect recipe variables
```
 
Output is under `build/tmp/deploy/images/<machine>/`.
 
**Why Docker.** Yocto needs specific host packages and is sensitive to the host distribution. The `Dockerfile` pins that environment so every developer gets the same result.
 
**Disk and time.** A first build takes hours and tens of GB. Keep `downloads/` and `sstate-cache/` between builds; they make later builds fast.
 
### 4.11 Device tree overlays
 
**What it is.** The device tree describes hardware to the kernel. An **overlay** adds or modifies nodes at boot without rebuilding the base tree.
 
In this project `screen_overlayer.dts` describes the touch display. The `screen-overlayer` recipe compiles it with `dtc` into a `.dtbo`, which the Raspberry Pi firmware loads through `config.txt`:
 
```text
dtoverlay=screen_overlayer       
```


### 4.12 Wi-Fi and TLS on the target
 
- `wifi-init.service` and `wifi-connect.service` plus `wifi-connect.sh` bring up Wi-Fi at boot. Credentials are in `wifi.conf` and the `wpa_supplicant-wlan0.conf` template.
- `wpa_supplicant@wlan0.service` manages the association.
- MQTT over TLS needs correct **time** (certificate validity) and a CA bundle. If TLS fails right after boot, check the clock (`timedatectl`) and that NTP is reachable.
### 4.13 Concurrency patterns used in the daemon
 
The daemon uses producer/consumer threads with queues:
 
| Thread / loop | Role |
|---|---|
| `ReliableTransport::tx_loop` | Takes frames from the TX queue and sends them through ARQ |
| `ReliableTransport::rx_loop` | Reads bytes from the UART and feeds the parser |
| `ReliableTransport::dispatch_loop` | Hands decoded frames to a callback |
| `Gateway::worker_loop` | Pulls mesh events and runs the dispatcher |
| `DBusGDBus::worker_thread_loop` | Runs D-Bus callbacks |
 
Core tools: `std::mutex`, `std::condition_variable`, thread-safe queues. Why separate threads? Reading the UART must never block on slow D-Bus work, and a send waiting for an ACK must not block receiving.
 
## 5. Module Reference
 
| Module | Recipe | systemd unit | D-Bus publishes on | Subscribes to |
|---|---|---|---|---|
| `smart_gateway_ui` | `ui-app` | `ui-qt.service` | `ui.events` | `ActuatorSyncEvent`, `SensorSyncEvent`, `GroupSyncEvent`, `NodeSyncEvent` |
| `local-database` | `local-database-app` | `local-database.service` | `db.events` | `Ui*` and group commands (UI, MQTT), `ActuatorStatus`, `SensorDataStatus`, `GroupStatus`, `NodeInfo`, `HeartbeatEvent` |
| `gateway-daemon` | `gateway-app` | `daemon-uart.service` | `mesh.events` | `MeshCmd*`, `Group*Cmd` (DB), `UuidWhitelistCmd` (UI, MQTT) |
| `mqtt-connect` | `mqtt-connect` | `mqtt-connect.service` | `mqtt.events` | `GroupSyncEvent`, `NodeSyncEvent`, mesh events (see Known Issues) |
 
## 6. Message Flows

The system operates on an event-driven architecture utilizing System D-Bus and MQTT.

### Flow 1: Local Command (UI to Device)
Describes how a user interaction on the Qt/QML HMI is processed. The command is routed to the local SQLite database for persistence before being re-broadcast to the Gateway Daemon, which encodes it into a robust UART frame (Opcode + CRC16) for the ESP32 Provisioner.

![Local Command Flow](images/flow1.png)

### Flow 2: Actuator Status Feedback
Details the state synchronization after a hardware execution. Once the ESP32 reports a successful toggle, the Daemon emits a D-Bus signal. The Database captures this, updates its records, and simultaneously triggers both the UI (clearing the "pending" visual state) and the MQTT bridge (updating the cloud dashboard).

![Actuator Status Feedback](images/flow2.png)

### Flow 3: Sensor Telemetry
Illustrates the asynchronous handling of environmental data. Sensor packets arriving via UART are dispatched over D-Bus, where they are concurrently consumed by the Database (for historical time-series storage), the UI (to update live QtCharts), and the MQTT module (to push to HiveMQ).

![Sensor Telemetry Flow](images/flow3.png)

### Flow 4: Edge Automation Config Orchestration
Highlights the Database's role as a system orchestrator. A complex Automation Rule created on the UI is broken down by the Database into multiple hardware-level commands (Thresholds, Auto-Mode, Subscriptions). The Database manages a `mesh_applied` synchronization flag, ensuring the UI accurately reflects whether the mesh network has successfully applied the new configuration.

![Edge Automation Config Flow](images/flow4.png)

### Flow 5: Cloud Remote Control (Virtual UI)
Demonstrates the "Unified Pipeline" pattern. The MQTT bridge acts as a Virtual UI, translating incoming cloud payloads into the exact same local D-Bus signals emitted by the physical HMI. 

![Cloud Remote Control Flow](images/flow5.png)


## 7. Configuration, Build, and Flash Guide

Before building the Yocto image, configure the Wi-Fi credentials and the MQTT broker settings. They are baked into the root filesystem.

> **Security note:** these files contain secrets. Do not commit real credentials to Git. Keep a template in the repository and your real values only on your machine.

### 7.1 Clone the Repository

```bash
git clone https://github.com/bluebirdvn/smart-home-gateway.git
cd smart-home-gateway
```

### 7.2 Configuration Steps

#### A. Wi-Fi Setup

The Raspberry Pi needs internet access to reach HiveMQ Cloud.

- **File:** `yocto/meta-ble-mesh/recipes-connectivity/wifi-config/files/wifi.conf`
- **Action:** edit the file with your local Wi-Fi credentials.

```bash
WIFI_SSID="Your_WiFi_Name"
WIFI_PASSWORD="Your_WiFi_Password"
```

#### B. MQTT Broker Credentials (HiveMQ Cloud)

The `mqtt-connect` daemon reads its connection settings from a JSON file.

- **File:** `yocto/meta-ble-mesh/recipes-apps-config/files/mqtt_config.json`
- **Action:** replace the placeholders with your HiveMQ Cloud cluster details. Keep `use_tls` set to `true` and the port set to `8883`.


```json
{
  "host": "your-cluster-id.hivemq.cloud",
  "port": 8883,
  "client_id": "rpi_gateway_01",
  "username": "your_hivemq_user",
  "password": "your_hivemq_password",
  "use_tls": true,
  "verify_server": true
}
```

#### C. System D-Bus Configuration (Optional)

The D-Bus object paths and interfaces are defined in `config.json`.

- **File:** `yocto/meta-ble-mesh/recipes-apps-config/files/config.json`
- **Action:** you normally do not need to change it, unless you add new services to the architecture.

### 7.3 Build the Yocto Image

The project provides a reproducible build environment based on Docker, which avoids host OS incompatibilities.

**Prerequisites**

- Linux host (Ubuntu 20.04 or 22.04 recommended)
- Docker installed
- About 100 GB of free disk space

```bash
# 1. Start and enter the Yocto build container
./run_docker.sh

# 2. Initialize the OpenEmbedded build environment
source poky/oe-init-build-env build

# 3. Verify that the custom meta-layer is included
bitbake-layers show-layers | grep meta-ble-mesh

# 4. Build the final image (replace gateway-image with your real image recipe name)
bitbake gateway-image
```

> **Note:** the first build downloads all sources and compiles the toolchain from scratch, which can take several hours depending on your CPU. Later builds are much faster thanks to `sstate-cache`.

### 7.4 Flash the SD Card

When the build finishes, the output image (`.wic` or `.wic.bz2`) is in `build/tmp/deploy/images/raspberrypi0-2w-64/`.

```bash
# Flash the image to your SD card (/dev/sdX).
# WARNING: double-check the device path, or you may wipe your host drive.
sudo bmaptool copy \
  build/tmp/deploy/images/raspberrypi0-2w-64/gateway-image.wic.bz2 /dev/sdX

# Alternatively, use the provided script:
./autoflash.sh /dev/sdX
```

Use `lsblk` to find the correct device name before flashing.

### 7.5 Display & Touch Screen Setup (ILI9341)

The UI runs on an SPI-based ILI9341 TFT display with a touch controller (typically XPT2046/ADS7846). It communicates directly with the Raspberry Pi via the SPI bus, requiring no HDMI connection.

**1. Hardware Wiring (SPI)**
*(Note: Always verify the exact GPIO pins defined in your custom device tree overlay `yocto/meta-ble-mesh/recipes-kernel/rapi-dts-overlays/files/screen_overlayer.dts`.)*

| ILI9341 Pin | Raspberry Pi Zero 2 W | Notes |
|---|---|---|
| **VCC / VDD** | 3.3V or 5V (Pin 1 or 2) | Check your specific screen module's voltage |
| **GND** | GND (Pin 6) | Common Ground |
| **MOSI** | GPIO 10 (Pin 19) | SPI0 MOSI (Data to screen) |
| **MISO** | GPIO 9 (Pin 21) | SPI0 MISO (Data from touch) |
| **SCLK** | GPIO 11 (Pin 23) | SPI0 Clock |
| **CS** | GPIO 8 (Pin 24) | SPI0 CE0 (Chip Select for LCD) |
| **DC / RS** | GPIO 24 (Pin 18) | Data/Command control pin |
| **RESET** | GPIO 25 (Pin 22) | LCD Reset |
| **LED** | 3.3V (Pin 17) | Backlight power |
| **T_CS (Touch)** | GPIO 7 (Pin 26) | SPI0 CE1 (Chip Select for Touch) |
| **T_IRQ (Touch)**| GPIO 17 (Pin 11) | Hardware interrupt for touch events |

**2. Software Configuration (How it works under the hood)**
*   **Device Tree:** The custom Yocto recipe `screen-overlayer` compiles `screen_overlayer.dts` into a `.dtbo` binary. During boot, the Pi's firmware reads `dtoverlay=screen_overlayer` from `config.txt` and loads the Linux framebuffer driver (typically `fb_ili9341`) and the touch driver (`ads7846`).
*   **Qt/QML:** To maximize performance and minimize RAM usage on the Pi Zero 2 W, the UI application (`ui-qt.service`) bypasses heavy desktop environments like X11 or Wayland. It renders directly to the framebuffer using `QT_QPA_PLATFORM=linuxfb`. Touch interactions are automatically routed via the Linux `evdev` input subsystem.

### 7.6 Optimizing UI Refresh Rate (SPI and Core Clock)

Driving a Qt UI over SPI can feel sluggish or show tearing when the bus bandwidth is too low. The ILI9341 panel is 320x240 with 16-bit color (RGB565), so one full frame is:

```text
320 x 240 x 16 bit = 1,228,800 bit  (about 1.23 Mbit)
```

To refresh the whole screen 30 times per second the bus must carry about 37 Mbit/s of pixel data, and real transfers add overhead (chip select, DC line, command bytes, kernel scheduling). The table below shows the best case: the highest frame rate each SPI clock allows if nothing else uses the bus.

| SPI clock | Theoretical max full-screen FPS |
|---|---|
| 16 MHz | 13 |
| 32 MHz | 26 |
| 40 MHz | 32 |
| 48 MHz | 39 |
| 50 MHz | 40 |

So a 32 MHz clock cannot reach 30 FPS with full-screen updates, and 40 to 50 MHz is a realistic target. Updating only the changed screen area (dirty rectangles) lowers the load much more than any clock change.

#### 1. Increase the SPI maximum frequency (device tree)

Many display drivers default to 16 MHz or 32 MHz. In the custom overlay (`screen_overlayer.dts`), raise `spi-max-frequency`:

```dts
/* Snippet inside screen_overlayer.dts */
ili9341: display@0 {
    compatible = "ilitek,ili9341";
    reg = <0>;
    spi-max-frequency = <50000000>;   /* see the divider note below */
    rotate = <90>;
    bgr = <1>;
    fps = <60>;                       /* upper limit for the refresh rate */
};
```

> **Notes**
>
> - The ILI9341 datasheet specifies a much lower write clock than 50 MHz. Many boards work at 40 to 60 MHz in practice, but this is outside the specification. It depends on short wires, a good ground, and your panel. If you see wrong colors or noise, lower the clock.
> - Properties such as `rotate`, `bgr` and `fps` belong to the `fbtft` driver. A DRM driver for the ILI9341 uses different properties (for example `rotation`). Use the set that matches the driver in your kernel.
> - Keep `fps` at or above your target. A value of `30` makes the driver limit the display to 30 FPS.

#### 2. The Raspberry Pi core clock

The SPI0 clock is derived from the core clock (`core_freq`). The Raspberry Pi SPI driver divides the core clock by an **even number**, and it rounds the divider **up**. The real SPI clock can therefore be lower than the value you request:

| `core_freq` | Requested SPI clock | Divider | Real SPI clock |
|---|---|---|---|
| 400 MHz | 48 MHz | 10 | 40 MHz |
| 400 MHz | 50 MHz | 8 | 50 MHz |
| 500 MHz | 48 MHz | 12 | 41.7 MHz |
| 500 MHz | 50 MHz | 10 | 50 MHz |

For this reason the overlay above requests 50 MHz instead of 48 MHz: it divides evenly at both 400 MHz and 500 MHz.

On some Raspberry Pi models the firmware changes `core_freq` together with CPU frequency scaling. When the system is idle the core clock drops, the SPI clock drops with it, and the UI stutters at the next touch. To prevent this, lock the core clock in the boot configuration:

```ini
# Append to config.txt
core_freq=400
core_freq_min=400
```


Where to put these lines:

- **Yocto (recommended):** set `RPI_EXTRA_CONFIG` in `local.conf` or in a `.bbappend` so the setting is part of every image:

  ```bitbake
  RPI_EXTRA_CONFIG = "core_freq=400\ncore_freq_min=400\n"
  ```

- **Directly on the SD card:** edit `/boot/config.txt` in the boot partition.

#### 3. Verify on the target

```bash
# Core clock in Hz (should stay constant)
vcgencmd measure_clock core

# Check the real SPI speed requested by the driver
dmesg | grep -i -E "spi|ili9341|fb"
```

Watch the core clock while the system is idle and while the UI is busy. If the value changes, the lock is not applied.

> **Note:** locking the core clock stops dynamic down-clocking, so the SPI clock stays at its target. It also raises idle power and heat slightly. This matters on a small board without a heatsink.


### 7.7 First Boot Checklist

Insert the SD card into the Raspberry Pi Zero 2 W, power it on, and connect through SSH (or attach a keyboard and monitor). Then run these checks.

1. **Wi-Fi:**

   ```bash
   ping google.com
   ```

2. **Microservices:**

   ```bash
   systemctl status local-database daemon-uart mqtt-connect ui-qt
   ```

   All services should show `active (running)`. If one has failed, read its log:

   ```bash
   journalctl -u <service_name> -e
   ```

3. **MQTT connection:**

   ```bash
   journalctl -u mqtt-connect -f
   ```

   Look for the "Connected to broker" message.

4. **UART link:** make sure the ESP32 Provisioner is wired to the Raspberry Pi UART pins and is powered on.

   | Raspberry Pi | Physical pin | ESP32 Provisioner |
   |---|---|---|
   | TX (GPIO14) | Pin 8 | RX |
   | RX (GPIO15) | Pin 10 | TX |
   | GND | any GND pin | GND |

   Cross the lines: Pi TX goes to ESP32 RX, and Pi RX goes to ESP32 TX. Connect the grounds as well.


### 7.8 Fast Application Update (For Developers)

If you change the C++ source in `apps/` and only want to update the binary, you do not need to rebuild the whole OS.

```bash
bitbake -c cleansstate gateway-app && bitbake gateway-app
```

Then copy the new binary to the Raspberry Pi and restart the service:

```bash
scp build/tmp/work/<path-to-binary>/gateway-daemon root@<pi-ip>:/usr/bin/
ssh root@<pi-ip> systemctl restart daemon-uart
```


## 8. Firmware Guide (ESP32)
 
The edge network is powered by ESP32 microcontrollers (typically ESP32-C6/H2 for their native BLE 5.0/NimBLE support). They run FreeRTOS, ensuring strict timing for BLE Mesh flooding and advertising.

### 8.1 Hardware Setup & UART Link
The gateway daemon running on the Raspberry Pi communicates with the ESP32 Provisioner via a dedicated UART bridge.
*   **RPi GPIOs:** `TX (Pin 8)`, `RX (Pin 10)`, `GND`.
*   **ESP32 GPIOs:** Mapped in `sdkconfig` or `main.c` (e.g., `GPIO 17` for TX, `GPIO 16` for RX).
*   **Baud Rate:** `115200 8N1` (No parity, 1 stop bit).
*   **Level Shifting:** Both RPi and ESP32 use 3.3V logic, so they can be connected directly (Tx to Rx, Rx to Tx) along with a common Ground.

### 8.2 BLE Mesh Models Implementation
To support both standard smart home integration and custom edge automation, the firmware utilizes two types of BLE Mesh Models:
1.  **SIG Standard Models:** 
    *   `Configuration Server/Client`: Manages keys, subscriptions, and publication settings.
    *   `Generic OnOff Server/Client`: Used for basic Actuator control (Relay on/off, Light toggle).
2.  **Custom Vendor Models (`0x0001` - `0x0005`):**
    *   Since SIG standard models do not natively support complex rules (like AC temperature thresholds or auto-mode flags), we define **Vendor Models** (identified by our custom Company ID `0xFFFF`). 
    *   These models handle specialized opcodes (`CMD_THRESHOLD_CONFIG`, `CMD_ACTUATOR_AUTO`) and telemetry (`EVT_SENSOR_STATUS`).

### 8.3 Firmware Roles & Projects

#### A. The Provisioner (`firmware/esp32-mesh/provisioner/`)
This is the "Network Co-Processor" (NCP) attached directly to the Raspberry Pi.
*   **Role:** It does not act as a sensor or actuator. It manages the network and acts as the bridge.
*   **UART to Mesh:** It listens to UART interrupts. When it receives a frame from the RPi (e.g., `CMD_ACTUATOR_SET`), it translates it to an `esp_ble_mesh_client_model_send_msg()` call.
*   **Mesh to UART:** It listens for incoming mesh messages (telemetry, acks) via `ESP_BLE_MESH_MODEL_OP_STATUS` and encodes them into UART frames sent back to the Pi.
*   **Provisioning Flow:** 
    1. It scans for unprovisioned beacons and sends an `UnprovAdvEvent` to the Gateway. 
    2. If the UI whitelists the UUID, the RPi sends `CMD_ADD_UNPROV_DEV`. 
    3. The Provisioner triggers `esp_ble_mesh_provisioner_add_unprov_dev()`. 
    4. Upon success, it fires `PROV_COMPLETE_COMP_EVT` and sends `NodeInfo` back to the Gateway.

#### B. The Actuator/Sensor Nodes (`firmware/esp32-mesh/node_ble_mesh_actuator_ac_r/`)
These are the edge devices scattered around the house.
*   **Sensors (Temp, Humidity, Lux, PIR):** They run a hardware timer (e.g., every 30 seconds) or hardware interrupts (PIR). They read data via I2C/ADC, format it, and **Publish** the data to a designated **Group Address** (configured by the Gateway).
*   **Actuators (Relay, AC, Lights):** They **Subscribe** to the Gateway's commands and, more importantly, to the Sensor's Group Address.
*   **Zero-Latency Edge Automation:** When an Actuator receives a `CMD_ACTUATOR_AUTO` command, its `is_auto` flag is set to `true`. From then on, if a Sensor publishes a threshold-exceeding value to the Group Address, the Actuator toggles its GPIO **locally and instantly**, completely bypassing the Raspberry Pi and Wi-Fi network.

#### C. Remote Provisioning Server (`firmware/esp32-mesh/rpr_server/`)
*   Supports the PB-Remote bearer. It allows the main Provisioner to securely add devices that are physically located out of its direct Bluetooth radio range by routing the provisioning packets through intermediate nodes.

### 8.4 Typical Bring-up Sequence
1.  **Flash the Provisioner:** Build and flash the `provisioner` project to the ESP32 connected to the RPi. Reboot the RPi daemon to establish the UART handshake.
2.  **Flash a Node:** Build and flash `node_ble_mesh_actuator_ac_r` to a separate ESP32. Turn it on.
3.  **Discovery:** Check the Qt UI (or log) for the Unprovisioned Device UUID.
4.  **Provision:** Click "Provision" on the UI. The Gateway orchestrates the key exchange. Once done, the UI will display the new device with its assigned Unicast Address and Model features.
5.  **Configure:** Assign the node to a Group to begin Edge Automation.


## 9. Further Reading
 
| Topic | Link |
|---|---|
| Bluetooth Mesh specification | https://www.bluetooth.com/specifications/specs/mesh-protocol/ |
| ESP BLE Mesh guide | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/esp-ble-mesh/ble-mesh-index.html |
| ESP-IDF programming guide | https://docs.espressif.com/projects/esp-idf/ |
| D-Bus specification | https://dbus.freedesktop.org/doc/dbus-specification.html |
| MQTT specification | https://mqtt.org/mqtt-specification/ |
| HiveMQ Cloud docs | https://docs.hivemq.com/ |
| Yocto Project documentation | https://docs.yoctoproject.org/ |
| meta-raspberrypi | https://github.com/agherzan/meta-raspberrypi |
| Qt documentation | https://doc.qt.io/ |
| SQLite documentation | https://www.sqlite.org/docs.html |
| systemd manual | https://www.freedesktop.org/software/systemd/man/ |
 
## 10. License
 
MIT. See [`yocto/meta-ble-mesh/COPYING.MIT`](yocto/meta-ble-mesh/COPYING.MIT).
