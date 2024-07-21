#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

#include "usb_descriptors.h"

#include "hardware/gpio.h"
#include "pico/binary_info.h"

static const uint32_t PushButtonGPIO = 16;

enum ButtonType {
  OnBoardButton = 1,
  PushButton,
};

void hid_task(void);
static void send_hid_report(uint32_t button);

int main() {
  board_init();
  tusb_init();

  gpio_init(PushButtonGPIO);
  gpio_set_dir(PushButtonGPIO, GPIO_IN);
  gpio_pull_up(PushButtonGPIO);

  while (1)
  {
    tud_task();

    hid_task();
  }

  return 0;
}

void hid_task(void) {
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if (board_millis() - start_ms < interval_ms) return;
  start_ms += interval_ms;

  uint32_t button = 0;
  if (board_button_read())
    button = OnBoardButton;
  if (!gpio_get(PushButtonGPIO))
    button = PushButton;

  if (tud_suspended() && button)
  {
    tud_remote_wakeup();
  } else {
    send_hid_report(button);
  }
}

static void send_hid_report(uint32_t button) {
  if (!tud_hid_ready()) return;
  
  static bool has_keyboard_key = false;
  if (button) {
    uint8_t keycode[6] = {0};
    if (button == OnBoardButton)
      keycode[0] = HID_KEY_A;
    else if (button == PushButton)
      keycode[0] = HID_KEY_B;

    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0 , keycode);
    has_keyboard_key = true;
  } else {
    if (has_keyboard_key) 
      tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
      has_keyboard_key = false;
  }
}

// callback
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) 
{
    // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) bufsize;
}
