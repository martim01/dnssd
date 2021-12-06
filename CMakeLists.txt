cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(pml_dnssd LANGUAGES CXX)


SET(DIR_BASE $ENV{HOME} CACHE STRING "base location for libraries")
SET(DIR_LOG ${DIR_BASE}/log CACHE STRING "location of pml log")
SET(BUILD_LOG ON CACHE BOOL "set to OFF if pml::log built by parent project")


#Get log
find_path(TEMP_DIR7 NAMES "CMakeLists.txt" PATHS ${DIR_LOG})
if((NOT TEMP_DIR7) OR (NOT EXISTS ${TEMP_DIR7}))
	message("log not found - clone from GitHub")
	execute_process(COMMAND git clone https://github.com/martim01/log/ ${DIR_LOG})
endif()
unset(TEMP_DIR7)

find_path(TEMP_DIR8 NAMES "CMakeLists.txt" PATHS ${DIR_LOG})
if((NOT TEMP_DIR8) OR (NOT EXISTS ${TEMP_DIR8}))
	message(FATAL_ERROR "Failed to clone pml::Log")
endif()
unset(TEMP_DIR8)

if(${BUILD_LOG})
	add_subdirectory(${DIR_LOG} ${DIR_LOG}/build)
else()
	message("dnssd - pml::log being build elsewhere")
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


add_library(pml_dnssd SHARED ${DNS_SOURCES})
set_target_properties(pml_dnssd PROPERTIES DEBUG_POSTFIX "d")

target_include_directories(pml_dnssd PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(pml_dnssd PUBLIC ${DIR_LOG}/include)

list(APPEND flags "-fPIC" "-Wall" "-fpermissive"  "-std=c++14")

if(CMAKE_BUILD_TYPE MATCHES Release)
   list(APPEND flags "-O3")
   target_compile_definitions(pml_dnssd PUBLIC NDEBUG DLL_EXPORTS _MSL_STDINT_H)
else()
   list(APPEND flags "-g")
   target_compile_definitions(pml_dnssd PUBLIC DEBUG DLL_EXPORTS _MSL_STDINT_H)
endif()

target_compile_options(pml_dnssd PRIVATE ${flags})

target_link_libraries(pml_dnssd optimized pml_log debug pml_logd)

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
