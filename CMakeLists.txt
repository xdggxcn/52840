#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(central_and_peripheral_hr)

set(curr_dir ${CMAKE_CURRENT_SOURCE_DIR})
set(user_driver_dir ${curr_dir}/app/user_driver)
# message("xdg ${user_driver_dir}")
include_directories(
    ${user_driver_dir}/inc
)

include_directories(
  app/demo_kernel_code/inc
  app/gh3221lib
)

set(SRC_LIST
        ${user_driver_dir}/src/led.c
        ${user_driver_dir}/src/buttons.c
        app/demo_kernel_code/src/gh3x2x_demo_hook.c
        app/demo_kernel_code/src/gh3x2x_demo_protocol.c
        app/demo_kernel_code/src/gh3x2x_demo_reg_array.c
        app/demo_kernel_code/src/gh3x2x_demo_soft_adt.c
        app/demo_kernel_code/src/gh3x2x_demo_user.c
        app/demo_kernel_code/src/gh3x2x_demo.c        
        app/gh3221lib/libgh3x2x_drv_cortexM4l_band_gcc_hard_agc.a
        app/gh3221lib/libgh3x2x_drv_cortexM4l_band_gcc_hard_ecg.a
        app/gh3221lib/libgh3x2x_drv_cortexM4l_band_gcc_hard_kernel.a
        app/gh3221lib/libgh3x2x_drv_cortexM4l_band_gcc_hard_zip.a        
         )


# NORDIC SDK APP START
target_sources(app PRIVATE
  app/main.c
  ${SRC_LIST}
)

# NORDIC SDK APP END
