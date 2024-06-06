# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/esp-idf/Espressif/frameworks/esp-idf-v4.4.7/components/bootloader/subproject"
  "C:/esp-idf/EVSE/build/bootloader"
  "C:/esp-idf/EVSE/build/bootloader-prefix"
  "C:/esp-idf/EVSE/build/bootloader-prefix/tmp"
  "C:/esp-idf/EVSE/build/bootloader-prefix/src/bootloader-stamp"
  "C:/esp-idf/EVSE/build/bootloader-prefix/src"
  "C:/esp-idf/EVSE/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/esp-idf/EVSE/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
