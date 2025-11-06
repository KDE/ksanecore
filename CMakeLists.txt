# SPDX-FileCopyrightText: none
#
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16.0)

# KDE Applications version, managed by release script.
set(RELEASE_SERVICE_VERSION_MAJOR "26")
set(RELEASE_SERVICE_VERSION_MINOR "03")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(KSaneCore VERSION ${RELEASE_SERVICE_VERSION})

include(FeatureSummary)

set(REQUIRED_QT_VERSION "6.4.0")
set(KF_MIN_VERSION "6.0.0")
set(KSANECORE_SUFFFIX "6")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")
set(CMAKE_MODULE_PATH ${KSaneCore_SOURCE_DIR}/cmake-modules ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(FeatureSummary)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMInstallIcons)
include(CMakePackageConfigHelpers)
include(CheckFunctionExists)
include(ECMGenerateExportHeader)
include(ECMQtDeclareLoggingCategory)
include(KDEGitCommitHooks)
include(KDEClangFormat)
include(ECMDeprecationSettings)

set(ksanecore_version_header "${CMAKE_CURRENT_BINARY_DIR}/src/ksanecore_version.h")
ecm_setup_version(
    ${RELEASE_SERVICE_VERSION}
    VARIABLE_PREFIX KSANECORE
    VERSION_HEADER "${ksanecore_version_header}"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KSaneCore${KSANECORE_SUFFFIX}ConfigVersion.cmake"
    SOVERSION 1
)

# Dependencies
# Required Qt components to build this framework
find_package(Qt6 ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Core Gui)
# Required KDE Frameworks
find_package(KF6I18n ${KF_MIN_VERSION} REQUIRED)

# Check if sane API is available.
find_package(Sane REQUIRED)
message(STATUS "SANE_FOUND:       ${SANE_FOUND}")
message(STATUS "SANE_INCLUDE_DIR: ${SANE_INCLUDE_DIR}")
message(STATUS "SANE_LIBRARY:     ${SANE_LIBRARY}")

ecm_set_disabled_deprecation_versions(QT 6.4
    KF 5.101
)

add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KSaneCore${KSANECORE_SUFFFIX}")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KSaneCoreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KSaneCore${KSANECORE_SUFFFIX}Config.cmake"
    INSTALL_DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/KSaneCore${KSANECORE_SUFFFIX}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KSaneCore${KSANECORE_SUFFFIX}ConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(FILES
    "${ksanecore_version_header}"
    DESTINATION "${KDE_INSTALL_INCLUDEDIR}/KSaneCore${KSANECORE_SUFFFIX}"
    COMPONENT Devel
)

install(EXPORT KSaneCore${KSANECORE_SUFFFIX}Targets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KSaneCore${KSANECORE_SUFFFIX}Targets.cmake
    NAMESPACE KSane${KSANECORE_SUFFFIX}::
)

ki18n_install(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
