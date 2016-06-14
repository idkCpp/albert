cmake_minimum_required(VERSION 2.8.11)

project(autoconf)

# Get Qt libraries
find_package(Qt5Core 5.2 REQUIRED)
find_package(Qt5Widgets 5.2 REQUIRED)


include_directories(
    ../../application/pluginsystem
)

# List files in the source directory
FILE(GLOB SRC *.cpp *.h *.hpp)

# Add the target as static lib
add_library(${PROJECT_NAME} STATIC ${SRC})

# Link qt core library
target_link_libraries(${PROJECT_NAME} ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES})

# Propagade headers to users of the target
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
 
