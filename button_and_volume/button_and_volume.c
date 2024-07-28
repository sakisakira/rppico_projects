#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

#include "usb_descriptors.h"

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

#define ADC_INDEX (0)
#define SAMPLE_COUNT (8)

static const uint32_t PushButtonGPIO = 16;
static const uint32_t VolumeGPIO = 26 + ADC_INDEX;

static const uint16_t DiffMax = 10;

static uint16_t s_hid_state = 0;
static uint32_t s_start_ms = 0;
static uint16_t s_past_adc_values[SAMPLE_COUNT];
static uint32_t s_sample_index = 0;
static uint32_t s_current_button = 0;
static uint16_t s_sent_volume = 0;

enum HIDState {
  HIDInitIdle,
  HIDInitSending,
  HIDIdle,
  HIDSending,
};

enum ButtonType {
  OnBoardButton = 1,
  PushButton,
  VolumeChanged,
  VolumeDown,
  VolumeUp,
  Error,
};

void initialize_volume(void);
void adjust_volume(uint16_t current_volume);
void hid_task(void);
static void send_hid_report(uint8_t report_id, uint32_t button);

int main() {
  board_init();
  tusb_init();

  gpio_init(PushButtonGPIO);
  gpio_set_dir(PushButtonGPIO, GPIO_IN);
  gpio_pull_up(PushButtonGPIO);

  adc_init();
  adc_gpio_init(VolumeGPIO);
  adc_set_clkdiv(1088.44 - 1.0);
  adc_select_input(ADC_INDEX);
  adc_set_round_robin(0x0);

  s_hid_state = HIDInitIdle;

  for (uint32_t index = 0; index < SAMPLE_COUNT; ++index)
    s_past_adc_values[index] = 0;

  while (1)
  {
    tud_task();

    if (s_hid_state == HIDInitIdle || s_hid_state == HIDInitSending) {
      initialize_volume();
    } else {
      hid_task();
    }
  }

  return 0;
}

void initialize_volume(void) {
  static const uint32_t interval_ms = 10;
  if (board_millis() - s_start_ms < interval_ms) return;
  s_start_ms += interval_ms;

  if (tud_suspended()) {
    tud_remote_wakeup();
  }
  else {
    static int count = 2;
    if (s_hid_state == HIDInitIdle) {
      if (count > 0) {
        s_current_button = OnBoardButton;
        send_hid_report(REPORT_ID_KEYBOARD, s_current_button);
        count--;
        s_hid_state = HIDInitSending;
      }
    } else if (s_hid_state == HIDInitSending) {
      s_current_button = 0;
      send_hid_report(REPORT_ID_KEYBOARD, s_current_button);
      if (count == 0) {
        s_hid_state = HIDIdle;
        s_sent_volume = 0;
      } else {
        s_hid_state = HIDInitIdle;
      }
    }
  }
}

void adjust_volume(uint16_t currnet_volume) {
return;

  currnet_volume /= 2;
  while (currnet_volume < s_sent_volume) {
    if (tud_suspended()) {
      tud_remote_wakeup();
      sleep_ms(2);
    }

    send_hid_report(REPORT_ID_CONSUMER_CONTROL, VolumeDown);
    s_sent_volume--;
    sleep_ms(2);
  }
  while (currnet_volume > s_sent_volume) {
    if (tud_suspended()) {
      tud_remote_wakeup();
      sleep_ms(2);
    }

    send_hid_report(REPORT_ID_CONSUMER_CONTROL, VolumeUp);
    s_sent_volume++;
    sleep_ms(2);
  }
}

void hid_task(void) {
  static const uint32_t interval_ms = 10;

  if (board_millis() - s_start_ms < interval_ms) return;
  s_start_ms += interval_ms;

  s_current_button = 0;
  static uint32_t prev_button = 0;
  if (board_button_read()) {
    s_current_button = OnBoardButton;
  }
  if (!gpio_get(PushButtonGPIO)) {
    if (!prev_button != PushButton)
      s_current_button = PushButton;
  }

  const uint16_t adc_value = adc_read();
  if ((adc_value & 0x8000) == 0) {
    s_past_adc_values[s_sample_index] = adc_value;
    s_sample_index = (s_sample_index + 1) % SAMPLE_COUNT;
    uint16_t prev_value = 0;
    for (uint32_t index = 0; index < SAMPLE_COUNT; ++index)
      prev_value += s_past_adc_values[index];
    prev_value /= SAMPLE_COUNT;
    const uint16_t scaled_volume = prev_value * 1.f / 4096 * 100;
    adjust_volume(scaled_volume);
  }
  else
  {
    s_current_button = Error;
  }
  prev_button = s_current_button;

  if (tud_suspended() && s_current_button)
  {
    tud_remote_wakeup();
  } else {
    send_hid_report(REPORT_ID_KEYBOARD, s_current_button);
  }
}

static void send_hid_report(uint8_t report_id, uint32_t button) {
  if (!tud_hid_ready()) return;
  
  switch (report_id) {
  case REPORT_ID_KEYBOARD:
  {
    static bool has_keyboard_key = false;
    if (button) {
      uint8_t keycode[6] = {0};
      if (button == OnBoardButton)
        keycode[0] = HID_KEY_A;
      else if (button == Error)
        keycode[0] = HID_KEY_E;

      tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
      has_keyboard_key = true;
    } else {
      if (has_keyboard_key) 
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        has_keyboard_key = false;
    }
    break;
  }
  
  case REPORT_ID_CONSUMER_CONTROL:
  {
    static bool has_consumer_key = false;

    if (button) {
      if (button == PushButton) {
        uint16_t play_pause = HID_USAGE_CONSUMER_PLAY_PAUSE;
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &play_pause, 2);
        sleep_ms(500);
        has_consumer_key = true;
      } else if (button == VolumeUp) {
        uint16_t volume_up = HID_USAGE_CONSUMER_VOLUME_INCREMENT;
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &volume_up, 2);
        sleep_ms(200);
        has_consumer_key = true;
      } else if (button == VolumeDown) {
        uint16_t volume_down = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &volume_down, 2);
        sleep_ms(200);
        has_consumer_key = true;
      } else {
        uint16_t empty_key = 0;
        if (has_consumer_key) 
          tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_key, 2);
        has_consumer_key = false;
      }
    }
    break;
  }

  case REPORT_ID_MOUSE:
  {
    int8_t const delta = 0;
    tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta, delta, 0, 0);
    break;
  }
  case REPORT_ID_GAMEPAD: 
  {
    break;
  }
  }
}

// callbacks

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance;
  (void) len;

  uint8_t next_report_id = report[0] + 1u;

  if (next_report_id < REPORT_ID_COUNT)
  {
    send_hid_report(next_report_id, s_current_button);
  }
}

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
