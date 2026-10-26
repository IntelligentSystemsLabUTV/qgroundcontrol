# MEDEA rover: generic MAVLink rover reached through the ROS 2 <-> MAVLink bridge
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MedeaRoverFirmwarePlugin.cc
        MedeaRoverFirmwarePlugin.h
        MedeaRoverFirmwarePluginFactory.cc
        MedeaRoverFirmwarePluginFactory.h
        MedeaRoverJobStatus.cc
        MedeaRoverJobStatus.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
