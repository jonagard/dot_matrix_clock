#include <MD_MAX72xx.h>
#include "RTClib.h"
#include <Wire.h>
#include "dot_matrix_clock.h"

void setTime()
{
  while (time_set_state == HIGH)
  {
    // turn the separator on, otherwise it's on or off depending on the state
    // when you hit the button
    mx.setColumn(sep_pos, 0x14);

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
        INCR(hour_int, 23);
        BLD_HOUR_STR(hour_char, hour_str, hour_int)
        PRINT_HOUR(hour_tens_pos, hour_char[0],
                   hour_ones_pos, hour_char[1],
                   hour_int, period);
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
      INCR(hour_int, 23);
      BLD_HOUR_STR(hour_char, hour_str, hour_int)
      PRINT_HOUR(hour_tens_pos, hour_char[0],
                 hour_ones_pos, hour_char[1],
                 hour_int, period);
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
        INCR(minute_int, 59);
        BLD_MIN_SEC_STR(minute_char, minute_str, minute_int);
        PRINT_MIN(min_tens_pos, minute_char[0], min_ones_pos, minute_char[1]);
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
      INCR(minute_int, 59);
      BLD_MIN_SEC_STR(minute_char, minute_str, minute_int);
      PRINT_MIN(min_tens_pos, minute_char[0], min_ones_pos, minute_char[1]);
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

void setAlarm()
{
  while (alarm_set_state == HIGH)
  {
    // turn the separator on, otherwise it's on or off depending on the state
    // when you hit the button
    mx.setColumn(sep_pos, 0x14);
    PRINT_HOUR(hour_tens_pos, alarm_hour_char[0],
               hour_ones_pos, alarm_hour_char[1],
               alarm_hour_int, alarm_period);
    PRINT_MIN(min_tens_pos, alarm_minute_char[0],
              min_ones_pos, alarm_minute_char[1]);

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
        setting_alarm_hour = 1;
        INCR(alarm_hour_int, 23);
        BLD_HOUR_STR(alarm_hour_char, alarm_hour_str, alarm_hour_int)
        PRINT_HOUR(hour_tens_pos, alarm_hour_char[0],
                   hour_ones_pos, alarm_hour_char[1],
                   alarm_hour_int, alarm_period);
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
      setting_alarm_hour = 1;
      INCR(alarm_hour_int, 23);
      BLD_HOUR_STR(alarm_hour_char, alarm_hour_str, alarm_hour_int)
      PRINT_HOUR(hour_tens_pos, alarm_hour_char[0],
                 hour_ones_pos, alarm_hour_char[1],
                 alarm_hour_int, alarm_period);
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
        setting_alarm_min = 1;
        INCR(alarm_minute_int, 59);
        BLD_MIN_SEC_STR(alarm_minute_char, alarm_minute_str, alarm_minute_int);
        PRINT_MIN(min_tens_pos, alarm_minute_char[0],
                  min_ones_pos, alarm_minute_char[1]);
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
      setting_alarm_min = 1;
      INCR(alarm_minute_int, 59);
      BLD_MIN_SEC_STR(alarm_minute_char, alarm_minute_str, alarm_minute_int);
      PRINT_MIN(min_tens_pos, alarm_minute_char[0],
                min_ones_pos, alarm_minute_char[1]);
      ignore_min_up = true;
      min_down_time = millis();
    }

    last_hour_set_state = hour_set_state;
    last_min_set_state = min_set_state;
    alarm_set_state = digitalRead(ALARM_SET_PIN);
  }

  printTime();

  if (setting_alarm_hour || setting_alarm_min)
    writeNewAlarm();
}

/*
 * I made the printing of minutes and hours a macro.  I am not doing so
 * with seconds.  For some reason when I printed this way the seconds
 * never printed.  The minutes stayed up.  I didn't feel like debugging.
 * My main goal was using the same handling to print minutes/hours for
 * alarm and time anyway.  Seconds are only called in one context.
 */
void printSeconds()
{
  CLEAR_DISP();
  mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mx.setChar(min_tens_pos, second_char[0]);
  mx.setChar(min_ones_pos, second_char[1]);
  mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void printTime()
{
  PRINT_MIN(min_tens_pos, minute_char[0],
            min_ones_pos, minute_char[1]);
  PRINT_HOUR(hour_tens_pos, hour_char[0],
             hour_ones_pos, hour_char[1],
             hour_int, period);
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

void writeNewAlarm()
{
  // setting hour and minute each time by default
  // code taken from RTClib adjust()
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  // set to register 8 and write two bytes
  Wire.write(8);
  Wire.write(decToBcd(alarm_minute_int));
  Wire.write(decToBcd(alarm_hour_int));
  Wire.endTransmission();

  setting_alarm_hour = 0;
  setting_alarm_min = 0;
}

void readAlarm()
{
  // get alarm minute and hour
  // ignore seconds because alarm seconds will always be zero
  // code taken from RTClib now()
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  // alarm 1 minutes register
  Wire.write(8);
  Wire.endTransmission();
  // read two bytes:  minute, hour
  Wire.requestFrom(DS3231_I2C_ADDRESS, 2);
  alarm_minute_int = bcdToDec(Wire.read());
  alarm_hour_int = bcdToDec(Wire.read());
  BLD_HOUR_STR(alarm_hour_char, alarm_hour_str, alarm_hour_int)
  BLD_MIN_SEC_STR(alarm_minute_char, alarm_minute_str, alarm_minute_int);
}

void updateTime()
{
  // blink on separator
  mx.setColumn(sep_pos, 0x14);

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
      BLD_MIN_SEC_STR(second_char, second_str, second_int);
    }
    if (!time_initialized || minute_int != new_minute_int)
    {
      minute_int = new_minute_int;
      BLD_MIN_SEC_STR(minute_char, minute_str, minute_int);
      PRINT_MIN(min_tens_pos, minute_char[0], min_ones_pos, minute_char[1]);
    }
  }
  if (!setting_hour)
  {
    if (!time_initialized || hour_int != new_hour_int)
    {
      hour_int = new_hour_int;
      BLD_HOUR_STR(hour_char, hour_str, hour_int)
      PRINT_HOUR(hour_tens_pos, hour_char[0],
                 hour_ones_pos, hour_char[1], hour_int, period);
    }
  }
}

// The function called at the 1Hz interrupt.  Can't do any
// real work here.  Just set the update_flag and let the
// loop do work based on its status.
void setUpdateFlag()
{
  update_flag = 1;
}

void setup()
{
  Serial.begin(9600);

  Wire.begin();

  // set up to handle interrupt from 1Hz pin
  pinMode(rtcTimerIntPin, INPUT_PULLUP);
  rtc.begin();
  // enable the 1Hz output
  rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
  attachInterrupt(digitalPinToInterrupt(rtcTimerIntPin),
                  setUpdateFlag, FALLING);

  pinMode(SECONDS_PIN, INPUT);
  pinMode(TIME_SET_PIN, INPUT);
  pinMode(ALARM_SET_PIN, INPUT);
  pinMode(HOUR_SET_PIN, INPUT);
  pinMode(MIN_SET_PIN, INPUT);

  // setup dot matrix
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 3);

  // build the initial time and print it for a start
  updateTime();
  time_initialized = 1;
  printTime();

  // Read the initial alarm value for display purposes
  readAlarm();
}

void loop()
{
  if (update_flag)
  {
    updateTime();
    // save time the blinker came on, will use this to turn it
    // off below
    start_sep = millis();
    if (display_seconds)
    {
      printSeconds();
    }
    update_flag = 0;
  }
  else
  {
    if ((millis() - start_sep) > 499)
      // blink off separator
      mx.setColumn(sep_pos, 0x00);
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
    if (!display_seconds && !alarm_set_state)
      // don't allow setting time while seconds
      // are showing
      setTime();
  }

  alarm_set_state = digitalRead(ALARM_SET_PIN);
  if (alarm_set_state == HIGH)
  {
    if (!display_seconds && !time_set_state)
      // don't allow setting alarm while seconds
      // are showing
      setAlarm();
  }
}

