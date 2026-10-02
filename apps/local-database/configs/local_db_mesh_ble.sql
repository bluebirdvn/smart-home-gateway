CREATE TABLE IF NOT EXISTS node (
    element_addr INTEGER PRIMARY KEY,       
    uuid         TEXT NOT NULL,            
    name         TEXT NOT NULL DEFAULT '',
    kind         TEXT NOT NULL DEFAULT 'unknown',  
    unicast      INTEGER NOT NULL DEFAULT 0,       
    elem_num     INTEGER NOT NULL DEFAULT 1,
    net_idx      INTEGER NOT NULL DEFAULT 0,
    company_id   INTEGER NOT NULL DEFAULT 65535,
    model_id     INTEGER NOT NULL DEFAULT 0,
    features     INTEGER NOT NULL DEFAULT 0,      
    is_online    INTEGER NOT NULL DEFAULT 0,
    last_seen    INTEGER NOT NULL DEFAULT 0,
    created_at   INTEGER NOT NULL DEFAULT (unixepoch()) 
);
CREATE INDEX IF NOT EXISTS idx_node_unicast ON node(unicast) WHERE unicast <> 0;

CREATE TABLE IF NOT EXISTS sensor_reading (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    element_addr  INTEGER NOT NULL REFERENCES node(element_addr) ON DELETE CASCADE,
    temperature REAL NOT NULL DEFAULT 0,
    humidity    REAL NOT NULL DEFAULT 0,
    soil_moisture REAL NOT NULL DEFAULT 0,
    lux         REAL NOT NULL DEFAULT 0,
    motion      INTEGER NOT NULL DEFAULT 0,
    battery     INTEGER NOT NULL DEFAULT 0,
    ts          INTEGER NOT NULL DEFAULT (unixepoch())
);
CREATE INDEX IF NOT EXISTS idx_sensor_node_ts ON sensor_reading(element_addr, ts DESC);
CREATE INDEX IF NOT EXISTS idx_sensor_ts      ON sensor_reading(ts);

CREATE TABLE IF NOT EXISTS actuator (
    element_addr  INTEGER PRIMARY KEY REFERENCES node(element_addr) ON DELETE CASCADE,
    actuator_type    INTEGER NOT NULL DEFAULT 0,
    present_setpoint REAL NOT NULL DEFAULT 0,
    target_setpoint  REAL NOT NULL DEFAULT 0,
    present_onoff    INTEGER NOT NULL DEFAULT 0,
    target_onoff     INTEGER NOT NULL DEFAULT 0,
    status           INTEGER NOT NULL DEFAULT 0,
    threshold_src_addr INTEGER NOT NULL DEFAULT 0,
    is_auto          INTEGER NOT NULL DEFAULT 0,
    threshold_on     REAL NOT NULL DEFAULT 0,
    threshold_off    REAL NOT NULL DEFAULT 0,
    threshold_type   INTEGER NOT NULL DEFAULT 0,
    updated_at       INTEGER NOT NULL DEFAULT (unixepoch()),
);

CREATE TABLE IF NOT EXISTS mesh_group (
    group_id      INTEGER PRIMARY KEY AUTOINCREMENT,
    group_addr    INTEGER NOT NULL UNIQUE CHECK (group_addr BETWEEN 0xC000 AND 0xFEFF),
    name          TEXT NOT NULL DEFAULT '',
    is_auto_mode  INTEGER NOT NULL DEFAULT 1,
    sensor_type   INTEGER NOT NULL DEFAULT 0,
    threshold_on  REAL NOT NULL DEFAULT 0,
    threshold_off REAL NOT NULL DEFAULT 0,
    created_at    INTEGER NOT NULL DEFAULT (unixepoch())
);

CREATE TABLE IF NOT EXISTS mesh_group_member (
    group_id     INTEGER NOT NULL REFERENCES mesh_group(group_id) ON DELETE CASCADE,
    element_addr      INTEGER NOT NULL REFERENCES node(element_addr)        ON DELETE CASCADE,
    role         TEXT    NOT NULL CHECK (role IN ('sensor','actuator')),
    mesh_applied INTEGER NOT NULL DEFAULT 0,
    applied_at   INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY (group_id, element_addr, role)
);
CREATE INDEX IF NOT EXISTS idx_member_node ON mesh_group_member(element_addr);

CREATE TABLE IF NOT EXISTS uuid_whitelist (
    uuid        TEXT PRIMARY KEY,                 
    name        TEXT NOT NULL DEFAULT '',          
    status      INTEGER NOT NULL DEFAULT 0,        
    created_at  INTEGER NOT NULL DEFAULT (unixepoch())
);

PRAGMA user_version = 1;