cmake_minimum_required(VERSION 3.16.0)
list(APPEND EXTRA_COMPONENT_DIRS wifi-manager ssd1306)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ESP_32_IDF)