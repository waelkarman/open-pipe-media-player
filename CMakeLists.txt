cmake_minimum_required(VERSION 3.16)

project(openAudioSwitcher LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_search_module(GLIB REQUIRED glib-2.0)

add_executable(${PROJECT_NAME}-1-2 ./drafts/draft1-2.c)
add_executable(${PROJECT_NAME}-3 ./drafts/draft3.c)
add_executable(${PROJECT_NAME}-4 main4.c)

target_compile_options(${PROJECT_NAME}-1-2 PRIVATE ${GST_CFLAGS})
target_compile_options(${PROJECT_NAME}-3 PRIVATE ${GST_CFLAGS})
target_compile_options(${PROJECT_NAME}-4 PRIVATE ${GST_CFLAGS})

target_link_libraries(${PROJECT_NAME}-1-2 PRIVATE 
    ${GST_LIBRARIES}
    ${GLIB_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME}-3 PRIVATE 
    ${GST_LIBRARIES}
    ${GLIB_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME}-4 PRIVATE 
    ${GST_LIBRARIES}
    ${GLIB_LIBRARIES}
)
