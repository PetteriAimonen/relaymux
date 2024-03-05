// STM32 USB driver configuration

#pragma once

#include <stm32f042x6.h>
#include <stm32f0xx_hal.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define USBD_MAX_NUM_INTERFACES               1U
#define USBD_MAX_NUM_CONFIGURATION            1U
#define USBD_MAX_STR_DESC_SIZ                 0x20U
#define USBD_SUPPORT_USER_STRING_DESC         1U
#define USBD_SELF_POWERED                     0U

#define USBD_CDC_INTERVAL                      2000U
#define USBD_malloc               malloc
#define USBD_free                 free
#define USBD_memset               memset
#define USBD_memcpy               memcpy

#define USBD_DEBUG_LEVEL           2U


void board_log(const char *data);
#define USBD_UsrLog(fmt, ...) board_log(fmt)
#define USBD_ErrLog(fmt, ...) board_log(fmt)
#define USBD_DbgLog(fmt, ...) board_log(fmt)
// #define USBD_DbgLog(fmt, ...) do {} while(0)
