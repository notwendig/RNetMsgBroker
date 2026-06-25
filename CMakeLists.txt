cmake_minimum_required(VERSION 3.16)

project(RNetMsgBroker VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core LinguistTools)

set(TS_FILES RNetMsgBroker_de_DE.ts)
set(JSON_FILES R-Net.json)

add_library(RNetMsgBroker SHARED
  RNetMsgBroker_global.h
  rnetmsgbroker.cpp
  rnetmsgbroker.h
  ${TS_FILES}
  ${JSON_FILES}
)

target_include_directories(RNetMsgBroker
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(RNetMsgBroker
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(RNetMsgBroker PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_compile_definitions(RNetMsgBroker
  PRIVATE
    RNETMSGBROKER_LIBRARY
    RNETMSGBROKER_VERSION_STRING="${PROJECT_VERSION}"
)

if(COMMAND qt_create_translation)
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

add_executable(RNetMsgBrokerTest
  testapp/main.cpp
)

target_link_libraries(RNetMsgBrokerTest PRIVATE RNetMsgBroker Qt${QT_VERSION_MAJOR}::Core)

add_custom_command(TARGET RNetMsgBrokerTest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_CURRENT_SOURCE_DIR}/R-Net.json
          $<TARGET_FILE_DIR:RNetMsgBrokerTest>/R-Net.json
)
