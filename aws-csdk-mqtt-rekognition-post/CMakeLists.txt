cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(sample-aws-iot)
list(APPEND EXTRA_COMPONENT_DIRS esp-aws-iot)