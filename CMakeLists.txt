cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(pml_dnssd LANGUAGES CXX VERSION 1.2.1)
include(FetchContent)

message(STATUS "----pml/dnssd ----")

execute_process(COMMAND ${CMAKE_COMMAND} -DNAMESPACE=dnssd -DMAJOR=${PROJECT_VERSION_MAJOR} -DMINOR=${PROJECT_VERSION_MINOR} -DPATCH=${PROJECT_VERSION_PATCH} -DOUTPUT=${CMAKE_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake)

SET(DIR_BASE ${CMAKE_SOURCE_DIR} CACHE STRING "base location for libraries")
SET(DIR_LOG ${DIR_BASE}/external/log CACHE STRING "location of pml log")

FetchContent_Declare(log GIT_REPOSITORY "https://github.com/martim01/log.git"  SOURCE_DIR ${DIR_LOG})

FetchContent_GetProperties(log)
if(NOT log_POPULATED)
	FetchContent_MakeAvailable(log)
endif()

list(APPEND DNS_SOURCES "src/dnssd.cpp"  "src/zcposter.cpp")

#linux specific
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	#use pkgconfig to find the required libs
	find_package(PkgConfig REQUIRED QUIET)
	pkg_search_module(avahi REQUIRED avahi-client IMPORTED_TARGET)
	if(TARGET PkgConfig::avahi-client)
		message(STATUS "Found avahi-client")
	endif()

	#add the os specific code
	SET(OS_SOURCES "../src/gnu/avahibrowser.cpp" "../src/gnu/avahipublisher.cpp")
	
	list(APPEND DNS_SOURCES "src/gnu/avahibrowser.cpp" "src/gnu/avahipublisher.cpp")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	#add the os specific code
	list(APPEND DNS_SOURCES "src/windows/bonjourbrowser.cpp" "src/windows/bonjourpublisher.cpp")
endif()


add_library(pml_dnssd SHARED ${DNS_SOURCES} ${CMAKE_BINARY_DIR}/src/dnssd_version.cpp)
set_target_properties(pml_dnssd PROPERTIES DEBUG_POSTFIX "d")

target_include_directories(pml_dnssd PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(pml_dnssd PUBLIC ${DIR_LOG}/include)
target_include_directories(pml_dnssd PUBLIC ${CMAKE_BINARY_DIR}/include)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_dependencies(pml_dnssd pml_log)
	target_include_directories(pml_dnssd PRIVATE "mdnssd")
	target_link_directories(pml_dnssd PRIVATE "bin")
	target_link_directories(pml_dnssd PRIVATE "mdnssd/windows")
	target_link_libraries(pml_dnssd optimized pml_log debug pml_logd)
	target_link_libraries(pml_dnssd dnssd ws2_32 iphlpapi)
endif()

list(APPEND flags "-fPIC" "-Wall" "-fpermissive"  "-std=c++17")

if(CMAKE_BUILD_TYPE MATCHES Release)
   list(APPEND flags "-O3")
   target_compile_definitions(pml_dnssd PUBLIC NDEBUG DNSSD_DLL _MSL_STDINT_H)
else()
   list(APPEND flags "-g")
   target_compile_definitions(pml_dnssd PUBLIC DEBUG DNSSD_DLL _MSL_STDINT_H)
endif()

target_compile_options(pml_dnssd PRIVATE ${flags})
link_directories(${CMAKE_SOURCE_DIR}/lib)


#linux specific
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_compile_definitions(pml_dnssd PRIVATE __GNU__)
	target_link_libraries(pml_dnssd pthread)
	target_link_libraries(pml_dnssd avahi-client)
endif()

set_target_properties(pml_dnssd PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/)

#linux specific
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	install(TARGETS pml_dnssd LIBRARY DESTINATION /usr/local/lib)
endif()
