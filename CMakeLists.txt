cmake_minimum_required(VERSION 3.19)
project(VetProject LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql)

qt_standard_project_setup()

qt_add_executable(VetProject
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    database.h database.cpp
    logindialog.h logindialog.cpp logindialog.ui

)

target_link_libraries(VetProject
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
)

install(TARGETS VetProject
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET VetProject
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})