# SPDX-FileCopyrightText: none
#
# SPDX-License-Identifier: BSD-2-Clause

add_library(KSaneCore)
add_library(KSane::Core ALIAS KSaneCore)

set_target_properties(KSaneCore PROPERTIES
    VERSION     ${KSANECORE_VERSION}
    SOVERSION   ${KSANECORE_SOVERSION}
    EXPORT_NAME "Core"
)

target_compile_definitions(KSaneCore PRIVATE -DTRANSLATION_DOMAIN=\"ksanecore\")

target_sources(KSaneCore PRIVATE
    finddevicesthread.cpp
    scanthread.cpp
    imagebuilder.cpp
    interface.cpp
    interface_p.cpp
    authentication.cpp
    option.cpp
    internaloption.cpp
    deviceinformation.cpp
    options/baseoption.cpp
    options/actionoption.cpp
    options/booloption.cpp
    options/stringoption.cpp
    options/gammaoption.cpp
    options/integeroption.cpp
    options/doubleoption.cpp
    options/listoption.cpp
    options/invertoption.cpp
    options/pagesizeoption.cpp
    options/batchmodeoption.cpp
    options/batchdelayoption.cpp
)

ecm_qt_declare_logging_category(KSaneCore
  HEADER ksanecore_debug.h
  IDENTIFIER KSANECORE_LOG
  CATEGORY_NAME org.kde.ksane.core
)

generate_export_header(KSaneCore BASE_NAME KSaneCore)

target_include_directories(KSaneCore
    INTERFACE
        "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KSaneCore>"
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/options
)

target_link_libraries(KSaneCore
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
    PRIVATE
        Sane::Sane
        KF${KF_MAJOR_VERSION}::I18n
)

ecm_generate_headers(KSaneCore_HEADERS
    HEADER_NAMES
        Interface
        Option
        DeviceInformation
    REQUIRED_HEADERS KSaneCore_HEADERS
    RELATIVE "../src/"
)

# Install files

install(TARGETS KSaneCore
    EXPORT KSaneCoreTargets
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ksanecore_export.h"
    ${KSaneCore_HEADERS}
    DESTINATION "${KDE_INSTALL_INCLUDEDIR}/KSaneCore"
    COMPONENT Devel
)
