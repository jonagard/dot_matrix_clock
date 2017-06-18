#include <MD_MAX72xx.h>
#include "TimerOne.h"
#include "RTClib.h"

// Define the number of devices we have in the chain and the hardware interface
#define	MAX_DEVICES	4

// dot matrix display
#define	CLK_PIN		13  // or SCK
#define	DATA_PIN	11  // or MOSI
#define	CS_PIN		10  // or SS

// buttons
#define SECONDS_PIN 7
#define TIME_SET_PIN 3
#define HOUR_SET_PIN 5
#define MIN_SET_PIN 6

// RTC interrupt pin
const byte rtcTimerIntPin = 2;

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(CS_PIN, MAX_DEVICES);

RTC_DS3231 rtc;

/*
 * Using mx.setChar() for ease to print one char at a time.  This means 
 * it gets a bit kludgey converting the integer time to a char array.
 * Will have to convert each part to a string first and then convert 
 * the string to an array.  That means each part needs an integer, string,
 * and character array representation.
 * 
 * I could simply store the time as characters and increment this way, but
 * it's easier for me to do the work of setting the time thinking in integers.
 */
int second_int = 0;
String second_str;
char second_char[3];

int minute_int = -1;
String minute_str;
char minute_char[3];

int hour_int = -1;
String hour_str;
char hour_char[3];

int update_flag = 0;

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

void setTime()
{
  while (time_set_state == HIGH)
  {
    /*
     * Read hour button
     * 
     * Code for catching the hold came from here:
     * http://jmsarduino.blogspot.com/2009/05/click-for-press-and-hold-for-b.html
     * 
     */
    int hour_set_state = digitalRead(HOUR_SET_PIN);
    
    // Test for button pressed and store the down time
    if (hour_set_state == HIGH && last_hour_set_state == LOW &&
        (millis() - hour_up_time) > long(debounce_delay)) {
      hour_down_time = millis();
    }

    // Test for button release and store the up time
    if (hour_set_state == LOW && last_hour_set_state == HIGH &&
        (millis() - hour_down_time) > long(debounce_delay))
    {
      if (ignore_hour_up == false)
      {
          setting_hour = 1;
          incrHour();
          buildHour();
          printHour();
      }
      else
      {
        ignore_hour_up = false;
      }
      hour_down_time = millis();
    }

    // Test for button held down for longer than the hold time
    if (hour_set_state == HIGH &&
        (millis() - hour_down_time) > button_hold_time)
    {
      setting_hour = 1;
      incrHour();
      buildHour();
      printHour();
      ignore_hour_up = true;
      hour_down_time = millis();
    }

    /*
     * Read minute button
     */
    int min_set_state = digitalRead(MIN_SET_PIN);

    // Test for button pressed and store the down time
    if (min_set_state == HIGH && last_min_set_state == LOW &&
        (millis() - min_up_time) > long(debounce_delay)) {
      min_down_time = millis();
    }

    // Test for button release and store the up time
    if (min_set_state == LOW && last_min_set_state == HIGH &&
        (millis() - min_down_time) > long(debounce_delay))
    {
      if (ignore_min_up == false)
      {
          setting_min = 1;
          incrMinute();
          buildMinute();
          printMinute();
      }
      else
      {
        ignore_min_up = false;
      }
      min_down_time = millis();
    }

    // Test for button held down for longer than the hold time
    if (min_set_state == HIGH &&
        (millis() - min_down_time) > button_hold_time)
    {
      setting_min = 1;
      incrMinute();
      buildMinute();
      printMinute();
      ignore_min_up = true;
      min_down_time = millis();
    }

    last_hour_set_state = hour_set_state;
    last_min_set_state = min_set_state;
    time_set_state = digitalRead(TIME_SET_PIN);
  } 

  if (setting_hour || setting_min)
    writeNewTime();
}

void incrMinute()
{
  minute_int++;
  if (minute_int > 59)
  {
    minute_int = 0;
  }
}

void incrHour()
{
   hour_int++;
   if (hour_int > 23)
  {
    hour_int = 0; 
  }
}

void buildSecond()
{
  if (second_int < 10)
    // process 0-9, adding '0' in the tens position
    second_str = '0' + String(second_int);
  else  
    // process 10-59
    second_str = String(second_int);
  second_str.toCharArray(second_char, 3); 
}

void buildMinute()
{
  if (minute_int < 10)
    // process 0-9, adding '0' in the tens position
    minute_str = '0' + String(minute_int);
  else
    // process 10-59
    minute_str = String(minute_int);
  minute_str.toCharArray(minute_char, 3);
}

