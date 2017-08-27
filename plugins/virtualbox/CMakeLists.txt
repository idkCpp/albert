cmake_minimum_required(VERSION 2.8.12)

PROJECT(virtualbox)

FILE(GLOB_RECURSE SRC src/* metadata.json)

find_package(Qt5 5.2.0 REQUIRED COMPONENTS
    Widgets
    Xml
)

qt5_add_resources(QRC
    ${PROJECT_NAME}.qrc
)

add_definitions(-DRT_OS_LINUX)

include_directories(
    src/sdk/
    src/sdk/idl
    src/sdk/include
    src/sdk/include/ipcd
    src/sdk/include/nsprpub
    src/sdk/include/string
    src/sdk/include/xpcom
)

# Define the target
add_library(${PROJECT_NAME} SHARED ${SRC} ${QRC})

# Set INCLUDE_DIRS and INTERFACE_INCLUDE_DIRS
target_include_directories(${PROJECT_NAME} PRIVATE src/)

# Link target to libraries
target_link_libraries(${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    albertcore
    xdg
    ${CMAKE_SOURCE_DIR}/plugins/virtualbox/src/sdk/VBoxXPCOM.so
)

# Install target
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/albert/plugins)
