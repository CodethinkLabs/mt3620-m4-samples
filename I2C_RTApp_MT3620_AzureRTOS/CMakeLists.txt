#  Copyright (c) Codethink Ltd. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.11)
project(I2C_RTApp_MT3620_AzureRTOS C ASM)

SET(CMAKE_ASM_FLAGS "-mcpu=cortex-m4")
ADD_LINK_OPTIONS(-specs=nano.specs -specs=nosys.specs)

add_compile_definitions(USE_I2C_THREADX_API)

include_directories(${CMAKE_SOURCE_DIR}
	                ${CMAKE_SOURCE_DIR}/lib
					${CMAKE_SOURCE_DIR}/lib/mt3620)

# Create executable
add_executable(${PROJECT_NAME} main.c LSM6DS3.c tx_initialize_low_level.S VectorTable.c "i2c_threadx.c" lib/GPT.c lib/GPIO.c lib/UART.c lib/Print.c lib/I2CMaster.c)

set(THREADX_ARCH "cortex_m4")
set(THREADX_TOOLCHAIN "gnu")
add_subdirectory(threadx)

target_link_libraries(${PROJECT_NAME} azrtos::threadx)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

azsphere_configure_tools(TOOLS_REVISION "20.10")

# Add MakeImage post-build command
azsphere_target_add_image_package(${PROJECT_NAME})


