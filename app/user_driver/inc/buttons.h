/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2023-07-11 17:58:49
 * @LastEditors: xdggxcn 460909495@qq.com
 * @LastEditTime: 2023-07-15 01:16:35
 * @FilePath: \central_and_peripheral_hr\src\user_driver\inc\led.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef BUTTONS_H__
#define BUTTONS_H__

#include <zephyr/kernel.h>

/**
 * @brief Type of button event
 */
typedef enum buttonPressType_t {
    BUTTONS_SHORT_PRESS,
    BUTTONS_LONG_PRESS
} buttonPressType_t;

/**
 * @brief Button id
 */
typedef enum buttonId_t {
    BUTTON_LINK_KEY,
    BUTTON_USER_KEY,
    BUTTON_END,
} buttonId_t;

typedef void(*buttonHandlerCallback_t)(buttonPressType_t type, buttonId_t id);

/**
 * @brief   Init Button press handler
 *
 * @param   handler          Pointer to callback function for button events.
 */
void buttonsInit(buttonHandlerCallback_t handler);

/**
 * @brief   Read button state
 * @param   button          Button id to read
 *
 * @return  1 if button is pressed, 0 if not
*/
int button_read(buttonId_t button);

/**
 * @brief   Set fake button press. Use to allow emulating button presses.
 *
 * @param   button          Button id
 * @param   pressed         True if button is pressed, false if not
*/
int button_set_fake_press(buttonId_t button, bool pressed);


#endif