void buildHour()
{  
  // 24 hour adjustment
  int adjustment = 0;
  
  // set am/pm
  period = 0;
  if (hour_int > 11)
    period = 1;
  
  if (hour_int == 0)
  {
    // for 0am
    hour_str = String(12);
  }
  else
  {
    // build the string, adjusting for 24 hour
    // if needed
    if (hour_int > 12)
      adjustment = 12;
    hour_str = String(hour_int - adjustment);
  }

  hour_str.toCharArray(hour_char, 3);
}

void printSeconds()
{
  mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mx.setBuffer(period_pos, 3, blank);
  mx.setBuffer(hour_tens_pos, 5, blank);
  mx.setBuffer(hour_ones_pos, 5, blank);
  mx.setChar(min_tens_pos, second_char[0]);
  mx.setChar(min_ones_pos, second_char[1]);
  mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void printMinute()
{
  if (!display_seconds)
  {
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    mx.setChar(min_tens_pos, minute_char[0]);
    mx.setChar(min_ones_pos, minute_char[1]);
   //alarm bell
   //mx.setBuffer(31, 3, bell);
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  }
}

void printHour()
{
  if (!display_seconds)
  {
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  
    // print PM or AM indicator
    mx.setBuffer(period_pos, 3, ampm[period]);

    if (hour_str.length() > 1)
    {
      // print '10:'
      mx.setChar(hour_tens_pos, hour_char[0]);
      mx.setChar(hour_ones_pos, hour_char[1]);
    }
    else
    {
      // must explicitly blank out the tens place or '1' will be left
      mx.setBuffer(hour_tens_pos, 5, blank);
      // print '9:'
      mx.setChar(hour_ones_pos, hour_char[0]);
    }

    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  }
}

void printTime()
{
  printMinute();
  printHour();
}

void writeNewTime()
{
  writing_time = 1;
  // setting just hour, and while we were reading the new hour we stopped
  // updating seconds, so we got behind.
  // read the new time first and get the correct minute and second, write that
  // back out
  if (setting_hour && !setting_min)
  {
    DateTime now = rtc.now();
    second_int = now.second();
    minute_int = now.minute();
  }
  else
  {
    second_int = 0;
  }
  
  DateTime new_time (0, 0, 0, hour_int, minute_int, second_int);
  rtc.adjust(new_time);
  writing_time = 0;
  setting_hour = 0;
  setting_min = 0;
  buildHour();
  buildMinute();
  printTime();
}

void updateTime()
{
  if (writing_time)
  {
    update_flag = 0;
    return;
  }
  
  DateTime now = rtc.now();
  if (display_seconds)
  {
    if (start_seconds == 0)
    {
     start_seconds = now.second();
    }
    else
    {
      if (now.second() == (((start_seconds) + 15) % 60))
      {
        start_seconds = 0;
        display_seconds = 0;
        printTime();
      }
    }
  }
  if (!setting_min)
  {
    if (second_int != now.second())
    {
      second_int = now.second();
      buildSecond();
    }
    if (minute_int != now.minute())
    {
      minute_int = now.minute();
      buildMinute();
      printMinute();
    }
  }
  if (!setting_hour)
  {
    if (hour_int != now.hour())
    {
      hour_int = now.hour();
      buildHour();
      printHour();
    }
  }
}

void setUpdateFlag()
{
  update_flag = 1;
}

void setup()
{
  Serial.begin(9600);

  // set up to handle interrupt from 1 Hz pin
  pinMode(rtcTimerIntPin, INPUT_PULLUP);
  rtc.begin();
  // enable the 1 Hz output
  rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
  attachInterrupt(digitalPinToInterrupt(rtcTimerIntPin),
                  setUpdateFlag, FALLING);
  
  pinMode(SECONDS_PIN, INPUT);
  pinMode(TIME_SET_PIN, INPUT);
  pinMode(HOUR_SET_PIN, INPUT);
  pinMode(MIN_SET_PIN, INPUT);

  // setup dot matrix
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 3);
  mx.setColumn(sep_pos, 0x14);

  // build the initial time and print it for a start
  updateTime();
  printTime();
}

void loop() 
{
  if (update_flag)
  {
    updateTime();
    if (display_seconds)
    {
      printSeconds();
    //  display_seconds--;
     // if (!display_seconds)
     //   printTime();
    }
    update_flag = 0;
  }

  seconds_disp_state = digitalRead(SECONDS_PIN);
  if (seconds_disp_state == HIGH)
  {
    display_seconds = 14;
    printSeconds();
  }
  
  time_set_state = digitalRead(TIME_SET_PIN);
  if (time_set_state == HIGH)
  {
    if (!display_seconds)
      setTime();
  }
}

