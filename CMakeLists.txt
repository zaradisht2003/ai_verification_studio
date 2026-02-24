cmake_minimum_required(VERSION 3.16)

# Define the project name and C++ version
project(ai_studio_v2 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure Qt
# Instruct CMake to automatically find moc, uic, and rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt modules
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent)

# Add source files
file(GLOB_RECURSE SRC_FILES "src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "src/*.h")

# Define the target executable
add_executable(${PROJECT_NAME} 
    ${SRC_FILES}
    ${HEADER_FILES}
)

# Link the Qt6 modules to the executable target
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

# On Windows, don't show the console window
if(WIN32 AND NOT MSVC_IDE)
    set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE ON)
endif()

# Deploy Qt dependencies on Windows
if(WIN32)
    # Find windeployqt automatically from the Qt directory
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    # Copy required DLLs next to the executable
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --no-opengl-sw
            --no-translations
            \"$<TARGET_FILE:${PROJECT_NAME}>\"
            COMMENT "Running windeployqt to copy required DLLs..."
        )
    endif()
endif()

