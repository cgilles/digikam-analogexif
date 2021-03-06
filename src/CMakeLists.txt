#
# Copyright (c) 2020, Gilles Caulier, <caulier dot gilles at gmail dot com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

cmake_minimum_required(VERSION "3.1.0")
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)

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

include_directories($<TARGET_PROPERTY:Digikam::digikamcore,INTERFACE_INCLUDE_DIRECTORIES>/digikam
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(analogexif_generic_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/analogexifgenericplugin.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/analogexif.cpp
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
                            ${CMAKE_CURRENT_SOURCE_DIR}/optgeartemplatemodel.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/progressdialog.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/tagnameitemdelegate.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/tagselectvalsitemdelegate.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/tagtypeitemdelegate.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/asciitextdialog.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/asciistringdialog.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/tagnameeditdialog.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/gearlistmodel.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/geartreemodel.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/exiftreemodel.cpp
                            ${CMAKE_CURRENT_SOURCE_DIR}/emptyspinbox.h
                            ${CMAKE_CURRENT_SOURCE_DIR}/checkedgeartreeview.h
                            ${CMAKE_CURRENT_SOURCE_DIR}/gearlistview.h
                            ${CMAKE_CURRENT_SOURCE_DIR}/geartableview.h
                            ${CMAKE_CURRENT_SOURCE_DIR}/geartreeview.h
                            ${CMAKE_CURRENT_SOURCE_DIR}/optgeartemplateview.h
)

qt5_wrap_ui(analogexif_generic_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/ui/analogexifoptions.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/ui/autofillexpnum.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/ui/copymetadatadialog.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/ui/edittagselectvalues.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/ui/multitagvaluesdialog.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/ui/progressdialog.ui
                                    ${CMAKE_CURRENT_SOURCE_DIR}/ui/tagnameeditdialog.ui
)

if (APPLE)

    qt5_wrap_ui(analogexif_generic_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/ui/analogexif_mac.ui
                                        ${CMAKE_CURRENT_SOURCE_DIR}/ui/editgear_mac.ui
                                        ${CMAKE_CURRENT_SOURCE_DIR}/ui/asciistringdialog_mac.ui
                                        ${CMAKE_CURRENT_SOURCE_DIR}/ui/asciitextdialog_mac.ui
    )

else()

    qt5_wrap_ui(analogexif_generic_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/ui/analogexif.ui
                                        ${CMAKE_CURRENT_SOURCE_DIR}/ui/editgear.ui
                                        ${CMAKE_CURRENT_SOURCE_DIR}/ui/asciistringdialog.ui
                                        ${CMAKE_CURRENT_SOURCE_DIR}/ui/asciitextdialog.ui
    )

endif()

qt5_add_resources(analogexif_generic_QRC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/analogexif.qrc)

add_library(Generic_AnalogExif_Plugin
            MODULE ${analogexif_generic_SRCS}
                   ${analogexif_generic_QRC_SRCS}
)

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
