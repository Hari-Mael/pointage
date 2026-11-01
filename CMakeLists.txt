cmake_minimum_required(VERSION 3.16)
project(pointage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql PrintSupport)

add_executable(pointage
    src/main.cpp
    src/auth.h
    src/auth.cpp
    src/database.h
    src/database.cpp
    src/fields.h
    src/fields.cpp
    src/logindialog.h
    src/logindialog.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/persondialog.h
    src/persondialog.cpp
    src/restdaysdialog.h
    src/restdaysdialog.cpp
    src/presenceprinter.h
    src/presenceprinter.cpp
)

target_link_libraries(pointage PRIVATE Qt6::Widgets Qt6::Sql Qt6::PrintSupport)

if(WIN32)
    set_target_properties(pointage PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
