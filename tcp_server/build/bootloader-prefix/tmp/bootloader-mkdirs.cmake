# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/matheus/esp/esp-idf/components/bootloader/subproject"
  "/home/matheus/esp/esp-idf/examples/my_projects/tcp_server/build/bootloader"
  "/home/matheus/esp/esp-idf/examples/my_projects/tcp_server/build/bootloader-prefix"
  "/home/matheus/esp/esp-idf/examples/my_projects/tcp_server/build/bootloader-prefix/tmp"
  "/home/matheus/esp/esp-idf/examples/my_projects/tcp_server/build/bootloader-prefix/src/bootloader-stamp"
  "/home/matheus/esp/esp-idf/examples/my_projects/tcp_server/build/bootloader-prefix/src"
  "/home/matheus/esp/esp-idf/examples/my_projects/tcp_server/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/matheus/esp/esp-idf/examples/my_projects/tcp_server/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/matheus/esp/esp-idf/examples/my_projects/tcp_server/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
