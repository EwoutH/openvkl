## ======================================================================== ##
## Copyright 2019 Intel Corporation                                         ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

set(COMPONENT_NAME ospcommon)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

set(ENV{OSPCOMMON_TBB_ROOT} ${TBB_PATH})

ExternalProject_Add(${COMPONENT_NAME}
  PREFIX ${COMPONENT_NAME}
  DOWNLOAD_DIR ${COMPONENT_NAME}
  STAMP_DIR ${COMPONENT_NAME}/stamp
  SOURCE_DIR ${COMPONENT_NAME}/src
  BINARY_DIR ${COMPONENT_NAME}/build
  URL "https://github.com/ospray/ospcommon/archive/v${BUILD_OSPCOMMON_VERSION}.zip"
  CMAKE_ARGS
    -DBUILD_TESTING=OFF
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
    -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
    -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
    -DCMAKE_INSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}
    -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
    -DCMAKE_BUILD_TYPE=Release
    $<$<BOOL:${BUILD_TBB_FROM_SOURCE}>:-DOSPCOMMON_TBB_ROOT=${TBB_PATH}>
  BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
  BUILD_ALWAYS OFF
)

set(OSPCOMMON_PATH "${COMPONENT_PATH}/${CMAKE_INSTALL_LIBDIR}/cmake/ospcommon-${BUILD_OSPCOMMON_VERSION}")

if (BUILD_TBB_FROM_SOURCE)
  ExternalProject_Add_StepDependencies(${COMPONENT_NAME} configure tbb)
endif()
