#  Copyright (c) Codethink Ltd. All rights reserved.
#  Licensed under the MIT License.

CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
PROJECT(GPT_RTApp_MT3620_BareMetal C)

# Create executable
ADD_EXECUTABLE(${PROJECT_NAME} main.c lib/VectorTable.c lib/GPIO.c lib/UART.c lib/Print.c lib/GPT.c)
TARGET_LINK_LIBRARIES(${PROJECT_NAME})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

# Add MakeImage post-build command
INCLUDE("${AZURE_SPHERE_MAKE_IMAGE_FILE}")

TARGET_LINK_LIBRARIES(${PROJECT_NAME} PUBLIC -L"${CMAKE_SOURCE_DIR}")
