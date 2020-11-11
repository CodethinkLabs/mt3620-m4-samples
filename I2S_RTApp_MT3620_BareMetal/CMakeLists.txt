#  Copyright (c) Codethink Ltd. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.11)
project(I2S_RTApp_MT3620_BareMetal C)

# Create executable
add_executable(${PROJECT_NAME} main.c MAX98090.c lib/VectorTable.c lib/GPT.c lib/GPIO.c lib/UART.c lib/Print.c lib/I2S.c lib/I2CMaster.c)
target_link_libraries(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

azsphere_configure_tools(TOOLS_REVISION "20.10")

# Add MakeImage post-build command
azsphere_target_add_image_package(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC -L"${CMAKE_SOURCE_DIR}")
