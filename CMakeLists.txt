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


set (SQLITE_VERSIONS
    3.7.15.2
    3.34.0
)

foreach(SQLITE_VERSION ${SQLITE_VERSIONS})

    add_library(sqlite3-${SQLITE_VERSION} STATIC)

    target_include_directories(sqlite3-${SQLITE_VERSION} SYSTEM BEFORE
        PUBLIC
        sqlite/${SQLITE_VERSION}
    )

    target_compile_definitions(sqlite3-${SQLITE_VERSION} PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>"
        SQLITE_THREADSAFE=0
        SQLITE_OMIT_DEPRECATED=1
        SQLITE_ENABLE_API_ARMOR=1
        SQLITE_ENABLE_MEMORY_MANAGEMENT=1
        SQLITE_MEMDEBUG=1
        SQLITE_ENABLE_COLUMN_METADATA=1
    )

    target_compile_options(sqlite3-${SQLITE_VERSION} PRIVATE
        "$<$<CXX_COMPILER_ID:AppleClang>:-Wno-unused-function;-Wno-format-pedantic>"
        "$<$<CXX_COMPILER_ID:MSVC>:/Wv:18;/W3;/wd4244;/wd4146;/wd4267;/wd4996>"
    )

    target_sources(sqlite3-${SQLITE_VERSION} PRIVATE
        sqlite/${SQLITE_VERSION}/sqlite3.c
    )
endforeach()

add_subdirectory(lib)
add_subdirectory(test)

