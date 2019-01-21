cmake_minimum_required(VERSION 3.5)
project(cpp2sqlite)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED COMPONENTS log program_options)
include_directories(${Boost_INCLUDE_DIRS})

#-------------------------------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
message(STATUS "SQLITE3_INCLUDEDIR: ${SQLITE3_INCLUDEDIR}")
message(STATUS "SQLITE3_LIBRARIES: ${SQLITE3_LIBRARIES}")

include_directories(${SQLITE3_INCLUDEDIR})

#-------------------------------------------------------------------------------
include_directories("${CMAKE_SOURCE_DIR}")

#-------------------------------------------------------------------------------
add_executable(cpp2sqlite
	cpp2sqlite.cpp
	sqlDatabase.hpp sqlDatabase.cpp
	aips.hpp aips.cpp
	refdata.hpp refdata.cpp
	medicine.h)

target_link_libraries(cpp2sqlite ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES})
#set_target_properties(cpp2sqlite PROPERTIES CXX_STANDARD 17)

#-------------------------------------------------------------------------------

install(TARGETS cpp2sqlite RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
