# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/spanddhana/esp/esp-idf/tools/kconfig"
  "/Users/spanddhana/Downloads/Nobel-FYP-Opportunistic-Network-main/TagAlong-8bit/Firmware/ESP32/build/kconfig_bin"
  "/Users/spanddhana/Downloads/Nobel-FYP-Opportunistic-Network-main/TagAlong-8bit/Firmware/ESP32/build/mconf-idf-prefix"
  "/Users/spanddhana/Downloads/Nobel-FYP-Opportunistic-Network-main/TagAlong-8bit/Firmware/ESP32/build/mconf-idf-prefix/tmp"
  "/Users/spanddhana/Downloads/Nobel-FYP-Opportunistic-Network-main/TagAlong-8bit/Firmware/ESP32/build/mconf-idf-prefix/src/mconf-idf-stamp"
  "/Users/spanddhana/Downloads/Nobel-FYP-Opportunistic-Network-main/TagAlong-8bit/Firmware/ESP32/build/mconf-idf-prefix/src"
  "/Users/spanddhana/Downloads/Nobel-FYP-Opportunistic-Network-main/TagAlong-8bit/Firmware/ESP32/build/mconf-idf-prefix/src/mconf-idf-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/spanddhana/Downloads/Nobel-FYP-Opportunistic-Network-main/TagAlong-8bit/Firmware/ESP32/build/mconf-idf-prefix/src/mconf-idf-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/spanddhana/Downloads/Nobel-FYP-Opportunistic-Network-main/TagAlong-8bit/Firmware/ESP32/build/mconf-idf-prefix/src/mconf-idf-stamp${cfgdir}") # cfgdir has leading slash
endif()
