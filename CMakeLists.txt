cmake_minimum_required (VERSION 2.8)
project (c-ray C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin/)

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
	# set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -Wconversion -std-gnu99")
	set(CMAKE_C_FLAGS "-Wall -Wextra -Wshadow -Wstrict-aliasing -Wfloat-equal -Wpointer-arith -Wwrite-strings -Wformat=2 -Wstrict-overflow -Wunreachable-code -Wredundant-decls -Wmissing-declarations -Wold-style-definition -Wnested-externs -Wcast-qual -Wmissing-format-attribute -Wmissing-noreturn -std=c99")
	set(CMAKE_C_FLAGS_DEBUG "-fsanitize=address,undefined -O0 -g")
	set(CMAKE_C_FLAGS_RELEASE "-O3 -ftree-vectorize -ffast-math")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${c-ray_SOURCE_DIR}/cmake")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
configure_file("${c-ray_SOURCE_DIR}/src/utils/gitsha1.c.in" "${c-ray_BINARY_DIR}/src/utils/gitsha1.c" @ONLY)
list(APPEND SOURCES "${c-ray_BINARY_DIR}/src/utils/gitsha1.c" "${c-ray_SOURCE_DIR}/src/utils/gitsha1.h")

FILE(GLOB_RECURSE CSources src/*.c)
add_executable(c-ray ${CSources})

if (NOT NO_SDL2)
	find_package(SDL2 QUIET)

	if (SDL2_FOUND)
		message(STATUS "SDL2 found! Enabling UI components.")
		include_directories(${SDL2_INCLUDE_DIR})

		if (MSVC)
  			target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY})
		else()
  			target_link_libraries(${PROJECT_NAME} -lpthread -lm ${SDL2_LIBRARY})
		endif()
		add_definitions(-DUI_ENABLED)
	else()
		message(STATUS "SDL2 not found! You should install it, it'll be awesome! See README for instructions.")
		if (MSVC)
			target_link_libraries(${PROJECT_NAME})
		else()
			target_link_libraries(${PROJECT_NAME} -lpthread -lm)
		endif()
	endif()
else()
	message(STATUS "SDL2 explicitly disabled, disabling UI components")
	unset(NO_SDL2 CACHE)
	if (MSVC)
		target_link_libraries(${PROJECT_NAME})
	else()
		target_link_libraries(${PROJECT_NAME} -lpthread -lm)
	endif()
endif()
