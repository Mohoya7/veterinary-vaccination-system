cmake_minimum_required(VERSION 3.19)
project(VetProject LANGUAGES CXX)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql Svg)
qt_standard_project_setup()
qt_add_executable(VetProject
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    database.h database.cpp
    logindialog.h logindialog.cpp logindialog.ui
    dashboardwidget.h dashboardwidget.cpp dashboardwidget.ui
    sidebarwidget.h sidebarwidget.cpp
    ownerswidget.h ownerswidget.cpp ownerswidget.ui
    addownerdialog.h addownerdialog.cpp addownerdialog.ui
    addanimaldialog.h addanimaldialog.cpp addanimaldialog.ui
    addvaccinedialog.h addvaccinedialog.cpp addvaccinedialog.ui
    vaccinationswidget.h vaccinationswidget.cpp vaccinationswidget.ui
    styledmessagebox.h styledmessagebox.cpp
    icons/chevron-down.svg
    reminderswidget.h reminderswidget.cpp reminderswidget.ui
    animalswidget.h animalswidget.cpp animalswidget.ui
    persiandate.h
    animaltypeinfo.h
    persiandatepicker.h persiandatepicker.cpp
    session.h
    pagedirtytracker.h

    userstab.h userstab.cpp
    animaltypestab.h animaltypestab.cpp
    backuptab.h backuptab.cpp
    abouttab.h abouttab.cpp
    app.rc


)
qt_add_resources(VetProject "resources"
    PREFIX "/"
    FILES
        icons/bell.svg
        icons/dog.svg
        icons/grid.svg
        icons/log-out.svg
        icons/settings.svg
        icons/syringe.svg
        icons/users.svg
        icons/chevron-down.svg
        icons/app.png
        icons/login_logo.png
        icons/eye.svg
        icons/eye-off.svg
        icons/background.png
        icons/man-profile.png
        icons/woman-profile.png
        icons/plus.svg
        icons/edit-2.svg
        icons/trash-2.svg


)
target_include_directories(VetProject PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(VetProject
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        Qt6::Svg
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