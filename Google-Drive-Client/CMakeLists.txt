cmake_minimum_required(VERSION 3.19)
project(Google-Drive-Client LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Widgets Network NetworkAuth)
find_package(QuaZip-Qt6 REQUIRED)

qt_standard_project_setup()

qt_add_executable(Google-Drive-Client
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    setup.h setup.cpp setup.ui
    env.h env.cpp
    global.h
    global.cpp
    driveitem.h driveitem.cpp
    settings.h settings.cpp settings.ui
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources.cpp
    COMMAND Qt6::rcc --name resources --output ${CMAKE_CURRENT_BINARY_DIR}/resources.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc download.png trash.png
            download_catpp.png trash_catpp.png
    VERBATIM
)
target_sources(Google-Drive-Client PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources.cpp)

target_link_libraries(Google-Drive-Client
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Network
        Qt6::NetworkAuth
        QuaZip::QuaZip
)

include(GNUInstallDirs)

install(TARGETS Google-Drive-Client
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(Qt6Core_VERSION VERSION_GREATER_EQUAL "6.5")
    qt_generate_deploy_app_script(
        TARGET Google-Drive-Client
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()
