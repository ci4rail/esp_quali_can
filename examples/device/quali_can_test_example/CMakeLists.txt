# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXCLUDE_COMPONENTS asio coap)
set(EXTRA_COMPONENT_DIRS ../../..
                         ../../../../esp_test_status_report)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(quali_can_test_example)
