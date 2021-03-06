# Copyright: (C) 2012-2015 POETICON++, European Commission FP7 project ICT-288382
# CopyPolicy: Released under the terms of the GNU GPL v2.0.

cmake_minimum_required(VERSION 2.6)

set(APPNAME poeticon)

file(GLOB conf ${CMAKE_CURRENT_SOURCE_DIR}/conf/*.ini)
file(GLOB scripts ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.template)

file(GLOB prada ${CMAKE_CURRENT_SOURCE_DIR}/conf/planner.exe)
file(GLOB pradaFiles ${CMAKE_CURRENT_SOURCE_DIR}/conf/config ${CMAKE_CURRENT_SOURCE_DIR}/conf/*.dat)

find_package(YARP)
find_package(ICUB)
find_package(ICUBcontrib)
list(APPEND CMAKE_MODULE_PATH ${YARP_MODULE_PATH} ${ICUB_MODULE_PATH} ${ICUBCONTRIB_MODULE_PATH})
include(YarpInstallationHelpers)
include(ICUBcontribHelpers)
icubcontrib_set_default_prefix()

yarp_install(FILES ${conf} DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${APPNAME})
yarp_install(FILES ${scripts} DESTINATION ${ICUBCONTRIB_APPLICATIONS_TEMPLATES_INSTALL_DIR})

yarp_install(PROGRAMS ${prada} DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${APPNAME})
yarp_install(FILES ${pradaFiles} DESTINATION ${ICUBCONTRIB_CONTEXTS_INSTALL_DIR}/${APPNAME})
