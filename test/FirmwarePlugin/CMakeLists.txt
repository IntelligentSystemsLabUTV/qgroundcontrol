# ============================================================================
# Firmware Plugin Unit Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MedeaRoverFirmwarePluginTest.cc
        MedeaRoverFirmwarePluginTest.h
        MedeaRoverJobStatusTest.cc
        MedeaRoverJobStatusTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(MedeaRoverFirmwarePluginTest LABELS Unit)
add_qgc_test(MedeaRoverJobStatusTest LABELS Unit)
