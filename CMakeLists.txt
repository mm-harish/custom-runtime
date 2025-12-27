cmake_minimum_required(VERSION 3.15)
project(RuntimeProject C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER clang++-20)

# Generate assembly + keep normal build products
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fvectorize -march=native -flto -O3 -march=native -mtune=native -fno-exceptions -fno-rtti")

option(PERFORM_VALIDATION "Enable validation" OFF)
if(PERFORM_VALIDATION)
    add_definitions(-DPERFORM_VALIDATION)
endif()

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

# ============= OpenCilk Version (Optional) =============
# Explicitly search for the OpenCilk compiler
find_program(OPENCILK_CXX clang++ PATHS /opt/opencilk/bin NO_DEFAULT_PATH)

if(OPENCILK_CXX)
    message(STATUS "OpenCilk compiler found: ${OPENCILK_CXX} - building fib_cilk")
    
    # We use a custom command to build fib_cilk with its own compiler
    # while the rest of the project uses CMAKE_CXX_COMPILER (clang++-20)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fib_cilk
        COMMAND ${OPENCILK_CXX} -fopencilk -O3 -march=native -mtune=native 
                ${CMAKE_CURRENT_SOURCE_DIR}/apps/fib/fib_cilk.cpp 
                -o ${CMAKE_CURRENT_BINARY_DIR}/fib_cilk
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/apps/fib/fib_cilk.cpp
        COMMENT "Building fib_cilk with OpenCilk compiler"
        VERBATIM
    )
    
    # Create a custom target that is built by default
    add_custom_target(fib_cilk_target ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/fib_cilk)

else()
    message(WARNING "OpenCilk compiler not found at /opt/opencilk/bin/clang++. fib_cilk will not be built. To enable Cilk comparison tests, install OpenCilk from https://opencilk.org")
endif()



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

