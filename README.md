### Summary

This is an arduino-based clock using a 7219 dot matrix display:

https://www.amazon.com/Wangdd22-MAX7219-Arduino-Microcontroller-Display/dp/B01EJ1AFW8

For now the plan is to be like a traditional bedside alarm clock.  It
should support setting time, setting an alarm, displaying seconds, and
changing brightness.  A stretch goal would be supporting a backup 9v
battery power source to get a user through the night should the power go
out.

Clock is based on the DS3231 RTC:

https://www.amazon.com/DS3231-AT24C32-Precision-Module-Arduino/dp/B01IXXACD0

The clock stores the set time in the RTC and reads the time from it.  It
uses an interrupt from the RTC to read every second, rather than
continually polling to see if the time has changed.  Ideally this saves
battery but probably not in reality, since most of the power usage are
the LEDs.  If I get a backup battery working, maybe I can make the clock
brightness automatically dim to the lowest setting.  It also writes the
time to the dot matrix display and then does not rewrite that minute or
hour until a change occurs.  Again, big dreams of efficiency that are
probably wasted.

### Limitations

It does not support 24-hour time.  It does not support date even though
the RTC can handle that.  I don't want to add more buttons for either of
these functions.  Plus no traditional bedside alarm clock I used growing
up (the classic red-glow, 7-segment, fake-wood clocks) had these
functions, so I'm not motivated.

The RTC supports two alarms but I intend to only support one.  I plan on
using the second alarm in the implementation of snooze.

### Goals

- [x] Show time from an RTC
- [x] Show seconds upon button push
- [x] Set the time using time set, hour, and minute buttons
- [x] Blink separator
- [x] Set an alarm
- [x] Have alarm trigger a buzzer
  - [ ] Volume control?
- [x] Snoozer
- [ ] Support changing brightness
- [ ] Backup power (RTC has time backup already, this is for the display
  and alarm to still work)
- [ ] Show temperature?
- [ ] Build an enclosure
