cmake_minimum_required(VERSION 3.16)
project(pointage VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# QXlsx a besoin des en-têtes privés de QtGui. Le paquet Qt installé par aqtinstall
# ne fournit pas toujours la configuration CMake correspondante : on utilise la nôtre.
set(Qt6GuiPrivate_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/shim")
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network Charts PrintSupport Test GuiPrivate)

include(FetchContent)
set(QXLSX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    QXlsx
    GIT_REPOSITORY https://github.com/QtExcel/QXlsx.git
    GIT_TAG v1.5.1.1
    SOURCE_SUBDIR QXlsx
)
FetchContent_MakeAvailable(QXlsx)

add_executable(pointage
    src/main.cpp
    src/theme.h
    src/theme.cpp
    src/auth.h
    src/auth.cpp
    src/database.h
    src/fields.h
    src/fields.cpp
    src/logindialog.h
    src/logindialog.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/persondialog.h
    src/persondialog.cpp
    src/presencedialog.h
    src/presencedialog.cpp
    src/logger.h
    src/logger.cpp
    src/config.h
    src/supabaseauth.h
    src/supabaseauth.cpp
    src/secretstore.h
    src/secretstore.cpp
    src/cloudstore.h
    src/cloudstore.cpp
    src/exporter.h
    src/exporter.cpp
    src/reports.h
    src/reports.cpp
    src/privacynotice.h
    src/privacynotice.cpp
    src/logo.h
    src/logo.cpp
    src/dashboard.h
    src/dashboard.cpp
    src/excelreport.h
    src/excelreport.cpp
    src/dashboarddialog.h
    src/dashboarddialog.cpp
    src/schedule.h
    src/schedule.cpp
    src/settingsdialog.h
    src/settingsdialog.cpp
    src/schoolclasses.h
    src/schoolclasses.cpp
    src/classboarddialog.h
    src/classboarddialog.cpp
    src/studentreport.h
    src/studentreport.cpp
    src/contractboarddialog.h
    src/contractboarddialog.cpp
    src/photo.h
    src/photo.cpp
    src/appstyle.h
    src/appstyle.cpp
    resources/resources.qrc
)

if(WIN32)
    target_sources(pointage PRIVATE resources/app.rc)
endif()

target_link_libraries(pointage PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network Qt6::Charts Qt6::PrintSupport QXlsx::QXlsx)
if(WIN32)
    target_link_libraries(pointage PRIVATE Crypt32)
endif()
target_compile_definitions(pointage PRIVATE POINTAGE_VERSION="${PROJECT_VERSION}")

enable_testing()
add_executable(tst_rules
    tests/tst_rules.cpp
    src/auth.cpp
    src/database.cpp
    src/fields.cpp
)
target_include_directories(tst_rules PRIVATE src)
target_link_libraries(tst_rules PRIVATE Qt6::Test Qt6::Sql)
add_test(NAME rules COMMAND tst_rules)

add_executable(tst_export
    tests/tst_export.cpp
    src/exporter.cpp
)
target_include_directories(tst_export PRIVATE src)
target_link_libraries(tst_export PRIVATE Qt6::Test Qt6::Network)
add_test(NAME export COMMAND tst_export)

if(WIN32)
    set_target_properties(pointage PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

add_executable(tst_reports
    tests/tst_reports.cpp
    src/reports.cpp
    src/fields.cpp
)
target_include_directories(tst_reports PRIVATE src)
target_link_libraries(tst_reports PRIVATE Qt6::Test Qt6::Network)
add_test(NAME reports COMMAND tst_reports)

add_executable(tst_dashboard
    tests/tst_dashboard.cpp
    src/dashboard.cpp
    src/excelreport.cpp
    src/reports.cpp
    src/fields.cpp
)
target_include_directories(tst_dashboard PRIVATE src)
target_link_libraries(tst_dashboard PRIVATE Qt6::Test Qt6::Network QXlsx::QXlsx)
add_test(NAME dashboard COMMAND tst_dashboard)
