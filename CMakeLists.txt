# AUTOMOC requires 2.8.6. The CMAKE_CXX_STANDARD variable requires 3.1.
cmake_minimum_required(VERSION 3.8)
project(morphologica)
set(VERSION 0.1)
set(SOVERSION 1.0)

message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  (This can be changed with `cmake -DCMAKE_INSTALL_PREFIX=/some/place`")

# From CMAKE_SYSTEM work out which of __OSX__, __GLN__, __NIX__ or
# __WIN__ are required
message(STATUS "Operating system: " ${CMAKE_SYSTEM})
if(CMAKE_SYSTEM MATCHES Linux.*)
  set(MORPH_HOST_DEFINITION "-D__GLN__")
elseif(CMAKE_SYSTEM MATCHES BSD.*)
  set(MORPH_HOST_DEFINITION "-D__NIX__")
elseif(APPLE)
  set(MORPH_HOST_DEFINITION "-D__OSX__")
elseif(CMAKE_SYSTEM MATCHES Win.*)
  set(MORPH_HOST_DEFINITION "-D__WIN__")
else()
  message(ERROR " Operating system not supported: " ${CMAKE_SYSTEM})
endif()

# Need to find the host system processor to fine tune assembly in VisualFace.h
message(STATUS "Host system processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES aarch64)
  set(MORPH_HOST_DEFINITION "${MORPH_HOST_DEFINITION} -D__AARCH64__")
endif()

# An option that's useful for Ubuntu 16.04 builds OR wxWidgets
option(USE_GLEW "Link libglew.so (try if the linker can't find glCreateVertexArrays)" OFF)

# Using c++-14 (calls to std::decay_t in number_type.h). c++-17
# required to allow brace initializers for the morph::Vector class
set (CMAKE_CXX_STANDARD 17)

# There's ONE class that requires C++-20. C++-20 support is available
# in Gnu GCC so if we have a recent enough version of that, then go
# ahead and compile the morph::Grid code.
set (CXX_20_AVAILABLE 0)
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 10.0)
    message(STATUS "C++-20 is available on Gnu GCC version 10.0 and up")
    set (CXX_20_AVAILABLE 1) # Can do C++-20
  endif()
endif()

# Add the host definition to CXXFLAGS along with other switches
if (APPLE)
  set(CMAKE_CXX_FLAGS "${MORPH_HOST_DEFINITION} -Wall -g -O3")
else()
  if(NOT CMAKE_SYSTEM MATCHES Win.*)
    # To use Intel compiler, you can call cmake as: `cmake -DCMAKE_CXX_COMPILER=icpc ..` or `CXX=icpc cmake ..`
    if (CMAKE_CXX_COMPILER_ID MATCHES Intel)
      set(CMAKE_CXX_FLAGS "${MORPH_HOST_DEFINITION} -Wall -g -std=c++17 -xHOST -O3 -D__ICC__")
    else () # GCC or Clang
      if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
        # Add compiler version check, to ensure gcc is version 7 or later.
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.1)
          message(FATAL_ERROR "GCC version must be at least 7.1! Use CXX=g++-7 cmake (options) and don't forget to clean out your build directory first!")
        else()
          message(STATUS "GCC version ${CMAKE_CXX_COMPILER_VERSION} OK!")
        endif()
      endif()
      set(COMPREHENSIVE_WARNING_FLAGS "-Wall -Wextra -Wpedantic -pedantic-errors -Werror -Wfatal-errors -Wno-psabi")
      set(CMAKE_CXX_FLAGS "${MORPH_HOST_DEFINITION} -g ${COMPREHENSIVE_WARNING_FLAGS} -march=native -O3")
    endif()
  else() # Windows
    # Set flags for Windows. /EHsc required for use of exceptions
    set(CMAKE_CXX_FLAGS "${MORPH_HOST_DEFINITION} -DNOMINMAX /EHsc")
  endif() # CMAKE_SYSTEM MATCHES Win.*
endif()

# Tell clang to be quiet about brace initialisers
if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")
endif()

# Note that we require OpenMP version 3 or above for ability to use
# unsigned integral types in parallel for loops. At time of writing,
# Visual Studio 2019 offers OpenMP 2 by default and 3 with the special
# switch -openmp:llvm. I don't know how to get cmake to use the
# special version, so OpenMP will not work at present on
# Windows/VS2019. But then, you wouldn't use Windows for serious
# numerical work anyway.
find_package(OpenMP 3)
if(OpenMP_CXX_FOUND)
  message(STATUS "Found OpenMP version ${OpenMP_C_VERSION}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Following `cmake --help-policy CMP0072`
set(OpenGL_GL_PREFERENCE "GLVND")

# Additional GL compiler flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL3_PROTOTYPES -DGL_GLEXT_PROTOTYPES")
if(USE_GLEW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_GLEW")
endif(USE_GLEW)

if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION")
endif()

# The code in VisualFace which builds the Vera family truetype fonts
# into the program binary needs to have a define of MORPH_FONTS_DIR,
# so set it up here:
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMORPH_FONTS_DIR=\"\\\"${PROJECT_SOURCE_DIR}/fonts\\\"\"")
# Note that in client code you may have to do something similar. For
# example, if you're compiling with morphologica in a subdirectory, you may need:
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMORPH_FONTS_DIR=\"\\\"${PROJECT_SOURCE_DIR}/morphologica/fonts\\\"\"")

