file (GLOB_RECURSE C_SRCS "matter.cpp" "render.cpp" "*.c")
file (GLOB_RECURSE CPP_SRCS "*.cpp")

idf_component_register(SRCS ${C_SRCS} ${CPP_SRCS}
                    INCLUDE_DIRS ".")

# Required by matter
set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")