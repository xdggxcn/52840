/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-07-11 17:58:23
 * @LastEditors: xdggxcn 460909495@qq.com
 * @LastEditTime: 2023-07-16 00:04:50
 * @FilePath: \central_and_peripheral_hr\src\user_driver\src\led.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * 
 */
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <nrfx.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/pm/device.h>

LOG_MODULE_REGISTER(led, LOG_LEVEL_DBG);

#define LED0_NODE DT_ALIAS(led0)
  /* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 500

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void blinky_thread(void)
{
    if (!device_is_ready(led.port)) {
        printk("Didn't find LED device referred by the LED0_NODE\n");
        return 0;
    }
	// printf("pin = %d\r\n", led_pp.pin);
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    // // gpio_pin_set_dt(&led0, 0);

	for (;;) {
		gpio_pin_set_dt(&led, 1);
		k_sleep(K_MSEC(500));
		gpio_pin_set_dt(&led, 0);
		k_sleep(K_MSEC(500));	
		// printk("xdg\n");	
	}
}
K_THREAD_DEFINE(blinky_thread_id, 1024, blinky_thread, NULL, NULL,
		NULL, 7, 0, 0);