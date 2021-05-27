cmake_minimum_required(VERSION 3.14)
project(avcp C)

set( CMAKE_C_STANDARD 11 )
set( CMAKE_C_FLAGS "-Wall -Wextra" )

include_directories(.)

add_executable(avcp
    avcp.c avcp.h
    argtable3.c argtable3.h filemediainfo.c filemediainfo.h)

target_link_libraries( avcp m dl avcodec avformat avutil )

set( CMAKE_INSTALL_PREFIX /usr )
install( TARGETS avcp )
install( CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink /usr/bin/avcp /usr/bin/avln )" )
install( CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink /usr/bin/avcp /usr/bin/avls )" )

add_executable(mkln mkln.c)

install(TARGETS mkln RUNTIME
        DESTINATION /usr/bin )
