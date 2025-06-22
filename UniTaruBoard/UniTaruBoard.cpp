#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/pico-dfPlayerMini/dfPlayer/dfPlayer.h"
#include <vector>

const uint LED_PIN = PICO_DEFAULT_LED_PIN;
const uint ButtonRow0 = 2;
const uint ButtonRow1 = 3;
const uint ButtonCol0 = 4;
const uint ButtonCol1 = 5;

void displayMessage(const std::vector<uint8_t>& data)
{
    if (data.empty()) {
        printf("No data received.\n");
        return;
    }

    printf("Received data: ");
    for (const auto& byte : data) {
        printf("%02x ", byte);
    }
    printf("\n");
}

class DfPlayerPicoSd : public DfPlayer<DfPlayerPicoSd>
{
public:
    DfPlayerPicoSd()
    {
        // Initialize UART 0
        uart_init(uart0, 9600);

        // Set the GPIO pin mux to the UART - 0 is TX, 1 is RX
        gpio_set_function(0, GPIO_FUNC_UART);
        gpio_set_function(1, GPIO_FUNC_UART);

        sendCmd(dfPlayer::SPECIFY_PLAYBACK_SRC, 0x0002); // Set to use only SD card.
        displayMessage(uartRead());
    }

    inline void uartSend(uint8_t* a_cmd)
    {
        uart_write_blocking(uart0, a_cmd, SERIAL_CMD_SIZE);
    }

    inline std::vector<uint8_t> uartRead()
    {
        std::vector<uint8_t> data;
        if (!uart_is_readable(uart0)) return data;

        data.resize(SERIAL_CMD_SIZE);
        uart_read_blocking(uart0, data.data(), SERIAL_CMD_SIZE);
        return data;
    }

    uint getSoundCount()
    {
        sendCmd(0x47, 0x0000);
        while (!uart_is_readable(uart0)) {
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
        constexpr uint8_t folder_index = 1;
        sendCmd(dfPlayer::cmd::SPECIFY_FOLDER_PLAYBACK, 
            folder_index << 8 | soundNumber);
    }

    void setVolume(uint8_t volume)
    {
        if (volume > 31) {
            volume = 31; // Clip to maximum volume
        }
        sendCmd(dfPlayer::cmd::SPECIFY_VOL, volume);
    }
};

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
    printf("setup done.\n");
    sleep_ms(1000); // Allow time for setup to complete

    DfPlayerPicoSd player;
    printf("Player initialized.\n");

    player.setVolume(25); // Set initial volume
    displayMessage(player.uartRead());
    printf("Player set volume.\n");

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
            player.playSound(code); // Adjust for zero-based index
        }

        // Display results of player operations.
        const std::vector<uint8_t> data = player.uartRead();
        if (!data.empty())
            displayMessage(data);

        gpio_put(LED_PIN, false);
        sleep_ms(100);
    }
}
