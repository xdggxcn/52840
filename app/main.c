/*
 * @Author: xdggxcn 460909495@qq.com
 * @Date: 2023-07-11 16:03:35
 * @LastEditors: xdggxcn 460909495@qq.com
 * @LastEditTime: 2023-07-15 01:58:04
 * @FilePath: \central_and_peripheral_hr\app\main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <bluetooth/services/hrs_client.h>

#include <dk_buttons_and_leds.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/settings/settings.h>

#include <zephyr/kernel.h>
#include <buttons.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static void onButtonPressCb(buttonPressType_t type, buttonId_t id)
{
    LOG_WRN("button Pressed %d, type: %d", id, type);
}

int main(void)
{
	buttonsInit(&onButtonPressCb);
	for (;;) {
		k_sleep(K_MSEC(1000));	
		printk("xdg1\n");	
	}
}