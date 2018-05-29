#include <MD_MAX72xx.h>
#include "RTClib.h"
#include <Wire.h>
#include "dot_matrix_clock.h"

void adjustBrightness()
{
  // Test for button pressed and store the down time
  if (brightness_btn_state == HIGH && last_brightness_btn_state == LOW &&
      (millis() - brightness_up_time) > long(debounce_delay)) {
    brightness_down_time = millis();
  }

  // Test for button release and store the up time
  if (brightness_btn_state == LOW && last_brightness_btn_state == HIGH &&
      (millis() - brightness_down_time) > long(debounce_delay))
  {
    if (ignore_brightness_up == false)
    {
      // change the brightness by one setting
      brightness_index = (brightness_index + 1) % ((sizeof(brightness_options) /
                                                    sizeof(int)));
      mx.control(MD_MAX72XX::INTENSITY, brightness_options[brightness_index]);
    }
    else
    {
      ignore_brightness_up = false;
    }
    brightness_up_time = millis();
  }

  /*
   * Test for button held down for longer than the hold time.
   * In this case, set the pin to low and ignore.  I don't want
   * the brightness cycling through really fast like hours or
   * minutes do.  I want the brightness to only change by one
   * per button press.
   */
  if (brightness_btn_state == HIGH &&
      (millis() - brightness_down_time) > button_hold_time)
  {
    digitalWrite(BRIGHTNESS_PIN, LOW);
    ignore_brightness_up = true;
    brightness_down_time = millis();
  }
}

void handleAlarm()
{
  if (alarm_btn_state == HIGH)
  {
    // alarm was not turned on, turn it on
    alarm_pwr_state = 1;
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    // alarm bell
    mx.setBuffer(31, 3, bell);
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  }

  if (alarm_btn_state == LOW)
  {
    // alarm was on, turn it off and stop any tone
    // being generated
    digitalWrite(BUZZER_PIN, LOW);
    alarm_pwr_state = 0;
    alarming = 0;
    alarm_check_toggle = false;
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    // alarm bell
    mx.setBuffer(31, 3, blank);
    mx.control(0, MAX_DEVICES-1, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  }

  // We need to reset the alarm bit in either case.  If the alarm has been
  // off, that just means we haven't paid attention to it.  The A1F bit could
  // still be set from the last time it was triggered.  So every time the
  // alarm is turned on, that bit should be reset.  Coincidentally, if a user
  // turns the alarm on right at the moment it was about to go off, this
  // would reset the alarm until 24 hours from now.  But a user shouldn't
  // have a real use-case for this.
  // In the case of turning the alarm off, that's a no brainer, we need to
  // reset the bit.
  resetAlarmStatus();
}

void handleSnooze()
{
  // Test for button release and store the up time
  if ((millis() - snooze_btn_debounce_time) > long(debounce_delay))
  {
    if (snooze_btn_state == HIGH)
    {
      if (alarming)
      {
        digitalWrite(BUZZER_PIN, LOW);
        alarming = 0;
        alarm_check_toggle = false;

        // this is the magic that resets us every 9 minutes,
        // reset the A2F bit so it can come on again 9
        // minutes from now.
        // Also reset the alarm status.  They haven't turned
        // the alarm off yet, but if we don't do this we'll
        // keep triggering every check of the alarm status.
        // The only way for them to get out of this state is
        // to turn off the alarm. So we know it's coming.
        // Basically, once you enter this state by hitting
        // snooze, the only alarm you can use from that point
        // on is snooze until you turn alarm off.
        resetAlarmStatus();

        // Write 9 minutes from now into the alarm 2 register
        // setting hour and minute each time by default
        // code taken from RTClib adjust()
        Wire.beginTransmission(DS3231_I2C_ADDRESS);
        // set to register 0xb and write two bytes
        Wire.write(0xb);
        // grab minute_int once here in case it changes while
        // doing calculations
        int minute_plus = minute_int + 9;
        int new_minute = minute_plus % 60;
        int new_hour = hour_int + (minute_plus / 60);
        Wire.write(decToBcd(new_minute));
        Wire.write(decToBcd(new_hour));
        Wire.endTransmission();
      }
      snooze_btn_debounce_time = millis();
    }
  }
}

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
      hour_up_time = millis();
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
      min_up_time = millis();
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
  PRINT_HOUR(hour_tens_pos, alarm_hour_char[0],
             hour_ones_pos, alarm_hour_char[1],
             alarm_hour_int, alarm_period);
  PRINT_MIN(min_tens_pos, alarm_minute_char[0],
            min_ones_pos, alarm_minute_char[1]);

  while (alarm_set_state == HIGH)
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
  // turn the separator on
  mx.setColumn(sep_pos, 0x14);
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

  // reset alarm since we just wrote a new one, just in case
  resetAlarmStatus();

  setting_alarm_hour = 0;
  setting_alarm_min = 0;
}

