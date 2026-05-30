cmake_minimum_required(VERSION 3.19)
project(Google-Drive-Client LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network NetworkAuth)

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
)

target_compile_definitions(Google-Drive-Client PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

target_link_libraries(Google-Drive-Client
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Network
        Qt6::NetworkAuth
)

include(GNUInstallDirs)

install(TARGETS Google-Drive-Client
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Google-Drive-Client
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
