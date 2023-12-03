cmake_minimum_required (VERSION 3.9)
project (c-ray C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin/)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "The C compiler is: ${CMAKE_C_COMPILER}")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

if (MSVC)
	add_definitions(-DWINDOWS)

	set(CMAKE_CONFIGURATION_TYPES Release Debug CACHE STRING "" FORCE)

	# Silence "deprecation" warnings
	add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_DEPRECATE /D_SCL_SECURE_NO_WARNINGS)
	# Avoid non-compliant standard library extensions
	add_definitions(/D_CRT_DECLARE_NONSTDC_NAMES)
	# Avoid windows.h junk
	add_definitions(/DNOMINMAX)
	# Avoid windows.h from including some usually unused libs like winsocks.h, since this might cause some redefinition errors.
	add_definitions(/DWIN32_LEAN_AND_MEAN)

	set(CMAKE_C_FLAGS   "/W3 /MP /Zi /Zo /permissive-" CACHE STRING "" FORCE)
	set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /EHsc /std:c++latest /Zc:throwingNew,inline" CACHE STRING "" FORCE)
	set(CMAKE_C_FLAGS_DEBUG   "/Od /MDd" CACHE STRING "" FORCE)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE STRING "" FORCE)
	set(CMAKE_C_FLAGS_RELEASE   "/O2 /GS- /MD" CACHE STRING "" FORCE)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "" FORCE)
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG   "/DEBUG /MANIFEST:NO" CACHE STRING "" FORCE)
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG /MANIFEST:NO /INCREMENTAL:NO /OPT:REF,ICF" CACHE STRING "" FORCE)
else()
	# set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -Wconversion -std=gnu99")
	set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-missing-field-initializers -std=c99 -D_GNU_SOURCE")
	set(CMAKE_C_FLAGS_RELEASE "-O2 -ftree-vectorize -g")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g")
	if (ASAN)
		unset(UBSAN CACHE)
		unset(ASAN CACHE)
		unset(TSAN CACHE)
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
		message(STATUS "Address sanitizer enabled")
	elseif(UBSAN)
		unset(ASAN CACHE)
		unset(UBSAN CACHE)
		unset(TSAN CACHE)
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=undefined")
		message(STATUS "Undefined behaviour sanitizer enabled")
	elseif(TSAN)
		unset(ASAN CACHE)
		unset(TSAN CACHE)
		unset(UBSAN CACHE)
		SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=thread")
		message(STATUS "Thread sanitizer enabled")
	else()
		message(STATUS "No sanitizations")
	endif()
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DCRAY_DEBUG_ENABLED")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${c-ray_SOURCE_DIR}/cmake")
file(GLOB_RECURSE SOURCES src/*.c)

include(set_up_ccache)

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
configure_file("${c-ray_SOURCE_DIR}/src/common/gitsha1.c.in" "${c-ray_BINARY_DIR}/generated/gitsha1.c" @ONLY)
list(APPEND SOURCES "${c-ray_BINARY_DIR}/generated/gitsha1.c")

add_executable(c-ray ${SOURCES})
target_include_directories(c-ray PRIVATE ${c-ray_SOURCE_DIR}/include)
target_include_directories(c-ray PRIVATE ${c-ray_SOURCE_DIR}/tests)
if (NOT MSVC)
	target_link_libraries(c-ray PRIVATE -lpthread -lm -ldl)
endif ()

include(CheckIPOSupported)
if (CMAKE_BUILD_TYPE STREQUAL "Release")
	check_ipo_supported(RESULT ipo_supported OUTPUT ipo_errors)
	if (ipo_supported)
		message(STATUS "Enabling IPO")
		set_property(TARGET c-ray PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
	else()
		message(WARNING "IPO is not supported by this compiler. Everything will work, just a bit slower for now!")
	endif()
endif()

if (NO_IPO)
	message(STATUS "Explicitly disabling IPO")
	set_property(TARGET c-ray PROPERTY INTERPROCEDURAL_OPTIMIZATION FALSE)
	unset(NO_IPO CACHE)
endif()

if (NO_LOGO)
	target_compile_definitions(c-ray PRIVATE -DNO_LOGO)
endif()

if (TESTING)
	message(STATUS "Enabling test suite.")
	target_compile_definitions(c-ray PRIVATE -DCRAY_TESTING)
endif()
