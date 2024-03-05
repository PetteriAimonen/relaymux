#include "board.h"
#include <stm32f0xx_ll_utils.h>

static void buttons_poll();

void board_init()
{
    HAL_Init();

    // System clock comes from HSI48
    HAL_RCC_OscConfig(&(RCC_OscInitTypeDef){
        .OscillatorType = RCC_OSCILLATORTYPE_HSI48,
        .HSI48State = RCC_HSI48_ON
    });
    HAL_RCC_ClockConfig(&(RCC_ClkInitTypeDef){
        .ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1,
        .SYSCLKSource = RCC_SYSCLKSOURCE_HSI48,
        .AHBCLKDivider = RCC_SYSCLK_DIV1,
        .APB1CLKDivider = RCC_HCLK_DIV2
    }, FLASH_LATENCY_1);
    
    // Enable needed peripherals
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_CRS_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    // Enable clock recovery from USB
    CRS->CR |= CRS_CR_CEN | CRS_CR_AUTOTRIMEN;

    // Initialize relay outputs
    for (int i = 0; i < RELAY_COUNT; i++)
    {
        uint32_t pin = (1 << (i + RELAY_PIN_SHIFT));
        HAL_GPIO_WritePin(RELAY_PORT, pin, 0);
        HAL_GPIO_Init(RELAY_PORT, &(GPIO_InitTypeDef){
            .Pin = pin,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_LOW,
        });
    }

    // Button inputs (active high)
    HAL_GPIO_Init(CYCLE_BTN_PORT, &(GPIO_InitTypeDef){
        .Pin = CYCLE_BTN_PIN,
        .Mode = GPIO_MODE_INPUT,
        .Pull = GPIO_PULLDOWN
    });
    HAL_GPIO_Init(CLEAR_BTN_PORT, &(GPIO_InitTypeDef){
        .Pin = CLEAR_BTN_PIN,
        .Mode = GPIO_MODE_INPUT,
        .Pull = GPIO_PULLDOWN
    });

    // Status LED (active low)
    HAL_GPIO_Init(STATUS_LED_PORT, &(GPIO_InitTypeDef){
        .Pin = STATUS_LED_PIN,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_LOW,
    });

    // Relay power enable (active low)
    HAL_GPIO_WritePin(PWR_EN_PORT, PWR_EN_PIN, GPIO_PIN_SET);
    HAL_GPIO_Init(PWR_EN_PORT, &(GPIO_InitTypeDef){
        .Pin = PWR_EN_PIN,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_LOW,
    });
}

void SysTick_Handler()
{
    HAL_IncTick();
    buttons_poll();
}

void HardFault_Handler()
{
    // Turn off all relays
    RELAY_PORT->BRR = RELAY_MASK << RELAY_PIN_SHIFT;
    PWR_EN_PORT->BSRR = PWR_EN_PIN;

    // Blink status LED rapidly
    while (1)
    {
        STATUS_LED_ON();
        LL_mDelay(100);
        STATUS_LED_OFF();
        LL_mDelay(100);
    }
}

void set_relay_pwr(bool enable)
{
    if (enable)
    {
        PWR_EN_PORT->BRR = PWR_EN_PIN;
    }
    else
    {
        PWR_EN_PORT->BSRR = PWR_EN_PIN;
    }
}

void close_relays(uint32_t channels)
{
    RELAY_PORT->BSRR = (channels & RELAY_MASK) << RELAY_PIN_SHIFT;
    HAL_Delay(RELAY_OPERATE_DELAY_MS);
}

void open_relays(uint32_t channels)
{
    RELAY_PORT->BRR = (channels & RELAY_MASK) << RELAY_PIN_SHIFT;
    HAL_Delay(RELAY_RELEASE_DELAY_MS);
}

uint32_t relays_get_state()
{
    return (RELAY_PORT->ODR >> RELAY_PIN_SHIFT) & RELAY_MASK;
}

static volatile uint32_t g_prev_button_press;
static volatile uint32_t g_buttons_pressed;

static void buttons_poll()
{
    uint32_t buttons = 0;
    if (CYCLE_BTN_PORT->IDR & CYCLE_BTN_PIN) buttons |= BTN_CYCLE;
    if (CLEAR_BTN_PORT->IDR & CLEAR_BTN_PIN) buttons |= BTN_CLEAR;

    if (buttons)
    {
        g_prev_button_press = HAL_GetTick();
        g_buttons_pressed |= buttons;
    }
}

uint32_t read_buttons()
{
    if (g_buttons_pressed != 0 &&
        (int32_t)(HAL_GetTick() - g_prev_button_press) > BTN_DEBOUNCE_TIME_MS) 
    {
        uint32_t result = g_buttons_pressed;
        g_buttons_pressed = 0;
        return result;
    }

    return 0;
}

uint8_t g_logbuf[256];
uint32_t g_logidx;

void board_log(const char *data)
{
    while (*data)
    {
        g_logbuf[(g_logidx++) % sizeof(g_logbuf)] = *data++;
    }
}

const char *board_serialnumber()
{
    static char serialbuf[9] = {0};
    
    if (serialbuf[0] == 0)
    {
        // Initialize serial number on first call
        uint32_t serial = 0;
        for (int i = 0; i < 3; i++)
        {
            serial ^= *(uint32_t*)(UID_BASE + i * 4);
            serial ^= serial << 13;
            serial ^= serial >> 17;
            serial ^= serial << 5;
        }
        
        const char nibble[16] = "0123456789ABCDEF";
        for (int i = 0; i < 8; i++)
        {
            serialbuf[i] = nibble[(serial >> (i * 4)) & 0x0F];
        }

        serialbuf[8] = '\0';
    }

    return serialbuf;
}
