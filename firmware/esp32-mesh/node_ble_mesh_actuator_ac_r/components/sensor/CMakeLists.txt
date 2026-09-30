idf_component_register (
    SRCS
        "config/aht30_sensor.cpp"
        "config/aht30_sensor.c"
        "config/bh1750_i2c_hal.c"
        "config/bh1750_i2c.c"
        "config/bh1750.cpp"
        "config/sensor_manager.cpp"
        "config/actuator_manager.cpp"
        "config/light_actuator.cpp"
        "config/relay_actuator.cpp"
        "config/pir_sensor.cpp"
        "config/ac_actuator.cpp"
    INCLUDE_DIRS
        "config"
    EMBED_FILES "config/1064.json"

    REQUIRES driver freertos nvs_flash json mbedtls esp_timer
)