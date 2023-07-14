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

set(SRC_LIST
        ${user_driver_dir}/src/led.c
        ${user_driver_dir}/src/buttons.c
         )


# NORDIC SDK APP START
target_sources(app PRIVATE
  app/main.c
  ${SRC_LIST}
)

# NORDIC SDK APP END
