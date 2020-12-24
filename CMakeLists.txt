cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0092 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

#set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION 10.0.19041.0)

if (WIN32)
set(CMAKE_SYSTEM_VERSION "10.0.19041.0" CACHE STRING "Windows SDK Version" FORCE)
endif()


project(ThinSQLite++)



set(CLANG_OPTIONS
	-fvisibility=hidden
	-fvisibility-inlines-hidden
	-Wall
    -Wextra
    -pedantic
)

set(APPLE_CLANG_OPTIONS
	${CLANG_OPTIONS}
	-stdlib=libc++
	-fno-common
)

set(MSVC_OPTIONS
	/sdl
	/GR
	/EHsc
	"$<$<CONFIG:RELEASE>:/O2;/Ob2;/Oi;/Ot;/GT;/Qpar;/GL>"
	"$<$<CONFIG:DEBUG>:/Od;/Ob0>"
	/W4
	/WX
)

add_compile_options(
    "$<$<COMPILE_LANG_AND_ID:CXX,Clang>:${CLANG_OPTIONS}>"
    "$<$<COMPILE_LANG_AND_ID:C,Clang>:${CLANG_OPTIONS}>"
    "$<$<CXX_COMPILER_ID:AppleClang>:${APPLE_CLANG_OPTIONS}>"
    "$<$<CXX_COMPILER_ID:MSVC>:${MSVC_OPTIONS}>"
)

set(MSVC_DEFINITIONS
    $<$<CONFIG:DEBUG>:_DEBUG>
)

add_compile_definitions(
    "$<$<CXX_COMPILER_ID:MSVC>:${MSVC_DEFINITIONS}>"
    $<$<CONFIG:RELEASE>:NDEBUG>
)

set(APPLE_CLANG_LINK_OPTONS
    -stdlib=libc++
)

set(MSVC_LINK_OPTIONS
    $<$<CONFIG:DEBUG>:/DEBUG:FASTLINK;/INCREMENTAL>
    $<$<CONFIG:RELEASE>:/DEBUG;/RELEASE;/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/LTCG:incremental>
)

add_link_options(
    "$<$<CXX_COMPILER_ID:AppleClang>:${APPLE_CLANG_LINK_OPTONS}>"
    "$<$<CXX_COMPILER_ID:MSVC>:${MSVC_LINK_OPTIONS}>"
)


add_subdirectory(lib)
add_subdirectory(test)

