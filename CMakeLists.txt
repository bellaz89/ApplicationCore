cmake_minimum_required(VERSION 3.16)
PROJECT(ChimeraTK-ApplicationCore)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 04)
set(${PROJECT_NAME}_MINOR_VERSION 07)
set(${PROJECT_NAME}_PATCH_VERSION 02)
include(cmake/set_version_numbers.cmake)

option(BUILD_TESTS "Build tests." ON)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 2.10)

if(${pybind11_FOUND})
  find_program(STUBGEN stubgen)

  if(${STUBGEN} STREQUAL "STUBGEN-NOTFOUND")
    message(WARNING "Could not find program 'stubgen'. Python stubs will not be generated. Try installing package 'mypy'.")
  endif()

else()
  message(WARNING "pybind11 was not found in an appropriate version, disabling Python support!")
endif()

# Find the ControlSystemAdapter
FIND_PACKAGE(ChimeraTK-ControlSystemAdapter 02.11 REQUIRED)

# Find the DeviceAccess
FIND_PACKAGE(ChimeraTK-DeviceAccess 03.18 REQUIRED)

# Find the XML parser library libxml++
FIND_PACKAGE(PkgConfig REQUIRED)
set(LIBXML++_VERSION "libxml++-2.6")
PKG_CHECK_MODULES(LibXML++ REQUIRED IMPORTED_TARGET ${LIBXML++_VERSION})
PKG_CHECK_MODULES(glib REQUIRED IMPORTED_TARGET glib-2.0)

# Lua bindings via sol2
option(ENABLE_LUA_BINDINGS "Enable Lua scripting bindings" ON)
set(LUA_IMPL "AUTO" CACHE STRING "Lua implementation to use: AUTO searches LuaJIT first then Lua, LUAJIT forces LuaJIT, LUAC forces standard Lua")
set_property(CACHE LUA_IMPL PROPERTY STRINGS AUTO LUAJIT LUAC)

if(ENABLE_LUA_BINDINGS)
  set(_lua_found FALSE)

  if(LUA_IMPL STREQUAL "AUTO" OR LUA_IMPL STREQUAL "LUAJIT")
    PKG_CHECK_MODULES(LuaJIT REQUIRED IMPORTED_TARGET luajit)
    if(LuaJIT_FOUND)
      set(_lua_found TRUE)
      set(_lua_link_target PkgConfig::LuaJIT)
      message(STATUS "Lua bindings: using LuaJIT ${LuaJIT_VERSION}")
    elseif(LUA_IMPL STREQUAL "LUAJIT")
      message(FATAL_ERROR "LuaJIT was requested (LUA_IMPL=LUAJIT) but not found")
    endif()
  endif()

  if(NOT _lua_found AND (LUA_IMPL STREQUAL "AUTO" OR LUA_IMPL STREQUAL "LUAC"))
    find_package(Lua)
    if(LUA_FOUND)
      set(_lua_found TRUE)
      add_library(_LuaImported UNKNOWN IMPORTED)
      set_target_properties(_LuaImported PROPERTIES
        IMPORTED_LOCATION "${LUA_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}")
      set(_lua_link_target _LuaImported)
      message(STATUS "Lua bindings: using Lua ${LUA_VERSION_STRING}")
    elseif(LUA_IMPL STREQUAL "LUAC")
      message(FATAL_ERROR "Lua was requested (LUA_IMPL=LUAC) but not found")
    else()
      message(WARNING "Neither LuaJIT nor Lua was found, disabling Lua bindings")
    endif()
  endif()

  if(_lua_found)
    include(FetchContent)
    FetchContent_Declare(sol2
      GIT_REPOSITORY https://github.com/ThePhD/sol2.git
      GIT_TAG v3.3.0
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(sol2)
  else()
    set(ENABLE_LUA_BINDINGS OFF CACHE BOOL "Enable Lua scripting bindings" FORCE)
  endif()
endif()

# Find BOOST filesystem
# Note: we need to search this before looking for the unit_test_framework, since we must not link against the
# unit_test_framework library (use header-only)
FIND_PACKAGE(Boost COMPONENTS filesystem date_time chrono system thread REQUIRED)

FIND_PACKAGE(Threads)

include(cmake/set_default_build_to_release.cmake)
include(cmake/set_default_flags.cmake)
include(cmake/enable_code_coverage_report.cmake)
include(cmake/add_linter_target.cmake)

include_directories(${CMAKE_SOURCE_DIR}/include)
file(GLOB headers "${CMAKE_SOURCE_DIR}/include/*.h")
aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)
set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include/)

# add generic modules
include_directories(${CMAKE_SOURCE_DIR}/Modules/include)
file(GLOB module_headers "${CMAKE_SOURCE_DIR}/Modules/include/*.h")
set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/Modules/include/)
aux_source_directory(${CMAKE_SOURCE_DIR}/Modules/src library_module_sources)

if(${pybind11_FOUND})
  # Python module mmanager sources and headers
  include_directories(${CMAKE_SOURCE_DIR}/Python/manager)
  set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/Python/manager/)
  file(GLOB python_manager_headers "${CMAKE_SOURCE_DIR}/Python/manager/*.h")
  set(python_manager_sources "${CMAKE_SOURCE_DIR}/Python/manager/PythonModuleManager.cc")

  # Python bindings sources and headers
  include_directories(${CMAKE_SOURCE_DIR}/Python/bindings/include)
  set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/Python/bindings/include/)
  file(GLOB python_module_headers "${CMAKE_SOURCE_DIR}/Python/bindings/include/*.h")
  file(GLOB python_module_sources "${CMAKE_SOURCE_DIR}/Python/bindings/src/*.cc")
endif()

