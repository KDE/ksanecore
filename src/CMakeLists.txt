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

ecm_qt_declare_logging_category(KSaneCore${KSANECORE_SUFFFIX}
  HEADER ksanecore_debug.h
  IDENTIFIER KSANECORE_LOG
  CATEGORY_NAME org.kde.ksane.core
)

if (QT_MAJOR_VERSION STREQUAL "5")
    set(_generate_export_header_version_args)
else()
    # For Qt6/KF6 world transitively include the version header
    if(ECM_VERSION VERSION_LESS "5.106")
        set(include_version_header_code "#include <ksanecore_version.h>\n")
        set(_generate_export_header_version_args CUSTOM_CONTENT_FROM_VARIABLE include_version_header_code)
    else()
        set(_generate_export_header_version_args USE_VERSION_HEADER)
    endif()
endif()

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
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
    PRIVATE
        Sane::Sane
        KF${KF_MAJOR_VERSION}::I18n
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
