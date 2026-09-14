cmake_minimum_required(VERSION 3.16)

project(obsbot-control)
include(GNUInstallDirs)
option(OBSBOT_BUNDLE_RUNTIME "Deploy Qt and dependencies for host execution" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

if(OBSBOT_BUNDLE_RUNTIME)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
endif()

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets OpenGLWidgets)

# SDK paths
set(SDK_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/sdk/include)
set(SDK_LIB_DIR ${CMAKE_SOURCE_DIR}/sdk/lib)

# GUI Application
add_executable(obsbot-gui
    src/gui/main.cpp
    src/gui/MainWindow.cpp
    src/gui/MainWindow.h
    src/gui/CameraController.cpp
    src/gui/CameraController.h
    src/gui/TrackingControlWidget.cpp
    src/gui/TrackingControlWidget.h
    src/gui/PTZControlWidget.cpp
    src/gui/PTZControlWidget.h
    src/gui/CameraSettingsWidget.cpp
    src/gui/CameraSettingsWidget.h
    src/gui/FilterPreviewWidget.cpp
    src/gui/FilterPreviewWidget.h
    src/gui/CameraPreviewWidget.cpp
    src/gui/CameraPreviewWidget.h
    src/gui/VideoEffectsWidget.cpp
    src/gui/VideoEffectsWidget.h
    src/gui/VirtualCameraStreamer.cpp
    src/gui/VirtualCameraStreamer.h
    src/gui/PreviewWindow.cpp
    src/gui/PreviewWindow.h
    src/gui/V4l2MjpegCapture.cpp
    src/gui/V4l2MjpegCapture.h
    src/gui/V4l2Backend.cpp
    src/gui/V4l2Backend.h
    src/gui/XYPad.cpp
    src/gui/XYPad.h
    src/common/Config.cpp
    src/common/Config.h
    resources/resources.qrc
)

target_include_directories(obsbot-gui PRIVATE
    ${SDK_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_directories(obsbot-gui PRIVATE
    ${SDK_LIB_DIR}
)

target_link_libraries(obsbot-gui PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::OpenGLWidgets
    dev
)

target_compile_definitions(obsbot-gui PRIVATE
    $<$<CONFIG:Debug>:OBSBOT_DEBUG_GEOMETRY>
)

# CLI Application
add_executable(obsbot-cli
    src/cli/meet2_test.cpp
    src/common/Config.cpp
    src/common/Config.h
)

target_include_directories(obsbot-cli PRIVATE
    ${SDK_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_directories(obsbot-cli PRIVATE
    ${SDK_LIB_DIR}
)

target_link_libraries(obsbot-cli PRIVATE
    dev
)

# Set RPATH for finding libdev.so
# Build uses the local SDK; installed binaries use their own prefix.
set_target_properties(obsbot-gui PROPERTIES
    BUILD_RPATH "${SDK_LIB_DIR}"
    INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
)

set_target_properties(obsbot-cli PROPERTIES
    BUILD_RPATH "${SDK_LIB_DIR}"
    INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
)

# A normal prefix install includes the SDK and never changes host modules.
install(TARGETS obsbot-gui obsbot-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${SDK_LIB_DIR}/libdev.so.1.0.2 ${SDK_LIB_DIR}/libdev.so.1
    ${SDK_LIB_DIR}/libdev.so DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES obsbot-control.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES resources/icons/camera.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps RENAME obsbot-control.svg)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/obsbot-control)

if(OBSBOT_BUNDLE_RUNTIME)
    if(Qt6_VERSION VERSION_LESS 6.10)
        message(FATAL_ERROR "Runtime bundling requires Qt 6.10 or newer")
    endif()
    # Distro Qt lives under /usr/lib{,64}, excluded by Qt's defaults.
    # Deploy userspace dependencies, but retain the host loader and glibc.
    qt_generate_deploy_script(TARGET obsbot-gui OUTPUT_SCRIPT deploy_script
        CONTENT [=[
set(QT_DEPLOY_LIB_DIR lib)
set(QT_DEPLOY_PLUGINS_DIR plugins)
qt_deploy_runtime_dependencies(
    EXECUTABLE "$<TARGET_FILE:obsbot-gui>"
    GENERATE_QT_CONF
    INCLUDE_PLUGINS qxcb qwayland qwayland-egl qwayland-generic qoffscreen
    EXCLUDE_PLUGIN_TYPES qmltooling
    POST_EXCLUDE_REGEXES
        "/ld-linux[^/]*[.]so.*$"
        "/lib(c|m|pthread|dl|rt|resolv|util|nss_[^/]*)[.]so[.].*$"
)
]=])
    install(SCRIPT ${deploy_script})
endif()

option(OBSBOT_BUILD_HARDWARE_TESTS "Build opt-in camera hardware checks" OFF)
if(OBSBOT_BUILD_HARDWARE_TESTS)
    add_executable(camera-discovery-test tests/camera-discovery.cpp
        src/gui/CameraController.cpp src/gui/CameraController.h
        src/gui/V4l2Backend.cpp src/common/Config.cpp)
    target_include_directories(camera-discovery-test PRIVATE
        ${SDK_INCLUDE_DIR} src/gui src/common)
    target_link_directories(camera-discovery-test PRIVATE ${SDK_LIB_DIR})
    target_link_libraries(camera-discovery-test PRIVATE Qt6::Widgets dev)
    set_target_properties(camera-discovery-test PROPERTIES BUILD_RPATH "${SDK_LIB_DIR}")

    add_executable(mjpeg-capture-test tests/mjpeg-capture.cpp
        src/gui/V4l2MjpegCapture.cpp src/gui/V4l2MjpegCapture.h src/gui/V4l2Backend.cpp)
    target_include_directories(mjpeg-capture-test PRIVATE src/gui)
    target_link_libraries(mjpeg-capture-test PRIVATE Qt6::Gui)

    add_executable(preview-pipeline-test tests/preview-pipeline.cpp
        src/gui/CameraPreviewWidget.cpp src/gui/CameraPreviewWidget.h
        src/gui/FilterPreviewWidget.cpp src/gui/FilterPreviewWidget.h
        src/gui/VirtualCameraStreamer.cpp src/gui/VirtualCameraStreamer.h
        src/gui/V4l2MjpegCapture.cpp src/gui/V4l2MjpegCapture.h)
    target_include_directories(preview-pipeline-test PRIVATE src/gui)
    target_link_libraries(preview-pipeline-test PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::OpenGLWidgets)
endif()
