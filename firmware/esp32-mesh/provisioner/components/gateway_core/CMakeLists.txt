idf_component_register (
    SRCS
        "config/frame_codec.cpp"
        "mesh/mesh_dispatcher.cpp"
        "mesh/mesh_sender.cpp"

        "provisioner/device_management.cpp"
        "provisioner/provisioner.cpp"

        "transport/ack_builder.cpp"
        "transport/arq_controller.cpp"
        "transport/crc_calculator.cpp"
        "transport/frame_praser.cpp"
        "transport/reliable_transport.cpp"
        "transport/serial_port.cpp"
    INCLUDE_DIRS
        "config"
        "mesh"
        "provisioner"
        "transport"
    REQUIRES bt driver freertos nvs_flash
)