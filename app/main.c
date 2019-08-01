/*
 * @Author: xdggxcn 460909495@qq.com
 * @Date: 2023-07-11 16:03:35
 * @LastEditors: xdggxcn 460909495@qq.com
 * @LastEditTime: 2023-07-16 20:28:57
 * @FilePath: \central_and_peripheral_hr\app\main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * @Author: xdggxcn 460909495@qq.com
 * @Date: 2023-07-11 16:03:35
 * @LastEditors: xdggxcn 460909495@qq.com
 * @LastEditTime: 2023-07-16 19:16:40
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
#include <zephyr/drivers/i2c.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

// #if DT_NODE_HAS_STATUS(DT_ALIAS(arduino_i2c), okay)
#define I2C_DEV_NODE	DT_NODELABEL(arduino_i2c)
// #else
// #error "I2C 0 controller device not found"
// #endif

#define CW2015_ADDR          0x62
// #define CW2015_ADDR          0xc4
#define MODE_NORMAL             (0x0<<6)

struct bmi270_config {
    struct i2c_dt_spec i2c;
};

const struct device *i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;
static const struct device *device = NULL;

static void onButtonPressCb(buttonPressType_t type, buttonId_t id)
{
    LOG_WRN("button Pressed %d, type: %d", id, type);
}

int main(void)
{
	uint8_t buff[10];
	const struct bmi270_config *config = i2c_dev->config;
	const struct i2c_dt_spec *i2c = &config->i2c;

	if (!device_is_ready(i2c_dev)) 
	{
		printf("I2C device is not ready\n");
		return 0;
	}
	printf("Device %p name is %s\n", i2c_dev, i2c_dev->name);
	/* 1. Verify i2c_configure() */
	// if (i2c_configure(i2c_dev, i2c_cfg)) 
	// {
	// 	printf("I2C config failed\n");
	// 	return 0;
	// }	
	// printf("I2C config ok\n");
	buff[0] = MODE_NORMAL;
	i2c_reg_write_byte(i2c_dev, CW2015_ADDR, 0x0a, buff[0]);
	if (i2c_reg_read_byte(i2c_dev, CW2015_ADDR, 0, buff)) 
	{
		printf("failed to read chip revision\n");
		return 0;
	}
	printf("buf = %x\r\n", buff[0]);
	buttonsInit(&onButtonPressCb);
	for (;;) {
		k_sleep(K_MSEC(1000));	
		// printk("xdg1\n");	
	}
}