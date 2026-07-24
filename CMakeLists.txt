cmake_minimum_required(VERSION 3.21)

project(usbsync)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(usbsync WIN32 src/main.cpp
                             src/manager.cpp
                             src/sync.cpp
                             src/toast.cpp)

target_compile_definitions(usbsync PRIVATE USBSYNC_GUID="${USBSYNC_GUID}"
                                           USBSYNC_APP_ID="${USBSYNC_APP_ID}")

target_include_directories(usbsync PRIVATE include)

target_link_libraries(usbsync PRIVATE cfgmgr32 runtimeobject)

install(TARGETS usbsync DESTINATION .)
