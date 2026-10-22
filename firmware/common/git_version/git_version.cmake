# Commit hash of the firmware, for broadcasting over CAN.
#
# include() this from a board's CMakeLists.txt after add_executable(). On every build it
# regenerates git_version_generated.h from `git` (the file is only rewritten when the commit
# or dirty state changes, so a no-op build stays a no-op), and it adds git_version.h to the
# include path.

set(GIT_VERSION_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
set(GIT_VERSION_GENERATED_HEADER ${GIT_VERSION_GENERATED_DIR}/git_version_generated.h)

add_custom_target(git_version
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_CURRENT_LIST_DIR}
        -DOUTPUT=${GIT_VERSION_GENERATED_HEADER}
        -P ${CMAKE_CURRENT_LIST_DIR}/git_version_gen.cmake
    BYPRODUCTS ${GIT_VERSION_GENERATED_HEADER}
    COMMENT "Reading git commit hash"
    VERBATIM
)

add_dependencies(${CMAKE_PROJECT_NAME} git_version)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${GIT_VERSION_GENERATED_DIR}
)
