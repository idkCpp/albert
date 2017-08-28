cmake_minimum_required(VERSION 2.8.12)

PROJECT(virtualbox)

add_definitions(-DRT_OS_LINUX)
add_definitions(-DVBOX_WITH_XPCOM_NAMESPACE_CLEANUP)

FILE(GLOB_RECURSE SRC include/* src/* metadata.json)

find_package(Qt5 5.2.0 REQUIRED COMPONENTS
    Widgets
    Xml
)

qt5_add_resources(QRC
    ${PROJECT_NAME}.qrc
)

# Define the target
add_library(${PROJECT_NAME} SHARED ${SRC} ${QRC})

# Set INCLUDE_DIRS and INTERFACE_INCLUDE_DIRS
target_include_directories(${PROJECT_NAME}
    PRIVATE
        src/
        include/
        include/nsprpub
        include/string
        include/xpcom
        include/ipcd
)

# Link target to libraries
target_link_libraries(${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    albertcore
    /usr/lib/virtualbox/VBoxXPCOM.so
    xdg
)

# Install target
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib/albert/plugins)
