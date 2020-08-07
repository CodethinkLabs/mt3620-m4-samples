#  Copyright (c) Codethink Ltd. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.11)
project(ADC_RTApp_MT3620_BareMetal C)

# Create executable
add_executable(${PROJECT_NAME} main.c lib/ADC.c lib/VectorTable.c lib/GPT.c lib/UART.c lib/Print.c lib/GPIO.c)
target_link_libraries(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

azsphere_configure_tools(TOOLS_REVISION "20.07")

# Add MakeImage post-build command
include("${AZURE_SPHERE_MAKE_IMAGE_FILE}")

target_link_libraries(${PROJECT_NAME} PUBLIC -L"${CMAKE_SOURCE_DIR}")
