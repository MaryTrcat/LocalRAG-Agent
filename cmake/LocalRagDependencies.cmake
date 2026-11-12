set(LOCALRAG_SQLITE_INCLUDE_DIR "" CACHE PATH "Directory containing sqlite3.h")
set(LOCALRAG_SQLITE_LIBRARY "" CACHE FILEPATH "Path to the SQLite library")

if(NOT LOCALRAG_SQLITE_INCLUDE_DIR)
    set(_localrag_default_sqlite_include "E:/cstudy/x86_64-13.2.0-release-win32-seh-ucrt-rt_v11-rev1/mingw64/opt/include")
    if(EXISTS "${_localrag_default_sqlite_include}/sqlite3.h")
        set(LOCALRAG_SQLITE_INCLUDE_DIR "${_localrag_default_sqlite_include}" CACHE PATH "Directory containing sqlite3.h" FORCE)
    endif()
endif()

if(NOT LOCALRAG_SQLITE_LIBRARY)
    set(_localrag_default_sqlite_library "E:/cstudy/x86_64-13.2.0-release-win32-seh-ucrt-rt_v11-rev1/mingw64/opt/lib/libsqlite3.a")
    if(EXISTS "${_localrag_default_sqlite_library}")
        set(LOCALRAG_SQLITE_LIBRARY "${_localrag_default_sqlite_library}" CACHE FILEPATH "Path to the SQLite library" FORCE)
    endif()
endif()

if(NOT EXISTS "${LOCALRAG_SQLITE_INCLUDE_DIR}/sqlite3.h" OR NOT EXISTS "${LOCALRAG_SQLITE_LIBRARY}")
    message(FATAL_ERROR
        "SQLite was not found. Set LOCALRAG_SQLITE_INCLUDE_DIR to the directory containing sqlite3.h "
        "and LOCALRAG_SQLITE_LIBRARY to the SQLite library file.")
endif()
