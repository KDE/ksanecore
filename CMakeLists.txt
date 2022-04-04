# SPDX-FileCopyrightText: none
#
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16.0)

# KDE Applications version, managed by release script.
set(RELEASE_SERVICE_VERSION_MAJOR "22")
set(RELEASE_SERVICE_VERSION_MINOR "07")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(KSaneCore VERSION ${RELEASE_SERVICE_VERSION})

include(FeatureSummary)
set(KF5_VERSION "5.90.0")
find_package(ECM ${KF5_VERSION} REQUIRED NO_MODULE)
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
include(GenerateExportHeader)
include(ECMQtDeclareLoggingCategory)
include(KDEGitCommitHooks)
include(KDEClangFormat)

ecm_setup_version(
    ${RELEASE_SERVICE_VERSION}
    VARIABLE_PREFIX KSANECORE
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/ksanecore_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KSaneCoreConfigVersion.cmake"
    SOVERSION 1
)

# Dependencies
set(REQUIRED_QT_VERSION "5.15.2")
# Required Qt5 components to build this framework
find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} NO_MODULE REQUIRED Core Gui)
# Required KF5 frameworks
find_package(KF5I18n ${KF5_VERSION} REQUIRED)

# Check if sane API is available.
find_package(Sane REQUIRED)
message(STATUS "SANE_FOUND:       ${SANE_FOUND}")
message(STATUS "SANE_INCLUDE_DIR: ${SANE_INCLUDE_DIR}")
message(STATUS "SANE_LIBRARY:     ${SANE_LIBRARY}")

add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KSaneCore")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KSaneCoreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KSaneCoreConfig.cmake"
    INSTALL_DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/KSaneCoreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/KSaneCoreConfigVersion.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    COMPONENT Devel
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ksanecore_version.h"
    DESTINATION "${KDE_INSTALL_INCLUDEDIR}/KSaneCore"
    COMPONENT Devel
)

install(EXPORT KSaneCoreTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KSaneCoreTargets.cmake
    NAMESPACE KSane::
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
