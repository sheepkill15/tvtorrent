link_directories(
    ${GTKMM_LIBRARY_DIRS}
    ${LIBTORRENT_LIBRARY_DIRS}
    ${JSONCPP_LIBRARY_DIRS}
)

include_directories(
    ${GTKMM_INCLUDE_DIRS}
    ${LIBTORRENT_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
)

add_definitions(
    ${GTKMM_CFLAGS_OTHER}
    ${LIBTORRENT_CFLAGS_OTHER}
    ${JSONCPP_CFLAGS_OTHER}
)

file(GLOB SRC "./*.cpp")
add_executable(tvtorrent ${SRC})

target_compile_features(tvtorrent PRIVATE cxx_std_17)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

install(DIRECTORY DESTINATION /usr/local/share/TVTorrent)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/res DESTINATION /usr/local/share/TVTorrent)

install(TARGETS tvtorrent DESTINATION bin)

target_link_libraries(tvtorrent PRIVATE ${GTKMM_LIBRARIES}
    ${LIBTORRENT_LIBRARIES}
    ${JSONCPP_LIBRARIES}
)