void initializeAlarm()
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

  // Set the A1M4 bit to 1 so that the alarm matches on hour/minute/second
  // Otherwise it would match on date as well, and I'm not setting dates
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  // set to register a and write one byte
  Wire.write(0xa);
  Wire.write(decToBcd(0x80));
  Wire.endTransmission();

  // do the same for alarm 2, A2M4
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  // set to register 0xd and write one byte
  Wire.write(0xd);
  Wire.write(decToBcd(0x80));
  Wire.endTransmission();
}

void getAlarmStatus()
{
  // get A1F and A2F status
  // code taken from RTClib now()
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0xf);
  Wire.endTransmission();
  // read one byte: status register that holds A1F bit
  Wire.requestFrom(DS3231_I2C_ADDRESS, 1);
  status_register = bcdToDec(Wire.read());
}

// Reset alarm AND snooze
void resetAlarmStatus()
{
  getAlarmStatus();
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  // set to register 0xf and write one byte
  Wire.write(0xf);
  Wire.write(decToBcd(status_register ^ 0x03));
  Wire.endTransmission();
}

// Reset only snooze.  When snoozing we want to be able to reset
// this while leaving the alarm on.
void resetSnoozeStatus()
{
  getAlarmStatus();
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  // set to register 0xf and write one byte
  Wire.write(0xf);
  Wire.write(decToBcd(status_register ^ 0x02));
  Wire.endTransmission();
}

void checkAlarm()
{
  if (!alarm_pwr_state)
    return;

  getAlarmStatus();
  if ((status_register & 0x01) ||
      (status_register & 0x02))
  {
    alarming = 1;
    digitalWrite(BUZZER_PIN, HIGH);
  }
}

void updateTime()
{
  if (writing_time)
    return;

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
      // blink on the separator
      mx.setColumn(sep_pos, 0x14);
      start_sep = millis();
      second_int = new_second_int;
      BLD_MIN_SEC_STR(second_char, second_str, second_int);
    } else if (!display_seconds && ((millis() - start_sep) > 499))
      // blink off separator
      mx.setColumn(sep_pos, 0x00);

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
  checkAlarm();
}

void setup()
{
  Serial.begin(9600);

  Wire.begin();

  pinMode(SECONDS_PIN, INPUT);
  pinMode(TIME_SET_PIN, INPUT);
  pinMode(ALARM_SET_PIN, INPUT);
  pinMode(HOUR_SET_PIN, INPUT);
  pinMode(MIN_SET_PIN, INPUT);
  pinMode(ALARM_PWR_PIN, INPUT);
  pinMode(SNOOZE_PIN, INPUT);
  pinMode(BRIGHTNESS_PIN, INPUT);
  // this is an active buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  // setup dot matrix
  mx.begin();
  brightness_index = default_brightness_index;
  mx.control(MD_MAX72XX::INTENSITY, brightness_options[brightness_index]);

  // build the initial time and print it for a start
  updateTime();
  time_initialized = 1;
  printTime();

  // Read the initial alarm value for display purposes
  initializeAlarm();
}

void loop()
{
  updateTime();

  if (display_seconds)
  {
    printSeconds();
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

  alarm_btn_state = digitalRead(ALARM_PWR_PIN);
  if (((alarm_btn_state == HIGH) && (!alarm_pwr_state)) ||
      ((alarm_btn_state == LOW) && (alarm_pwr_state)))
  {
    // turn on alarm
    if (!time_set_state &&
        !alarm_set_state)
      handleAlarm();
  }

  snooze_btn_state = digitalRead(SNOOZE_PIN);
  if ((snooze_btn_state == HIGH) && (alarming))
  {
    if (!time_set_state &&
        !alarm_set_state)
      handleSnooze();
  }

  brightness_btn_state = digitalRead(BRIGHTNESS_PIN);
  // call adjust brightness if this reading is different than
  // the last one, either the user pushed the button down or
  // let it up.  Let the function deal with how to handle
  // LOW and HIGH.
  if ((brightness_btn_state != last_brightness_btn_state))
  {
    if (!time_set_state &&
        !alarm_set_state)
      adjustBrightness();
  }
  last_brightness_btn_state = brightness_btn_state;
}

