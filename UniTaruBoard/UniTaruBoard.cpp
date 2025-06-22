#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/pico-dfPlayerMini/dfPlayer/dfPlayer.h"

#include <vector>

class DfPlayerPicoSd : public DfPlayer<DfPlayerPicoSd>
{
public:
    DfPlayerPicoSd()
    {
        // Initialize UART 1
        uart_init(uart1, 9600);

        // Set the GPIO pin mux to the UART - 8 is TX, 9 is RX
        gpio_set_function(8, GPIO_FUNC_UART);
        gpio_set_function(9, GPIO_FUNC_UART);

        sendCmd(0x3f, 0x0002); // Set to use only SD card.
    }

    inline void uartSend(uint8_t* a_cmd)
    {
        uart_write_blocking(uart1, a_cmd, SERIAL_CMD_SIZE);
    }

    inline std::vector<uint8_t> uartRead()
    {
        std::vector<uint8_t> data;
        if (!uart_is_readable(uart1)) return data;

        data.resize(sizeof(SERIAL_CMD_SIZE));
        uart_read_blocking(uart1, data.data(), SERIAL_CMD_SIZE);
        return data;
    }

    uint getSoundCount()
    {
        sendCmd(0x47, 0x0000);
        while (!uart_is_readable(uart1)) {
            sleep_ms(100); // Wait for the response
        }
        const std::vector<uint8_t> response = uartRead();
        if (response.size() < SERIAL_CMD_SIZE) {
            printf("Not enough data received\n");
            return 0; // Not enough data received
        }
        return response[dfPlayer::serialCommFormat::PARA1] << 8 | response[dfPlayer::serialCommFormat::PARA2];
    }

    void playSound(uint8_t soundNumber)
    {
        constexpr uint8_t folder_index = 0;
        sendCmd(dfPlayer::cmd::SPECIFY_FOLDER_PLAYBACK, 
            folder_index << 8 | soundNumber);
    }

private:
    static constexpr uint8_t SERIAL_CMD_SIZE = 10;
};

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

    DfPlayerPicoSd player;

    while (true) {
        // Key input handling.
        const uint code = scan_matrix();
        for (uint index = 0; index < code; ++index) {
            gpio_put(LED_PIN, true);
            sleep_ms(100);
            gpio_put(LED_PIN, false);
            sleep_ms(100);
        }
        if (code > 0) {
            printf("Button pressed: %u\n", code);
            player.playSound(code);
        }

        // Display results of player operations.
        const std::vector<uint8_t> data = player.uartRead();
        if (!data.empty()) {
            printf("Received data: ");
            for (const auto& byte : data) {
                printf("%02x ", byte);
            }
            printf("\n");
        }

        gpio_put(LED_PIN, false);
        sleep_ms(100);
    }
}