if(ENABLE_LUA_BINDINGS)
  # Lua module manager sources and headers
  include_directories(${CMAKE_SOURCE_DIR}/Lua/manager)
  set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/Lua/manager/)
  file(GLOB lua_manager_headers "${CMAKE_SOURCE_DIR}/Lua/manager/*.h")
  set(lua_manager_sources "${CMAKE_SOURCE_DIR}/Lua/manager/LuaModuleManager.cc")

  # Lua binding sources and headers (sol2-based wrappers)
  include_directories(${CMAKE_SOURCE_DIR}/Lua/bindings/include)
  file(GLOB lua_bindings_headers "${CMAKE_SOURCE_DIR}/Lua/bindings/include/*.h")
  file(GLOB lua_bindings_sources "${CMAKE_SOURCE_DIR}/Lua/bindings/src/*.cc")
endif()

# Create the executables for automated unit testing.
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
endif()

# C++ library
add_library(${PROJECT_NAME} SHARED ${library_sources} ${library_module_sources} ${python_manager_sources} ${lua_manager_sources} ${lua_bindings_sources} ${headers} ${module_headers} ${python_module_headers} ${python_manager_headers} ${lua_manager_headers} ${lua_bindings_headers})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION}
  SOVERSION ${${PROJECT_NAME}_SOVERSION})

# exported includes are all under ${CMAKE_INSTALL_PREFIX}/include
target_include_directories(${PROJECT_NAME} INTERFACE "$<INSTALL_INTERFACE:include>")
target_link_libraries(${PROJECT_NAME}
  PUBLIC ChimeraTK::ChimeraTK-ControlSystemAdapter
  PUBLIC ${Boost_LIBRARIES}
  PRIVATE Threads::Threads PkgConfig::LibXML++ PkgConfig::glib ${HDF5_LIBRARIES})

if(${pybind11_FOUND})
  target_link_libraries(${PROJECT_NAME} PRIVATE pybind11::embed)
endif()

if(ENABLE_LUA_BINDINGS)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${_lua_link_target} sol2)
  target_compile_definitions(${PROJECT_NAME} PUBLIC CHIMERATK_APPLICATION_CORE_WITH_LUA)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE CHIMERATK_INSIDE_APPLICATION_CORE)

# do not remove runtime path of the library when installing
set_property(TARGET ${PROJECT_NAME} PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

if(${pybind11_FOUND})
  target_compile_definitions(${PROJECT_NAME} PUBLIC CHIMERATK_APPLICATION_CORE_WITH_PYTHON)

  # Python module
  pybind11_add_module(PyApplicationCore ${python_module_sources} ${python_module_headers} ${python_manager_headers})
  target_link_libraries(PyApplicationCore PRIVATE ${PROJECT_NAME})

  # Python module stub
  if((NOT ${CMAKE_BUILD_TYPE} STREQUAL "asan") AND(NOT ${CMAKE_BUILD_TYPE} STREQUAL "tsan") AND(NOT ${STUBGEN} STREQUAL "STUBGEN-NOTFOUND"))
    configure_file(cmake/runStubgen.sh.in ${CMAKE_CURRENT_BINARY_DIR}/runStubgen.sh)
    add_custom_target(PyApplicationCoreInterface ALL
      COMMAND ./runStubgen.sh
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS PyApplicationCore
      BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/PyApplicationCore.pyi
      COMMENT "Use stubgen to create .pyi for statement completion")
  else()
    # stubgen would fail to run due to missing asan/tsan symbols or stubgen not found
    if(${STUBGEN} STREQUAL "STUBGEN-NOTFOUND")
      message(NOTICE "Note: Python stub will not be generated because stubgen was not found.")
    else()
      message(NOTICE "Note: Python stub will not be generated for asan and tsan builds.")
    endif()
  endif()
endif()

# add a target to generate API documentation with Doxygen
include(cmake/enable_doxygen_documentation.cmake)

# enable coding style test
include(cmake/enable_code_style_check.cmake)

# Install the library and the executables
# this defines architecture-dependent ${CMAKE_INSTALL_LIBDIR}
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# all include files go into include/PROJECT_NAME
# The exclusion of ${PROJECT_NAME} prevents the recursive installation of the files just being installed.
# The original headers are in include/*.h, the installed ones in include/PROJECT_NAME/*.h.
install(DIRECTORY ${${PROJECT_NAME}_INCLUDE_DIRS} DESTINATION include/ChimeraTK/ApplicationCore
  FILES_MATCHING PATTERN "*.h"
  PATTERN "${PROJECT_NAME}" EXCLUDE)

if(${pybind11_FOUND})
  # install Python modules to correct platform-dependent directory (if installing to system prefix)
  if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr" OR "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local")
    set(python_install_path "${Python_SITEARCH}")
  else()
    set(python_install_path "lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages")
  endif()

  install(TARGETS PyApplicationCore LIBRARY DESTINATION "${python_install_path}/ChimeraTK/ApplicationCore${${PROJECT_NAME}_SOVERSION}")

  if((NOT ${CMAKE_BUILD_TYPE} STREQUAL "asan") AND(NOT ${CMAKE_BUILD_TYPE} STREQUAL "tsan"))
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/PyApplicationCore.pyi DESTINATION ${python_install_path})
  endif()
endif()

if(ENABLE_LUA_BINDINGS AND _lua_found)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/Lua/stubs/
    DESTINATION share/ChimeraTK/ApplicationCore/lua-stubs
    FILES_MATCHING PATTERN "*.lua")
endif()

# we need the public dependencies so create_cmake_config_files can find them as implicit dependencies
list(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES "Boost COMPONENTS system thread chrono filesystem date_time;ChimeraTK-ControlSystemAdapter")

# we support our cmake EXPORTS as imported targets
set(PROVIDES_EXPORTED_TARGETS 1)
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)
