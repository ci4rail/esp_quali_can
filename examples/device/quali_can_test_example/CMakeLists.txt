# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components)
set(EXTRA_COMPONENT_DIRS ../../..
                         ../../../../esp_test_status_report
                         ../../../../esp_io4edge
                         $ENV{IDF_PATH}/examples/common_components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(quali_can_test_example)
