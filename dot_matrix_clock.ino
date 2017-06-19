#include <MD_MAX72xx.h>
#include "RTClib.h"
#include <Wire.h>
#include "dot_matrix_clock.h"

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
      // print something like '10:'
      mx.setChar(hour_tens_pos, hour_char[0]);
      mx.setChar(hour_ones_pos, hour_char[1]);
    }
    else
    {
      // must explicitly blank out the tens place or '1' will be left
      // each char is five dots wide
      mx.setBuffer(hour_tens_pos, 5, blank);
      // print something like '9:'
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

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void writeNewTime()
{
  writing_time = 1;

  if (setting_hour && !setting_min)
  {
    // setting just hour
    // code taken from RTClib adjust()
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    // set to register 2 and write one byte
    Wire.write(2);
    Wire.write(decToBcd(hour_int));
    Wire.endTransmission();
  }
  else
  {
    // setting hour, minute, and second
    // code taken from RTClib adjust()
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    // set to register 0 and write three bytes
    Wire.write(0);
    Wire.write(decToBcd(0));
    Wire.write(decToBcd(minute_int));
    // The set time function will stay in a loop until the
    // time set button is released.  So someone could set
    // both hour and minutes while holding the button down.
    // While it may be rare, it's safer to set the hour every
    // time the minute is changed.
    Wire.write(decToBcd(hour_int));
    Wire.endTransmission();
  }
  
  writing_time = 0;
  setting_hour = 0;
  setting_min = 0;
}

void updateTime()
{
  if (writing_time)
  {
    update_flag = 0;
    return;
  }
  
  // get time
  // code taken from RTClib now()
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();
  // read three bytes:  second, minute, hour
  Wire.requestFrom(DS3231_I2C_ADDRESS, 3);
  uint8_t new_second_int = bcdToDec(Wire.read() & 0x7F);
  uint8_t new_minute_int = bcdToDec(Wire.read());
  uint8_t new_hour_int = bcdToDec(Wire.read());

  // the logic that controls seconds displaying for 15 seconds
  // this sets the initial value and resets everything after
  // 15 seconds
  if (display_seconds)
  {
    if (start_seconds == 0)
    {
      //start_seconds = now.second();
      start_seconds = new_second_int;
    }
    else
    {
      if (new_second_int == (((start_seconds) + 15) % 60))
      {
        start_seconds = 0;
        display_seconds = 0;
        printTime();
      }
    }
  }

  // update the time
  if (!setting_min)
  {
    if (!time_initialized || second_int != new_second_int)
    {
      second_int = new_second_int;
      buildSecond();
    }
    if (!time_initialized || minute_int != new_minute_int)
    {
      minute_int = new_minute_int;
      buildMinute();
      printMinute();
    }
  }
  if (!setting_hour)
  {
    if (!time_initialized || hour_int != new_hour_int)
    {
      hour_int = new_hour_int;
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

  Wire.begin();
  
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
  time_initialized = 1;
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
    }
    update_flag = 0;
  }

  seconds_disp_state = digitalRead(SECONDS_PIN);
  if (seconds_disp_state == HIGH)
  {
    display_seconds = 1;
    printSeconds();
  }
  
  time_set_state = digitalRead(TIME_SET_PIN);
  if (time_set_state == HIGH)
  {
    if (!display_seconds)
      // don't allow setting time while seconds
      // are showing
      setTime();
  }
}

