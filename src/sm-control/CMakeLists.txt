set(SRC
    sm_control.c
)

include_directories(${SANCUS_INCLUDES})
add_library(sm-control STATIC ${SRC})
target_link_libraries(sm-control
                      loader
                      ${SANCUS_MODULE_LIBS} ${SANCUS_HOST_LIBS})
install(TARGETS sm-control
        EXPORT ExportTargets
        ARCHIVE DESTINATION ${INSTALL_LIBS_DIR})
