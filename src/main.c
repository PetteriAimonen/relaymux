#include "board.h"
#include "usb_serial.h"
#include <stm32f0xx_hal.h>

void poll_buttons()
{
    uint32_t buttons = read_buttons();

    if (buttons & BTN_CLEAR)
    {
        // Open all relays
        open_relays(RELAY_MASK);
    }
    else if (buttons & BTN_CYCLE)
    {
        // Activate next relay in sequence
        uint32_t relays = relays_get_state();
        relays = (relays << 1) & RELAY_MASK;
        if (relays == 0)
            relays = 1;
        else if ((relays & (relays - 1)) != 0)
            relays = 1;

        open_relays(~relays);
        close_relays(relays);
    }
}

int main()
{
    board_init();
    STATUS_LED_ON();
    set_relay_pwr(true);

    usb_serial_start();

    while (1)
    {
        poll_buttons();
        usb_serial_poll();
    }
}
