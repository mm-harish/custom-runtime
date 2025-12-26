cmake_minimum_required(VERSION 3.15)
project(RuntimeProject C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER clang++-20)

# Generate assembly + keep normal build products
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fvectorize -march=native -flto -O3 -march=native -mtune=native -fno-exceptions -fno-rtti")

include_directories(include)

find_package(LLVM REQUIRED CONFIG)
find_package(PkgConfig REQUIRED)
#pkg_check_modules(GPERFTOOLS REQUIRED libtcmalloc)
include_directories(${LLVM_INCLUDE_DIR})

add_executable(fib
    apps/fib/main.cpp
)

#target_include_directories(runtime PRIVATE ${GPERFTOOLS_INCLUDE_DIRS})
#target_link_directories(runtime PRIVATE ${GPERFTOOLS_LIBRARY_DIRS})

target_link_libraries(fib 
    PRIVATE 
    LLVMSupport
 #   ${GPERFTOOLS_LIBRARIES}
)


# Doxygen
find_package(Doxygen)
if (DOXYGEN_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    # note the option ALL which allows to build the docs together with the application
    add_custom_target( doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)
