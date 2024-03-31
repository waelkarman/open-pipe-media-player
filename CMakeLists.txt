cmake_minimum_required(VERSION 3.16)

project(open-pipe-media-player LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

add_executable(${PROJECT_NAME}-1-2 ./drafts/draft1-2.c)
add_executable(${PROJECT_NAME}-3 ./drafts/draft3.c)
add_executable(${PROJECT_NAME} draft4.c)


add_definitions(${GTK3_CFLAGS_OTHER})
target_compile_options(${PROJECT_NAME}-1-2 PRIVATE ${GST_CFLAGS})
target_compile_options(${PROJECT_NAME}-3 PRIVATE ${GST_CFLAGS})
target_compile_options(${PROJECT_NAME} PRIVATE ${GST_CFLAGS})

target_link_libraries(${PROJECT_NAME}-1-2 PRIVATE 
    ${GST_LIBRARIES}
    ${GLIB_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME}-3 PRIVATE 
    ${GST_LIBRARIES}
    ${GLIB_LIBRARIES}
)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${GST_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GTK3_LIBRARIES}
)