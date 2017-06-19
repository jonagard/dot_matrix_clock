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
#define SECONDS_PIN 7
#define TIME_SET_PIN 3
#define HOUR_SET_PIN 5
#define MIN_SET_PIN 6

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

// for am, set period = 0
// for pm, set period = 1
int period = 0;

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

int time_set_state = 0;
unsigned long debounce_delay = 50;    // the debounce time; increase if the output flickers
unsigned long button_hold_time = 150;

// hour-setting variables
int hour_set_state = 0;
int last_hour_set_state = LOW;
unsigned long last_hour_debounce_time = 0;
int setting_hour = 0;
long hour_up_time;
long hour_down_time;
boolean ignore_hour_up = false;

// minute-setting variables
int min_set_state = 0;
int last_min_set_state = LOW;
unsigned long last_min_debounce_time = 0;
int setting_min = 0;
long min_up_time;
long min_down_time;
boolean ignore_min_up = false;

int start_seconds = 0;
int writing_time= 0;
int time_initialized = 0;
int update_flag = 0;

