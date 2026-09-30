set(srcs "main.cpp")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS  "."
                    REQUIRES gateway_core)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)