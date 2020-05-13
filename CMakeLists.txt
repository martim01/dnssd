cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(pml_dnssd LANGUAGES CXX)

list(APPEND SOURCES "src/dnssd.cpp"  "src/zcposter.cpp")

#os specific
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	list(APPEND SOURCES "src/gnu/avahibrowser.cpp" "src/gnu/avahipublisher.cpp")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	list(APPEND SOURCES "src/windows/bonjourbrowser.cpp" "src/windows/bonjourpublisher.cpp")
endif()


add_library(pml_dnssd SHARED ${SOURCES})

target_include_directories(pml_dnssd PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(pml_dnssd PUBLIC ${PROJECT_SOURCE_DIR}/../log/include)

list(APPEND flags "-Wall" "-s" "-O3" "-fexceptions" "-std=c++14")
target_compile_options(pml_dnssd PRIVATE ${flags})

target_link_libraries(pml_dnssd pml_log)

#linux specific
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_compile_definitions(pml_dnssd PRIVATE __GNU__)
	target_link_libraries(pml_dnssd pthread)
endif()

set_target_properties(pml_dnssd PROPERTIES LIBRARY_OUTPUT_DIRECTORY "../lib/")

#linux specific
#if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#	install(TARGETS pml_dnssd LIBRARY DESTINATION /usr/local/lib)
#endif()
