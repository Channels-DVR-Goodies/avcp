cmake_minimum_required(VERSION 3.14)
project(avcp C)

set( CMAKE_C_STANDARD 11 )
set( CMAKE_C_FLAGS "-Wall -Wextra" )

include_directories(.)

add_executable(avcp
    avcp.c avcp.h
    argtable3.c argtable3.h)

target_link_libraries( avcp m dl mediainfo )

set( CMAKE_INSTALL_PREFIX /usr )
install( TARGETS avcp )