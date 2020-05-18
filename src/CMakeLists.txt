#
# Copyright (c) 2020, Gilles Caulier, <caulier dot gilles at gmail dot com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

cmake_minimum_required(VERSION "3.1.0")
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)
include(FeatureSummary)

find_package(Qt5 "5.6.0" REQUIRED
             NO_MODULE COMPONENTS
             Core
             Widgets
             Gui
             Sql
             Network
             Concurrent
)

find_package(EXPAT REQUIRED)
find_package(exiv2 REQUIRED)

set_package_properties("exiv2"     PROPERTIES
                       DESCRIPTION "Required to build digiKam"
                       URL         "https://www.exiv2.org"
                       TYPE        RECOMMENDED
                       PURPOSE     "Library to manage image metadata"
)

find_package(DigikamCore
             CONFIG REQUIRED)

set_package_properties(DigikamCore PROPERTIES
                       URL "http://www.digikam.org"
                       DESCRIPTION "digiKam core library"
)

include_directories($<TARGET_PROPERTY:Digikam::digikamcore,INTERFACE_INCLUDE_DIRECTORIES>/digikam)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(analogexif_generic_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/analogexifgenericplugin.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/analogexif.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/aboutdialog.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/analogexifoptions.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/autofillexpnum.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/copymetadatadialog.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/dirsortfilterproxymodel.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/editgear.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/editgeartagsmodel.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/editgeartreemodel.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/edittagselectvalues.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/exifitem.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/exifitemdelegate.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/exifutils.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/metadatatagcompleter.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/multitagvaluesdialog.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/onlineversionchecker.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/optgeartemplatemodel.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/progressdialog.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/tagnameitemdelegate.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/tagselectvalsitemdelegate.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/tagtypeitemdelegate.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/asciitextdialog.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/asciistringdialog.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/tagnameeditdialog.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/exiftreemodel.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/gearlistmodel.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/geartreemodel.cpp
)

qt5_wrap_ui(analogexif_generic_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/analogexifoptions.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/autofillexpnum.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/copymetadatadialog.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/edittagselectvalues.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/multitagvaluesdialog.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/progressdialog.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/tagnameeditdialog.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/aboutdialog.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/analogexif.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/editgear.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/asciistringdialog.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/asciitextdialog.ui
)

add_library(Generic_AnalogExif_Plugin
            MODULE ${analogexif_generic_SRCS})

set_target_properties(Generic_AnalogExif_Plugin PROPERTIES PREFIX "")

target_link_libraries(Generic_AnalogExif_Plugin
                      Digikam::digikamcore

                      Qt5::Core
                      Qt5::Widgets
                      Qt5::Gui
                      Qt5::Sql
                      Qt5::Network
                      Qt5::Concurrent
)

MACRO_ADD_PLUGIN_INSTALL_TARGET(Generic_AnalogExif_Plugin generic)

# ---

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
