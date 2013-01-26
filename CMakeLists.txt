project(traypost)

OPTION(WITH_QT5 "Qt5 support" OFF)

if (WITH_QT5)
    cmake_minimum_required(VERSION 2.8.8)
else()
    cmake_minimum_required(VERSION 2.8.6)
endif()

set(CMAKE_AUTOMOC TRUE)

# Qt modules
if (WITH_QT5)
    find_package(Qt5Widgets REQUIRED)
    set(traypost_Qt5_Modules Widgets)

    set(USE_QXT FALSE)
else()
    find_package(Qt4 REQUIRED)
    set(QT_USE_QTNETWORK TRUE)
    set(QT_USE_QTSVG TRUE)
    set(QT_USE_QTXML TRUE)
    set(QT_USE_QTSCRIPT TRUE)

    include(${QT_USE_FILE})
    set(USE_QXT TRUE)
endif()

# Project files
file(GLOB traypost_SOURCES *.cpp)

file(GLOB traypost_FORMS *.ui)

file(GLOB traypost_TRANSLATIONS translations/*.ts)

# Qt include paths and definitions
if (WITH_QT5)
    include_directories(${Qt5Widgets_INCLUDES} ${CMAKE_BINARY_DIR} src/include)
    add_definitions(${Qt5Widgets_DEFINITIONS})
else()
    include_directories(${QT_INCLUDES} ${CMAKE_BINARY_DIR} src/include)
    add_definitions(${QT_DEFINITIONS})
endif()

# C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

# Be more strict while compiling debugging version
if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wextra -Wall -pedantic")
endif()

set(traypost_LIBRARIES ${traypost_LIBRARIES} ${X11_LIBRARIES} ${X11_Xfixes_LIB})

if (WITH_QT5)
    qt5_wrap_ui(traypost_FORMS_HEADERS ${traypost_FORMS})
    find_package(Qt5LinguistTools)
    if (Qt5LinguistTools_FOUND)
        qt5_add_translation(traypost_QM ${traypost_TRANSLATIONS})
    endif()
else()
    qt4_wrap_ui(traypost_FORMS_HEADERS ${traypost_FORMS})
    qt4_add_translation(traypost_QM ${traypost_TRANSLATIONS})
endif()

# Generate a qrc file for the translations
set(traypost_QM_RCC ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
file(WRITE ${traypost_QM_RCC} "<RCC><qresource prefix=\"/translations\">")
foreach(QM_FILE ${traypost_QM})
    file(RELATIVE_PATH traypost_RELATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR} ${QM_FILE})
    file(APPEND ${traypost_QM_RCC} "<file>" ${traypost_RELATIVE_PATH} "</file>")
endforeach(QM_FILE)
file(APPEND ${traypost_QM_RCC} "</qresource></RCC>")
if (WITH_QT5)
    qt5_add_resources(traypost_RESOURCES_RCC ${traypost_QM_RCC})
else()
    qt4_add_resources(traypost_RESOURCES_RCC ${traypost_QM_RCC})
endif()

add_executable(traypost ${traypost_SOURCES}
    ${traypost_FORMS_HEADERS}
    ${traypost_RESOURCES_RCC}
    ${traypost_RC}
    ${traypost_QM}
    )

if (WITH_QT5)
    qt5_use_modules(traypost ${traypost_Qt5_Modules})
endif()

target_link_libraries(traypost ${QT_LIBRARIES} ${traypost_LIBRARIES})

install(TARGETS traypost DESTINATION bin)

