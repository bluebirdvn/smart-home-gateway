set(srcs "main.cpp"
    "board.cpp")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS  ".")
