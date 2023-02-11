cmake_minimum_required(VERSION 3.16...3.21)

project(spout2-texture-filter VERSION 0.0.1)

add_library(${PROJECT_NAME} MODULE)

set(PROJECT_FULL_NAME "Spout2 texture filter")

set(PLUGIN_AUTHOR "mahgu")

target_sources(${PROJECT_NAME} PRIVATE
  inc/SpoutCommon.h
  inc/SpoutSenderNames.h
  inc/SpoutSharedMemory.h
  inc/SpoutUtils.h
  inc/SpoutSenderNames.cpp
  inc/SpoutSharedMemory.cpp
  inc/SpoutUtils.cpp
  spout2-texture-filter.h
  spout2-texture-filter.cpp
)

# Import libobs as main plugin dependency
find_package(libobs REQUIRED)
include(cmake/ObsPluginHelpers.cmake)

find_package(obs-frontend-api REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

if(MSVC)
  target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/deps/w32-pthreads/")
  target_link_libraries(${PROJECT_NAME} PRIVATE OBS::w32-pthreads)
endif()

find_qt(VERSION 6 COMPONENTS Widgets Core)

target_link_libraries(${PROJECT_NAME} PRIVATE
  OBS::libobs
  Qt::Core
  Qt::Widgets)

set_target_properties(${project_name} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# --- Windows-specific build settings and tasks ---
if(OS_WINDOWS)
  configure_file(cmake/bundle/windows/installer-Windows.iss.in
                 ${CMAKE_BINARY_DIR}/installer-Windows.generated.iss)

  configure_file(cmake/bundle/windows/resource.rc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.rc)
  target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.rc)

  if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /WX-)
  endif()
  # --- End of section ---


endif()

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "plugins/${PLUGIN_AUTHOR}")

setup_plugin_target(${PROJECT_NAME})