# Lib finding
find_package(OpenCV) # No longer required for morph::Visual, used by some tests
if(APPLE)
  find_package(OpenGL REQUIRED)
else()
  find_package(OpenGL REQUIRED EGL) # GL ES 3.1 used in one gl compute example
endif()

if(USE_GLEW)
  find_package(GLEW REQUIRED)
  include_directories(${GLEW_INCLUDE_DIRS})
endif(USE_GLEW)
find_package(glfw3 3.2...3.4 REQUIRED)
# Find the HDF5 library. To prefer the use of static linking of HDF5, set HDF5_USE_STATIC_LIBRARIES first
#set(HDF5_USE_STATIC_LIBRARIES ON)
find_package(HDF5)
if(HDF5_FOUND)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${HDF5_DEFINITIONS}")
endif()
find_package(Armadillo)

if(${OpenCV_FOUND})
  include_directories(${OpenCV_INCLUDE_DIRS})
endif()
include_directories(${OPENGL_INCLUDE_DIR})
if(HDF5_FOUND)
  include_directories(${HDF5_INCLUDE_DIR})
endif()
if(ARMADILLO_FOUND)
  # Two possible values for the armadillo include dirs
  include_directories(${ARMADILLO_INCLUDE_DIR} ${ARMADILLO_INCLUDE_DIRS})
endif()
include_directories(${GLFW3_INCLUDE_DIR})

# New, text rendering dependencies
find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

# If Qt5 is present, then some optional Qt examples will be compiled
find_package(Qt5 QUIET COMPONENTS Gui Core Widgets)
set(FOUND_QT_AND_HAVE_NO_GLEW 0)
if(Qt5_FOUND)
  if(USE_GLEW)
    message(WARNING "Qt was found, but GLEW was requested with -DUSE_GLEW=ON. GLEW interferes with Qt, so not compiling the Qt examples")
  else()
    set(FOUND_QT_AND_HAVE_NO_GLEW 1)
    message(STATUS "Compiling Qt examples")
  endif()
endif()


# wxwidgets. Optional. Use `apt install libwxgtk3.2-dev` on Debian/Ubuntu Linux (Min compatible Ubuntu version is 23.04)
set(FOUND_WX_AND_GLEW 0)
set(WX_OPTIONAL_COMPONENTS "")
find_package(wxWidgets 3.1 COMPONENTS gl core base ${WX_OPTIONAL_COMPONENTS} net QUIET)
if(wxWidgets_FOUND)
  if(USE_GLEW)
    message(STATUS "Compiling wx examples")
    set(FOUND_WX_AND_GLEW 1)
  else()
    message(WARNING "The wxWidgets examples require GLEW. wxWidgets was found but USE_GLEW is OFF. You will need to call cmake with the -DUSE_GLEW=ON option to compile wxWidgets examples.")
  endif()
endif()

# Make it possible to compile complex constexpr functions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconstexpr-ops-limit=5000000000")

# rapidxml is bundled in the source, but its headers will be installed in ${CMAKE_INSTALL_PREFIX}/morph/, and they're symlinked in ${PROJECT_SOURCE_DIR}/morph
#include_directories("${PROJECT_SOURCE_DIR}/include/rapidxml-1.13")

# OpenGL code needs the local include directory, too
include_directories("${PROJECT_SOURCE_DIR}/include")

# Library code is compiled up as a shared library in lib/ (could also
# be compiled static if needed)
add_subdirectory(morph)

# We have to make a morph/rapidxml-1.13 installation for client code
# that uses the ReadCurves class.
add_subdirectory(include)

# Unit testing using the ctest framework
enable_testing()
add_subdirectory(tests)

# incbin for Visual Studio
if(WIN32)
add_subdirectory(buildtools)
endif()

# Install the font files, for the examples, which seek to work with an
# *installed* morphologica, as opposed to an in-tree morphologica.
add_subdirectory(fonts)

# Example code (you can also see tests/ for examples)
add_subdirectory(examples)

# first we can indicate the documentation build as an option and set it to ON by default
option (BUILD_DOC "Build documentation" OFF)
if (BUILD_DOC)
  # check if Doxygen is installed
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    # note the option ALL which allows to build the docs together with the application
    add_custom_target(doc_doxygen ALL
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM )
  else(DOXYGEN_FOUND)
    message("Need doxygen for documentation")
  endif(DOXYGEN_FOUND)
endif (BUILD_DOC)

# For debugging of variables:
option(DEBUG_VARIABLES OFF)
if(DEBUG_VARIABLES)
  get_cmake_property(_variableNames VARIABLES)
  foreach(_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
  endforeach()
endif(DEBUG_VARIABLES)
