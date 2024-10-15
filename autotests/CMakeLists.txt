# SPDX-FileCopyrightText: none
#
# SPDX-License-Identifier: BSD-2-Clause

find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

include(ECMMarkAsTest)

macro(ksane_tests)
  foreach(_testname ${ARGN})
    add_executable(${_testname} ${_testname}.cpp)
    target_link_libraries(${_testname} Qt6::Test KSane::Core)
    add_test(ksanecore-${_testname} ${_testname})
    ecm_mark_as_test(${_testname})
  endforeach(_testname)
endmacro()
