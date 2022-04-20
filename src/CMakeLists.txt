# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
idf_component_register(SRCS ${app_sources} 
    EMBED_TXTFILES 
        "certs/aws_root_c1.pem" 
        "certs/device.pem.crt" 
        "certs/private.pem.key")


