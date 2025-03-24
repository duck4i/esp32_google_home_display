file (GLOB_RECURSE C_SRCS "*.c")
file (GLOB_RECURSE CPP_SRCS "*.cpp")

idf_component_register(SRCS ${C_SRCS} ${CPP_SRCS}
                    INCLUDE_DIRS ".")
