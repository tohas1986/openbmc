cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

cmake_policy (SET CMP0054 NEW)

option (YOCTO "Use YOCTO depedencies system" OFF)
include (ExternalProject)
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

project (yandex-ipmi-oem CXX)

add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)
add_definitions (-DBOOST_ASIO_DISABLE_THREADS)
add_definitions (-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)
add_definitions (-Wno-psabi)
add_definitions (-Wno-unused-result)

if (YOCTO)
    find_package (PkgConfig REQUIRED)
    pkg_check_modules (LOGGING phosphor-logging REQUIRED)
    include_directories (${LOGGING_INCLUDE_DIRS})
    link_directories (${LOGGING_LIBRARY_DIRS})

    pkg_check_modules (LIBIPMID libipmid REQUIRED)
    include_directories (${LIBIPMID_INCLUDE_DIRS})
    link_directories (${LIBIPMID_LIBRARY_DIRS})

endif ()

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

#
# import OpenSSL (crypto)
find_package (OpenSSL REQUIRED)
include_directories (${OPENSSL_INCLUDE_DIR})

add_library (
    yndxoemcmds SHARED oemcmd.cpp
)
set_target_properties (yndxoemcmds PROPERTIES VERSION "0.1.0")
set_target_properties (yndxoemcmds PROPERTIES SOVERSION "0")
target_link_libraries (yndxoemcmds stdc++fs)
target_link_libraries (yndxoemcmds ipmid)
target_link_libraries (yndxoemcmds sdbusplus)
target_link_libraries (yndxoemcmds phosphor_logging)
target_link_libraries (yndxoemcmds -luserlayer)
target_link_libraries (yndxoemcmds -lchannellayer)
target_link_libraries (yndxoemcmds gpiodcxx)
target_link_libraries (yndxoemcmds ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries (yndxoemcmds peci)
target_link_libraries (yndxoemcmds i2c)

install (TARGETS yndxoemcmds DESTINATION lib/ipmid-providers)
