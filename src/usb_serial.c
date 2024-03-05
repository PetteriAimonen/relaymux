// Connect USB serial port into SCPI parser

#include "usb_serial.h"
#include "board.h"
#include "scpi_commands.h"

#include <stm32f042x6.h>
#include <stm32f0xx_hal.h>
#include <usbd_cdc.h>

/******************************************
 * USB Descriptors                        *
 ******************************************/

static USBD_HandleTypeDef g_usb_dev;
static uint8_t g_usb_strbuf[USBD_MAX_STR_DESC_SIZ];

uint8_t *GetDeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    static const uint8_t desc[] =
    {
        0x12,                       /* bLength */
        USB_DESC_TYPE_DEVICE,
        0x00, 0x02,                 /* bcdUSB */
        0x02,                       /* bDeviceClass */
        0x02,                       /* bDeviceSubClass */
        0x00,                       /* bDeviceProtocol */
        USB_MAX_EP0_SIZE,           /* bMaxPacketSize */
        0x83, 0x04,                 /* idVendor */
        0x40, 0x56,                 /* idProduct */
        0x00, 0x02,                 /* bcdDevice rel. 2.00*/
        USBD_IDX_MFC_STR,           /*Index of manufacturer  string*/
        USBD_IDX_PRODUCT_STR,       /*Index of product string*/
        USBD_IDX_SERIAL_STR,        /*Index of serial number string*/
        USBD_MAX_NUM_CONFIGURATION  /*bNumConfigurations*/
    };
    
    *length = sizeof(desc);
    return (uint8_t*)&desc;
}

uint8_t *GetLangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    static const uint8_t desc[] =
    {
        USB_LEN_LANGID_STR_DESC,
        USB_DESC_TYPE_STRING,
        0x09, 0x04
    };

    *length = sizeof(desc);
    return (uint8_t*)&desc;
}

uint8_t *GetManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    USBD_GetString((uint8_t*)"devEmbedded", g_usb_strbuf, length);
    return g_usb_strbuf;
}

uint8_t *GetProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    USBD_GetString((uint8_t*)"Relay Mux", g_usb_strbuf, length);
    return g_usb_strbuf;
}

uint8_t *GetSerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    USBD_GetString((uint8_t*)board_serialnumber(), g_usb_strbuf, length);
    return g_usb_strbuf;
}

uint8_t *GetConfigurationStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    USBD_GetString((uint8_t*)"CDC-ACM", g_usb_strbuf, length);
    return g_usb_strbuf;
}

uint8_t *GetInterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    USBD_GetString((uint8_t*)"CDC-ACM", g_usb_strbuf, length);
    return g_usb_strbuf;
}

static USBD_DescriptorsTypeDef g_usb_descriptor = {
    GetDeviceDescriptor,
    GetLangIDStrDescriptor,
    GetManufacturerStrDescriptor,
    GetProductStrDescriptor,
    GetSerialStrDescriptor,
    GetConfigurationStrDescriptor,
    GetInterfaceStrDescriptor
};

/******************************************
 * CDC-ACM serial data transfer           *
 ******************************************/

#define CDC_BUFSIZE 128
static uint8_t g_cdc_rxbuf[CDC_BUFSIZE];
static volatile size_t g_cdc_rxsize;

static int8_t CDC_Init(void)
{
    USBD_CDC_SetRxBuffer(&g_usb_dev, g_cdc_rxbuf);
    USBD_CDC_SetTxBuffer(&g_usb_dev, NULL, 0);
    return USBD_OK;
}

static int8_t CDC_DeInit(void)
{
    return USBD_OK;
}

static int8_t CDC_Control(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
    return USBD_OK;
}

static int8_t CDC_Receive(uint8_t* buf, uint32_t *len)
{
    if (*len < sizeof(g_cdc_rxbuf) - g_cdc_rxsize)
    {
        USBD_CDC_SetRxBuffer(&g_usb_dev, g_cdc_rxbuf + g_cdc_rxsize);
        USBD_CDC_ReceivePacket(&g_usb_dev);
        g_cdc_rxsize += *len;
    }

    return USBD_OK;
}


static USBD_CDC_ItfTypeDef g_cdc_interface = {
    CDC_Init, CDC_DeInit, CDC_Control, CDC_Receive
};

/******************************************
 * SCPI parser                            *
 ******************************************/

static scpi_t g_scpi_context;
static char g_scpi_inbuf[SCPI_INPUT_BUFFER_LENGTH];
static char g_scpi_outbuf[SCPI_INPUT_BUFFER_LENGTH];
static size_t g_scpi_outbuf_len;
static scpi_error_t g_scpi_error_queue[SCPI_ERROR_QUEUE_SIZE];

size_t SCPI_Write(scpi_t * context, const char * data, size_t len)
{
    int pos = 0;
    while (pos < len && g_scpi_outbuf_len < sizeof(g_scpi_outbuf))
    {
        g_scpi_outbuf[g_scpi_outbuf_len++] = data[pos++];
    }

    return pos;
}

int SCPI_Error(scpi_t * context, int_fast16_t err)
{
    const char *errtxt = SCPI_ErrorTranslate(err);
    SCPI_Write(context, errtxt, strlen(errtxt));
    SCPI_Write(context, "\r\n", 2);
    return 0;
}

static scpi_interface_t g_scpi_interface = {
    .write = SCPI_Write,
    .error = SCPI_Error,
};

void usb_serial_start()
{
    USBD_Init(&g_usb_dev, &g_usb_descriptor, 0);
    USBD_RegisterClass(&g_usb_dev, &USBD_CDC);
    USBD_CDC_RegisterInterface(&g_usb_dev, &g_cdc_interface);
    USBD_Start(&g_usb_dev);

    SCPI_Init(&g_scpi_context,
              g_scpi_commands,
              &g_scpi_interface,
              scpi_units_def,
              SCPI_IDN1, SCPI_IDN2, SCPI_IDN3, SCPI_IDN4,
              g_scpi_inbuf, SCPI_INPUT_BUFFER_LENGTH,
              g_scpi_error_queue, SCPI_ERROR_QUEUE_SIZE);
}

void usb_serial_poll()
{
    size_t len = g_cdc_rxsize;
    if (len > 0)
    {
        SCPI_Input(&g_scpi_context, (const char*)g_cdc_rxbuf, g_cdc_rxsize);

        __disable_irq();
        if (g_cdc_rxsize > len)
        {
            // Got more data while was handling previous
            memcpy(g_cdc_rxbuf, g_cdc_rxbuf + len, g_cdc_rxsize - len);
        }
        g_cdc_rxsize = 0;
        __enable_irq();

        if (g_scpi_outbuf_len > 0)
        {
            USBD_CDC_SetTxBuffer(&g_usb_dev, (uint8_t*)g_scpi_outbuf, g_scpi_outbuf_len);
            USBD_CDC_TransmitPacket(&g_usb_dev);
            g_scpi_outbuf_len = 0;
        }
    }
}
