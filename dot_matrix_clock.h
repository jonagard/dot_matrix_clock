#include <MD_MAX72xx.h>
#include "RTClib.h"

/*
 * Pins
 */

// dot matrix display
#define CLK_PIN   13  // or SCK
#define DATA_PIN  11  // or MOSI
#define CS_PIN    10  // or SS

// buttons
#define BRIGHTNESS_PIN 2
#define TIME_SET_PIN 3
#define ALARM_SET_PIN 4
#define HOUR_SET_PIN 5
#define MIN_SET_PIN 6
#define SECONDS_PIN 7
#define BUZZER_PIN 8
#define ALARM_PWR_PIN 9
#define SNOOZE_PIN 12

/*
 * RTC pins
 * defined as addresses in rtclib.h
 * SDA = A4
 * SCL = A5
 */

// Define the number of devices we have in the chain and the hardware interface
#define  MAX_DEVICES 4
// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(MD_MAX72XX::FC16_HW, CS_PIN, MAX_DEVICES);

RTC_DS3231 rtc;
// Address of the RTC
#define DS3231_I2C_ADDRESS 0x68

uint8_t second_int = 0;
uint8_t minute_int = 0;
uint8_t hour_int = 0;

uint8_t alarm_minute_int = 0;
uint8_t alarm_hour_int = 0;

// for am, set period = 0
// for pm, set period = 1
int period = 0;
int alarm_period = 0;

#define NUM_WIDTH 5

/*
 * bitmaps
 */

// bitmap for "A" (AM) and "P" (PM) indicator
uint8_t ampm[2][3] =
{
  // 'A'
  {
    0b01111100,
    0b00010100,
    0b01111100
  },
  // 'P'
  {
    0b01111100,
    0b00010100,
    0b00011100
  }
};

// bitmap for "alarm on" indicator
uint8_t bell[3] =
{
  0b00000101,
  0b00000011,
  0b00000111
};

// bitmap for blank clock spots
uint8_t blank[5] =
{
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000,
  0b00000000
};

/*
 * Stole the numbers 0-9 from the MX library, but modified '1' to be five
 * columns wide instead of 3.  Because of that, it's easier to keep all 10
 * digits here instead of trying to carry along a modified MX lib just to
 * change one digit.
 */
uint8_t numbers[10][NUM_WIDTH] =
{
  {
    62, 81, 73, 69, 62    // 48 - '0'
  },
  {
    64, 66, 127, 64, 64   // 49 - '1'
  },
  {
    114, 73, 73, 73, 70   // 50 - '2'
  },
  {
    33, 65, 73, 77, 51    // 51 - '3'
  },
  {
    24, 20, 18, 127, 16   // 52 - '4'
  },
  {
    39, 69, 69, 69, 57    // 53 - '5'
  },
  {
    60, 74, 73, 73, 49    // 54 - '6'
  },
  {
    65, 33, 17, 9, 7      // 55 - '7'
  },
  {
    54, 73, 73, 73, 54    // 56 - '8'
  },
  {
    70, 73, 73, 41, 30    // 57 - '9'
  }
};

// positions for each number in the clock output
int hour_tens_pos = 28;
int hour_ones_pos = 22;
int min_tens_pos = 14;
int min_ones_pos = 8;
int sep_pos = 16;
int period_pos = 2;

// variables for displaying seconds
int seconds_disp_state = 0;
int display_seconds = 0;

// the debounce time; increase if the output flickers
unsigned long debounce_delay = 100;
unsigned long button_hold_time = 350;

// hour-setting variables
int hour_set_state = 0;
int last_hour_set_state = LOW;
int setting_hour = 0;
int setting_alarm_hour = 0;
long hour_up_time;
long hour_down_time;
boolean ignore_hour_up = false;

// minute-setting variables
int min_set_state = 0;
int last_min_set_state = LOW;
int setting_min = 0;
int setting_alarm_min = 0;
long min_up_time;
long min_down_time;
boolean ignore_min_up = false;

// brightness-setting variables
int brightness_btn_state;
int default_brightness_index = 1;
int brightness_index = 0;
int brightness_options [5] = {1, 3, 5, 9, 13};
int last_brightness_btn_state = LOW;
long brightness_up_time;
long brightness_down_time;
boolean ignore_brightness_up = false;

// alarm and snooze variables
int alarm_set_state = 0;
int alarm_btn_state;
int alarm_pwr_state;
int alarming = 0;
int snooze_btn_state;
unsigned long snooze_btn_debounce_time = 0;
int snooze_state = 0;
uint8_t status_register;
int alarm_duration = 0;
boolean alarm_check_toggle = false;
int beep_count = 0;

// various time variables
int time_set_state = 0;
int start_seconds = 0;
int writing_time= 0;
int time_initialized = 0;
int update_flag = 0;
unsigned long start_sep = 0;
int blink_sep_enable = 1;

/*
 * increment a value (a, which is an hour or minute) and wrap if it is greater
 * than the limit (b, which is 23 or 59) since there isn't something to do in
 * both the if and else part of the statement, setup the logic so value is set
 * to zero in the else (I can leave the if blank but not the else)
 */
#define INCR(a, b) ((++a <= b) ? :a = 0)

/*
 * print a minute value to the display
 *
 * a - tens position to print to
 * b - ones position to print to
 * c - minute int value
 */
#define PRINT_MIN(a, b, c) \
  if (!display_seconds) \
  { \
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF); \
    mx.setBuffer(a, NUM_WIDTH, numbers[c/10]); \
    mx.setBuffer(b, NUM_WIDTH, numbers[c%10]); \
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON); \
  }

/*
 * print an hour value to the display, and the period
 *
 * a - tens position to print to
 * b - ones position to print to
 * c - hour int value
 * d - period we work on
 *
 * Clear the display first for single-length hours, so there is nothing in the
 * tens place.
 *
 * Convert the hour int, which is in 24-hour format, to 12-hour format before
 * printing.
 *
 * Also print the period here, will work for an alarm or time.
 */
#define PRINT_HOUR(a, b, c, d) \
  int adjustment = 0; \
  int hour_in_12; \
  if (c == 0) \
    adjustment = -12; \
  else if (c > 12) \
    adjustment = 12; \
  hour_in_12 = c - adjustment; \
  if (!display_seconds) \
  { \
    CLEAR_DISP(); \
    d = 0; \
    if (c > 11) \
      d = 1; \
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF); \
    mx.setBuffer(period_pos, 3, ampm[d]); \
    if (hour_in_12/10) \
      mx.setBuffer(a, NUM_WIDTH, numbers[hour_in_12/10]); \
    mx.setBuffer(b, NUM_WIDTH, numbers[hour_in_12%10]); \
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON); \
  }

/*
 * Macro to clear positions on the display.  For hours I just need to clear the
 * tens position.  But when printing seconds I need to clear the period and the
 * ones hour position.  So try using this one-size-fits-all approach as I doubt
 * there will be any negative fallout.
 */
#define CLEAR_DISP() \
  mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF); \
  mx.setBuffer(period_pos, 3, blank); \
  mx.setBuffer(hour_tens_pos, NUM_WIDTH, blank); \
  mx.setBuffer(hour_ones_pos, NUM_WIDTH, blank); \
  mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
