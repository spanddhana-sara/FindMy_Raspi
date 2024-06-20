# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)


# ESP-IDF components
set(EXTRA_COMPONENT_DIRS components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(radiotunnel_audio_tx)


# Include ggwave library
idf_build_get_property(project_dir PROJECT_DIR)
set(GGWAVE_COMPONENT_DIR ${project_dir}/components/ggwave)
idf_component_register(SRCS "" INCLUDE_DIRS ${GGWAVE_COMPONENT_DIR})

# Include DFRobot_VEML7700 library
idf_component_register(SRCS "DFRobot_VEML7700.cpp" INCLUDE_DIRS "DFRobot_VEML7700")

# Add ESP-IDF specific settings
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable)