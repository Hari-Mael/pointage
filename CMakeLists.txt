cmake_minimum_required(VERSION 3.16)
project(pointage VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network Test)

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
)

target_link_libraries(pointage PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network)
if(WIN32)
    target_link_libraries(pointage PRIVATE Crypt32)
endif()
target_compile_definitions(pointage PRIVATE POINTAGE_VERSION="${PROJECT_VERSION}")

if(WIN32)
    set_target_properties(pointage PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

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

add_executable(tst_reports
    tests/tst_reports.cpp
    src/reports.cpp
    src/fields.cpp
)
target_include_directories(tst_reports PRIVATE src)
target_link_libraries(tst_reports PRIVATE Qt6::Test Qt6::Network)
add_test(NAME reports COMMAND tst_reports)
