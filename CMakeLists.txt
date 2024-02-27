cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(pml_dnssd LANGUAGES CXX VERSION 1.2.0)

message(STATUS "----pml/dnssd ----")

execute_process(COMMAND ${CMAKE_COMMAND} -DNAMESPACE=dnssd -DMAJOR=${PROJECT_VERSION_MAJOR} -DMINOR=${PROJECT_VERSION_MINOR} -DPATCH=${PROJECT_VERSION_PATCH} -P ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake)

SET(DIR_BASE $ENV{HOME} CACHE STRING "base location for libraries")
SET(DIR_LOG ${DIR_BASE}/log CACHE STRING "location of pml log")

if(NOT DEFINED BUILD_LOG)
	SET(BUILD_LOG ON)
	#Get log
	find_path(TEMP_DIR7 NAMES "CMakeLists.txt" PATHS ${DIR_LOG} NO_CACHE)
	if((NOT TEMP_DIR7) OR (NOT EXISTS ${TEMP_DIR7}))
		message(STATUS "log not found - clone from GitHub")
		execute_process(COMMAND git clone https://github.com/martim01/log/ ${DIR_LOG})
	endif()
	unset(TEMP_DIR7)

	find_path(TEMP_DIR8 NAMES "CMakeLists.txt" PATHS ${DIR_LOG} NO_CACHE)
	if((NOT TEMP_DIR8) OR (NOT EXISTS ${TEMP_DIR8}))
		message(FATAL_ERROR "Failed to clone pml::Log")
	endif()
	unset(TEMP_DIR8)
	add_subdirectory(${DIR_LOG} ${CMAKE_SOURCE_DIR}/build/log)
else()
	message(STATUS "dnssd - pml::log being build elsewhere")
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
	list(APPEND DNS_SOURCES "src/windows/bonjourbrowser.cpp" "src/windows/bonjourpublisher.cpp")
endif()


add_library(pml_dnssd SHARED ${DNS_SOURCES} ${CMAKE_BINARY_DIR}/src/dnssd_version.cpp)
set_target_properties(pml_dnssd PROPERTIES DEBUG_POSTFIX "d")

target_include_directories(pml_dnssd PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(pml_dnssd PUBLIC ${DIR_LOG}/include)
target_include_directories(pml_dnssd PUBLIC ${CMAKE_BINARY_DIR}/include)

list(APPEND flags "-fPIC" "-Wall" "-fpermissive"  "-std=c++17")

if(CMAKE_BUILD_TYPE MATCHES Release)
   list(APPEND flags "-O3")
   target_compile_definitions(pml_dnssd PUBLIC NDEBUG DLL_EXPORTS _MSL_STDINT_H)
else()
   list(APPEND flags "-g")
   target_compile_definitions(pml_dnssd PUBLIC DEBUG DLL_EXPORTS _MSL_STDINT_H)
endif()

target_compile_options(pml_dnssd PRIVATE ${flags})
link_directories(${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(pml_dnssd optimized pml_log debug pml_logd)
target_link_libraries(pml_dnssd avahi-client)

#linux specific
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_compile_definitions(pml_dnssd PRIVATE __GNU__)
	target_link_libraries(pml_dnssd pthread)
endif()

set_target_properties(pml_dnssd PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib/)

#linux specific
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	install(TARGETS pml_dnssd LIBRARY DESTINATION /usr/local/lib)
endif()
