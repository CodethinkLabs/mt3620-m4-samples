#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
project(SUPER_SERIOUS C)

# Create executable
add_executable(${PROJECT_NAME} main.c display/SSD1306.c display/SSD1331.c display/display.c display/primitive.c lib/VectorTable.c lib/GPT.c lib/GPIO.c lib/UART.c lib/Print.c lib/I2CMaster.c lib/SPIMaster.c)
target_link_libraries(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

azsphere_configure_tools(TOOLS_REVISION "20.10")

# Add MakeImage post-build command
azsphere_target_add_image_package(${PROJECT_NAME})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -L${CMAKE_SOURCE_DIR})
