# SPDX-FileCopyrightText: none
#
# SPDX-License-Identifier: BSD-2-Clause

add_library(KF5SaneCore)
add_library(KF5::SaneCore ALIAS KF5SaneCore)

target_compile_definitions(KF5SaneCore PRIVATE -DTRANSLATION_DOMAIN=\"ksanecore\")

target_sources(KF5SaneCore PRIVATE
    ksanefinddevicesthread.cpp
    ksanescanthread.cpp
    ksaneimagebuilder.cpp
    ksanecore.cpp
    ksanecore_p.cpp
    ksaneauth.cpp
    ksaneoption.cpp
    ksaneinternaloption.cpp
    options/ksanebaseoption.cpp
    options/ksaneactionoption.cpp
    options/ksanebooloption.cpp
    options/ksanestringoption.cpp
    options/ksanegammaoption.cpp
    options/ksaneintegeroption.cpp
    options/ksanedoubleoption.cpp
    options/ksanelistoption.cpp
    options/ksaneinvertoption.cpp
    options/ksanepagesizeoption.cpp
    options/ksanebatchmodeoption.cpp
    options/ksanebatchdelayoption.cpp
)

ecm_qt_declare_logging_category(KF5SaneCore
  HEADER ksanecore_debug.h
  IDENTIFIER KSANECORE_LOG
  CATEGORY_NAME org.kde.ksane.core
)

generate_export_header(KF5SaneCore BASE_NAME KSaneCore)

target_include_directories(KF5SaneCore
    INTERFACE
        "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF}/KSaneCore>"
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/options
)

target_link_libraries(KF5SaneCore
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
    PRIVATE
        Sane::Sane
        KF5::I18n
)

set_target_properties(KF5SaneCore
    PROPERTIES VERSION ${KSANECORE_VERSION}
    SOVERSION ${KSANECORE_SOVERSION}
    EXPORT_NAME "SaneCore"
)


ecm_generate_headers(KSaneCore_HEADERS
    HEADER_NAMES
        KSaneCore
        KSaneOption
    REQUIRED_HEADERS KSaneCore_HEADERS
    RELATIVE "../src/"
)

# Install files

install(TARGETS KF5SaneCore
    EXPORT KF5SaneCoreTargets
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ksanecore_export.h
    ${KSaneCore_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/KSaneCore
    COMPONENT Devel
)
