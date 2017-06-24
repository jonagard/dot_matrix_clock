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
#define TIME_SET_PIN 3
#define ALARM_SET_PIN 4
#define HOUR_SET_PIN 5
#define MIN_SET_PIN 6
#define SECONDS_PIN 7

// RTC interrupt pin
const byte rtcTimerIntPin = 2;

// Define the number of devices we have in the chain and the hardware interface
#define  MAX_DEVICES 4
// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(CS_PIN, MAX_DEVICES);

RTC_DS3231 rtc;
// Address of the RTC
#define DS3231_I2C_ADDRESS 0x68

/*
 * Using mx.setChar() for ease to print one char at a time.  This means
 * it gets a bit kludgey converting the integer time to a char array.
 * Will have to convert each part to a string first and then convert
 * the string to an array.  That means each part needs an integer, string,
 * and character array representation.
 *
 * I could use one function pretty easily to convert second and minute
 * and get rid of a couple of these. But then I'd have to pass variables
 * around.  I like how easy these are to call this way.
 */
uint8_t second_int = 0;
String second_str;
char second_char[3];

uint8_t minute_int = 0;
String minute_str;
char minute_char[3];

uint8_t hour_int = 0;
String hour_str;
char hour_char[3];

uint8_t alarm_minute_int = 0;
String alarm_minute_str;
char alarm_minute_char[3];

uint8_t alarm_hour_int = 0;
String alarm_hour_str;
char alarm_hour_char[3];

// for am, set period = 0
// for pm, set period = 1
int period = 0;
int alarm_period = 0;

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

unsigned long debounce_delay = 50;    // the debounce time; increase if the output flickers
unsigned long button_hold_time = 150;

// hour-setting variables
int hour_set_state = 0;
int last_hour_set_state = LOW;
unsigned long last_hour_debounce_time = 0;
int setting_hour = 0;
int setting_alarm_hour = 0;
long hour_up_time;
long hour_down_time;
boolean ignore_hour_up = false;

// minute-setting variables
int min_set_state = 0;
int last_min_set_state = LOW;
unsigned long last_min_debounce_time = 0;
int setting_min = 0;
int setting_alarm_min = 0;
long min_up_time;
long min_down_time;
boolean ignore_min_up = false;

int time_set_state = 0;
int alarm_set_state = 0;
int start_seconds = 0;
int writing_time= 0;
int time_initialized = 0;
int update_flag = 0;
unsigned long start_sep;

/*
 * increment a value (a, which is an hour or minute) and wrap
 * if it is greater than the limit (b, which is 23 or 59)
 * since there isn't something to do in both the if and else
 * part of the statement, setup the logic so value is set to
 * zero in the else (I can leave the if blank but not the else)
 */
#define INCR(a, b) ((++a <= b) ? :a = 0)

/*
 * build the string version of the minute or second and put
 * it in the char array
 *
 * a - min or second char array
 * b - min or second string
 * c - min or second integer
 *
 * if it is 0-9, add the leading zero
 */
#define BLD_MIN_SEC_STR(a, b, c) \
  ((c < 10) ? (b = '0' + String(c)) : (b = String(c))); \
  b.toCharArray(a, 3);

/*
 * build the string version for the hour and put it in the
 * char array
 *
 * a - hour char array
 * b - hour string
 * c - hour integer
 *
 * If the hour is 0, we want to print 12.  Do so by making
 * the adjustment var -12, so that (0 - (-12)) == 12.
 * Otherwise if time is greater than 12 adjust the time
 * by 12 since we aren't doing 24-hour time.
 *
 * Do some work here to deal with a single-length hour (1-9)
 * rather than dealing with it in the printing.  Single-length looks
 * like:
 * ['1']['\0']
 * So shift things over and put a literal 0 in the zero position:
 * [0]['1']['\0]
 * That literal 0 won't print out five columns, so I still have to
 * call CLEAR_DISP().  Mostly just keeping something in it in case.
 */
#define BLD_HOUR_STR(a, b, c) \
  int adjustment = 0; \
  if (c == 0) \
    adjustment = -12; \
  else if (c > 12) \
    adjustment = 12; \
  b = String(c - adjustment); \
  b.toCharArray(a, 3); \
  if (b.length() == 1) \
  { \
    a[2] = '\0'; \
    a[1] = a[0]; \
    a[0] = 0; \
  }

/*
 * print a minute value to the display
 *
 * a - tens position to print to
 * b - tens value
 * c - ones position to print to
 * d - ones value
 */
#define PRINT_MIN(a, b, c, d) \
  if (!display_seconds) \
  { \
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF); \
    mx.setChar(a, b); \
    mx.setChar(c, d); \
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON); \
  }

/*
 * print an hour value to the display, and the period
 *
 * a - tens position to print to
 * b - tens value
 * c - ones position to print to
 * d - ones value
 * e - hour int value
 * f - period we work on
 *
 * Clear the display first as the second part of dealing with single-length
 * hours.
 *
 * Also print the period here, will work for an alarm or time.
 */
#define PRINT_HOUR(a, b, c, d, e, f) \
  if (!display_seconds) \
  { \
    CLEAR_DISP(); \
    f = 0; \
    if (e > 11) \
      f = 1; \
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF); \
    mx.setBuffer(period_pos, 3, ampm[f]); \
    mx.setChar(a, b); \
    mx.setChar(c, d); \
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON); \
  }

/*
 * Macro to clear positions on the display.  For hours I just need to
 * clear the tens position.  But when printing seconds I need to clear
 * the period and the ones hour position.  So try using this
 * one-size-fits-all approach as I doubt there will be any negative
 * fallout.
 */
#define CLEAR_DISP() \
  mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF); \
  mx.setBuffer(period_pos, 3, blank); \
  mx.setBuffer(hour_tens_pos, 5, blank); \
  mx.setBuffer(hour_ones_pos, 5, blank); \
  mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);

