# SPDX-FileCopyrightText: none
#
# SPDX-License-Identifier: BSD-2-Clause

add_library(KSaneCore${KSANECORE_SUFFFIX})
add_library(KSane${KSANECORE_SUFFFIX}::Core ALIAS KSaneCore${KSANECORE_SUFFFIX})

set_target_properties(KSaneCore${KSANECORE_SUFFFIX} PROPERTIES
    VERSION     ${KSANECORE_VERSION}
    SOVERSION   ${KSANECORE_SOVERSION}
    EXPORT_NAME "Core"
)

target_compile_definitions(KSaneCore${KSANECORE_SUFFFIX} PRIVATE -DTRANSLATION_DOMAIN=\"ksanecore\")

target_sources(KSaneCore${KSANECORE_SUFFFIX} PRIVATE
    finddevicesthread.cpp finddevicesthread.h
    scanthread.cpp scanthread.h
    imagebuilder.cpp
    interface.cpp interface.h
    interface_p.cpp interface_p.h
    authentication.cpp authentication.h
    option.cpp
    internaloption.cpp internaloption.h
    deviceinformation.cpp deviceinformation.h
    options/baseoption.cpp options/baseoption.h
    options/actionoption.cpp options/actionoption.h
    options/booloption.cpp options/booloption.h
    options/stringoption.cpp options/stringoption.h
    options/gammaoption.cpp options/gammaoption.h
    options/integeroption.cpp options/integeroption.h
    options/doubleoption.cpp options/doubleoption.h
    options/listoption.cpp options/listoption.h
    options/invertoption.cpp options/invertoption.h
    options/pagesizeoption.cpp options/pagesizeoption.h
    options/batchmodeoption.cpp options/batchmodeoption.h
    options/batchdelayoption.cpp options/batchdelayoption.h
)

ecm_qt_declare_logging_category(KSaneCore${KSANECORE_SUFFFIX}
  HEADER ksanecore_debug.h
  IDENTIFIER KSANECORE_LOG
  CATEGORY_NAME org.kde.ksane.core
)

set(_generate_export_header_version_args USE_VERSION_HEADER)

ecm_generate_export_header(KSaneCore${KSANECORE_SUFFFIX}
    BASE_NAME KSaneCore
    VERSION ${KSANECORE_VERSION}
    DEPRECATED_BASE_VERSION 0
    ${_generate_export_header_version_args}
)

target_include_directories(KSaneCore${KSANECORE_SUFFFIX}
    INTERFACE
        "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KSaneCore${KSANECORE_SUFFFIX}>"
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/options
)

target_link_libraries(KSaneCore${KSANECORE_SUFFFIX}
    PUBLIC
        Qt6::Core
        Qt6::Gui
    PRIVATE
        Sane::Sane
        KF6::I18n
)

ecm_generate_headers(KSaneCore_CamelCase_HEADERS
    HEADER_NAMES
        Interface
        Option
        DeviceInformation
    REQUIRED_HEADERS KSaneCore_HEADERS
    PREFIX KSaneCore
    RELATIVE "../src/"
)

# Install files

install(TARGETS KSaneCore${KSANECORE_SUFFFIX}
    EXPORT KSaneCore${KSANECORE_SUFFFIX}Targets
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ksanecore_export.h"
    DESTINATION "${KDE_INSTALL_INCLUDEDIR}/KSaneCore${KSANECORE_SUFFFIX}"
    COMPONENT Devel
)

install(FILES
    ${KSaneCore_HEADERS}
    DESTINATION "${KDE_INSTALL_INCLUDEDIR}/KSaneCore${KSANECORE_SUFFFIX}/ksanecore"
    COMPONENT Devel
)

install(FILES
    ${KSaneCore_CamelCase_HEADERS}
    DESTINATION "${KDE_INSTALL_INCLUDEDIR}/KSaneCore${KSANECORE_SUFFFIX}/KSaneCore"
    COMPONENT Devel
)
