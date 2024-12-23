# CMakeList.txt : CMake project for exe, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (app "main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.20)
  set_property(TARGET app PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
target_link_libraries(app PRIVATE math)
target_include_directories(app PUBLIC "${PROJECT_SOURCE_DIR}/math")