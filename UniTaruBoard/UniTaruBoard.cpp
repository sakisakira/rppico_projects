#include <stdio.h>
#include "pico/stdlib.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;
const uint ButtonRow0 = 2;
const uint ButtonRow1 = 3;
const uint ButtonCol0 = 4;
const uint ButtonCol1 = 5;

void setup()
{
    // Initialize the standard I/O
    stdio_init_all();

    // Initialize the LED pin
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Initialize Button In/Out.
    gpio_init(ButtonRow0);
    gpio_set_dir(ButtonRow0, GPIO_OUT);
    gpio_put(ButtonRow0, 0);
    gpio_init(ButtonRow1);
    gpio_set_dir(ButtonRow1, GPIO_OUT);
    gpio_put(ButtonRow1, 0);

    gpio_init(ButtonCol0);
    gpio_set_dir(ButtonCol0, GPIO_IN);
    gpio_pull_down(ButtonCol0);
    gpio_init(ButtonCol1);
    gpio_set_dir(ButtonCol1, GPIO_IN);
    gpio_pull_down(ButtonCol1);
}

uint scan_matrix()
{
    uint result = 0;

    // Scan the first row
    gpio_put(ButtonRow0, 1);
    sleep_ms(1);
    if (gpio_get(ButtonCol0)) {
        result = 1; // Button at (0, 0)
    }
    if (gpio_get(ButtonCol1)) {
        result = 2; // Button at (0, 1)
    }
    gpio_put(ButtonRow0, 0);

    // Scan the second row
    gpio_put(ButtonRow1, 1);
    sleep_ms(1);
    if (gpio_get(ButtonCol0)) {
        result = 3; // Button at (1, 0)
    }
    if (gpio_get(ButtonCol1)) {
        result = 4; // Button at (1, 1)
    }
    gpio_put(ButtonRow1, 0);

    return result;
}

int main()
{
    setup();

    while (true) {
        const uint code = scan_matrix();
        for (uint index = 0; index < code; ++index) {
            gpio_put(LED_PIN, true);
            sleep_ms(100);
            gpio_put(LED_PIN, false);
            sleep_ms(100);
        }
        if (code > 0) {
            printf("Button pressed: %u\n", code);
        }
        gpio_put(LED_PIN, false);
        sleep_ms(100);
    }
}
