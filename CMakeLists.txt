cmake_minimum_required(VERSION 3.5)
project(AIPS VERSION "1.2.319")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

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
message(STATUS "XLNT_DIR: ${XLNT_DIR}")
set(CMAKE_PREFIX_PATH ${XLNT_DIR} ${CMAKE_PREFIX_PATH})
pkg_check_modules(XLNT REQUIRED xlnt)
message(STATUS "XLNT_LIBRARY_DIRS: ${XLNT_LIBRARY_DIRS}")
message(STATUS "XLNT_INCLUDEDIR: ${XLNT_INCLUDEDIR}")
message(STATUS "XLNT_LIBRARIES: ${XLNT_LIBRARIES}")
message(STATUS "XLNT_INCLUDE_DIRS: ${XLNT_INCLUDE_DIRS}")

include_directories(${XLNT_INCLUDEDIR})
link_directories(${XLNT_LIBRARY_DIRS})

#-------------------------------------------------------------------------------
message(STATUS "JSON_DIR: ${JSON_DIR}")
set(CMAKE_PREFIX_PATH ${JSON_DIR} ${CMAKE_PREFIX_PATH})
find_package(nlohmann_json 3.2.0 REQUIRED)

#-------------------------------------------------------------------------------
include_directories("${CMAKE_SOURCE_DIR}")

#-------------------------------------------------------------------------------
source_group(barcode FILES src/c2s/ean13/functii.cpp src/c2s/ean13/functii.h)

add_executable(cpp2sqlite
	src/c2s/cpp2sqlite.cpp
	src/c2s/aips.hpp src/c2s/aips.cpp
	src/c2s/refdata.hpp src/c2s/refdata.cpp
	src/c2s/swissmedic.hpp src/c2s/swissmedic.cpp
	src/c2s/sappinfo.hpp src/c2s/sappinfo.cpp
	src/bag.hpp src/bag.cpp
	src/gtin.hpp src/gtin.cpp
	src/beautify.hpp src/beautify.cpp
	src/sqlDatabase.hpp src/sqlDatabase.cpp
	src/c2s/atc.hpp src/c2s/atc.cpp
	src/c2s/epha.hpp src/c2s/epha.cpp
	src/c2s/peddose.hpp src/c2s/peddose.cpp
	src/c2s/drugshortage.hpp src/c2s/drugshortage.cpp
    src/report.hpp src/report.cpp
	src/c2s/ean13/functii.cpp src/c2s/ean13/functii.h
	src/c2s/medicine.h
	src/c2s/html_tags.h)

target_include_directories(cpp2sqlite PUBLIC
	"${CMAKE_SOURCE_DIR}/src"
	"${CMAKE_SOURCE_DIR}/src/c2s")
target_link_libraries(cpp2sqlite ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES} ${XLNT_LIBRARIES})
#set_target_properties(cpp2sqlite PROPERTIES CXX_STANDARD 17)

#-------------------------------------------------------------------------------
add_executable(interaction
	src/int/lang/en.h
	src/int/lang/de.h
	src/int/lang/fr.h
	src/int/atc.hpp src/int/atc.cpp
	src/int/main.cpp)

target_include_directories(interaction PUBLIC "${CMAKE_SOURCE_DIR}/src/int")
target_link_libraries(interaction ${Boost_LIBRARIES})

#-------------------------------------------------------------------------------
add_executable(sappinfo
	src/sap/main.cpp)

target_include_directories(sappinfo PUBLIC "${CMAKE_SOURCE_DIR}/src/sap")
target_link_libraries(sappinfo ${Boost_LIBRARIES} ${XLNT_LIBRARIES})

#-------------------------------------------------------------------------------
add_executable(pharma
	src/gtin.hpp src/gtin.cpp
	src/bag.hpp src/bag.cpp
	src/report.hpp src/report.cpp
	src/beautify.hpp src/beautify.cpp
	src/pha/refdata.hpp src/pha/refdata.cpp
	src/pha/swissmedic1.hpp src/pha/swissmedic1.cpp
	src/pha/swissmedic2.hpp src/pha/swissmedic2.cpp
    src/pha/ddd.hpp src/pha/ddd.cpp
    src/pha/aips.hpp src/pha/aips.cpp
	src/pha/main.cpp)

target_include_directories(pharma PUBLIC
			"${CMAKE_SOURCE_DIR}/src"
			"${CMAKE_SOURCE_DIR}/src/pha")
target_link_libraries(pharma ${Boost_LIBRARIES} ${XLNT_LIBRARIES})

#-------------------------------------------------------------------------------
add_executable(zurrose
	src/zur/main.cpp
	src/gtin.hpp src/gtin.cpp
	src/report.hpp src/report.cpp
	src/sqlDatabase.hpp src/sqlDatabase.cpp
	src/zur/Article.hpp src/zur/Article.cpp
	src/zur/atc.hpp src/zur/atc.cpp
	src/zur/bag.hpp src/zur/bag.cpp
	src/zur/direkt.hpp src/zur/direkt.cpp
	src/zur/galen.hpp src/zur/galen.cpp
	src/zur/generika.hpp src/zur/generika.cpp
	src/zur/neu.hpp src/zur/neu.cpp
	src/zur/nota.hpp src/zur/nota.cpp
	src/zur/stamm.hpp src/zur/stamm.cpp
	src/zur/gripp.hpp src/zur/gripp.cpp
	src/zur/swissmedic.hpp src/zur/swissmedic.cpp
	src/zur/voll.hpp src/zur/voll.cpp)

target_include_directories(zurrose PUBLIC
			"${CMAKE_SOURCE_DIR}/src"
			"${CMAKE_SOURCE_DIR}/src/zur")
target_link_libraries(zurrose PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(zurrose PUBLIC ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES} ${XLNT_LIBRARIES})

#-------------------------------------------------------------------------------

install(TARGETS cpp2sqlite interaction sappinfo pharma RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
