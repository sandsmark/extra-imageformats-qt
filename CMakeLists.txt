cmake_minimum_required(VERSION 3.0)
project(extra-imageformats-qt)

include(GNUInstallDirs)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(BPG)

find_package(Qt5 REQUIRED COMPONENTS Gui)

include(QueryQmake)
query_qmake(QT_INSTALL_PLUGINS)

find_package(PkgConfig REQUIRED)
pkg_search_module(libpgf REQUIRED IMPORTED_TARGET libpgf)

# Add plugins

add_library(bpg MODULE bpg/bpghandler.cpp)
target_include_directories(bpg PRIVATE bpg)
target_link_libraries(bpg Qt::Gui BPG::BPG)

add_library(pgf MODULE pgf/pgfhandler.cpp)
target_include_directories(pgf PRIVATE pgf)
target_link_libraries(pgf Qt::Gui PkgConfig::libpgf)

## And fuif, for historical reasons

add_library(fuif MODULE
    fuif/fuifhandler.cpp

    # libfuif is dead, so bundled here
    fuif/libfuif/image/image.cpp
    fuif/libfuif/transform/transform.cpp
    fuif/libfuif/maniac/bit.cpp
    fuif/libfuif/maniac/chance.cpp
    fuif/libfuif/encoding/encoding.cpp
    fuif/libfuif/io.cpp
    )
target_include_directories(fuif PRIVATE fuif fuif/libfuif)
target_link_libraries(fuif Qt::Gui)

install(TARGETS bpg pgf fuif DESTINATION ${QT_INSTALL_PLUGINS}/imageformats/)

# Mime types
install(FILES
    bpg/xdg-bpg.xml
    fuif/xdg-fuif.xml
    pgf/xdg-pgf.xml

    DESTINATION
    ${DATAROOTDIR}/mime/image/
    )

# kservicetrader files
install(FILES
    bpg/bpg.desktop
    fuif/fuif.desktop
    pgf/pgf.desktop

    DESTINATION
    # Should probably not hardcode this. So sue me.
    ${DATAROOTDIR}/kservices5/qimageioplugins/
    )
