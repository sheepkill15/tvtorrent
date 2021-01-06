
file(GLOB SRC "./*.cpp")

set(RES_FILES "")

if(MINGW)
    set(RES_FILES "build.rc" "info.rc")
    set(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -i <SOURCE> -o <OBJECT>")
endif(MINGW)

if(NOT WIN32)
add_executable(tvtorrent ${SRC} ${RES_FILES})
endif()
if(WIN32)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_executable(tvtorrent ${SRC} ${RES_FILES})
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_executable(tvtorrent WIN32 ${SRC} ${RES_FILES})
    endif()

endif()
target_compile_features(tvtorrent PRIVATE cxx_std_17)


set(CMAKE_THREAD_LIBS_INIT "-lpthread")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)


#if(NOT WIN32)
link_directories(
        ${GTKMM_LIBRARY_DIRS}
        ${LIBTORRENT_LIBRARY_DIRS}
        ${JSONCPP_LIBRARY_DIRS}
        ${CURL_LIBRARY_DIRS}
)

if(NOT WIN32)
include_directories(
        ${GTKMM_INCLUDE_DIRS}
        ${LIBTORRENT_INCLUDE_DIRS}
        ${JSONCPP_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        /usr/include/rapidxml
)
endif()

if(WIN32)
    include_directories(
        ${GTKMM_INCLUDE_DIRS}
        ${LIBTORRENT_INCLUDE_DIRS}
        ${JSONCPP_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/third_party/
)
endif()

add_definitions(
        ${GTKMM_CFLAGS_OTHER}
        ${LIBTORRENT_CFLAGS_OTHER}
        ${JSONCPP_CFLAGS_OTHER}
        ${CURL_CFLAGS_OTHER}
)

if(NOT WIN32)
install(DIRECTORY DESTINATION /usr/local/share/TVTorrent)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/res DESTINATION /usr/local/share/TVTorrent)

install(TARGETS tvtorrent DESTINATION bin)
endif()

if(WIN32)

    install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/res DESTINATION ${CMAKE_INSTALL_PREFIX})

    install(TARGETS tvtorrent DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

endif()


file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/src/res DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(tvtorrent PRIVATE ${GTKMM_LIBRARIES}
    ${LIBTORRENT_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${CURL_LIBRARIES}
)