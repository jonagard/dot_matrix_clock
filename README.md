### Summary

![Protoype picture](IMG_20190322_015840763.jpg?raw=true "Prototype")

This is an arduino-based clock using a 7219 dot matrix display:

https://www.amazon.com/Wangdd22-MAX7219-Arduino-Microcontroller-Display/dp/B01EJ1AFW8

This is an alarm clock based on a traditional bedside alarm clock.  It
supports setting time, setting an alarm, displaying seconds, and changing
brightness.  It supports a backup battery source, 3 AA batteries, to get
a user through the night should the power go out.

Clock is based on the DS3231 RTC:

https://www.amazon.com/DS3231-AT24C32-Precision-Module-Arduino/dp/B01IXXACD0

The clock stores the set time in the RTC and reads the time from it.  It
uses an interrupt from the RTC to read every second, rather than
continually polling to see if the time has changed.  Ideally this saves
battery but probably not in reality, since most of the power usage are
the LEDs.  It also writes the time to the dot matrix display and then
does not rewrite that minute or hour until a change occurs.  Big dreams
of efficiency that are probably wasted.

If power switches to the backup battery, the clock brightness
automatically dims to the lowest setting and the separator stops
blinking.  Also, a "b" symbol is printed on the left-most three columns.
If the battery gets below 1.5V, an "L" will be printed.  When power is
restored, the brightness goes back to the default brightness.

The clock only checks the battery voltage every time the hour changes.
It is my assumption it would use more battery to check it more
regularly?  Thus, if you put a new battery in, it will take till the
next hour change for the "L" to go away.  The "L" won't appear when
there is no battery in the clock.  Again, if a battery is removed after
"L" is printed, it will take till the next hour change to disappear.

### Limitations

It does not support 24-hour time.  It does not support date even though
the RTC can handle that.  I don't want to add more buttons for either of
these functions.  Plus no traditional bedside alarm clock I used growing
up (the classic red-glow, 7-segment, fake-wood clocks) had these
functions, so I'm not motivated.

The RTC supports two alarms but I intend to only support one.  The second
alarm is used in the implementation of snooze.

### Goals

- [x] Show time from an RTC
- [x] Show seconds upon button push
- [x] Set the time using time set, hour, and minute buttons
- [x] Blink separator
- [x] Set an alarm
- [x] Have alarm trigger a buzzer
- [x] Snooze
- [x] Support changing brightness
- [x] Backup power (RTC has time backup already, this is for the display
  and alarm to still work)
- [ ] ~~Show temperature?~~
  - I added this functionality and the temperature was impressively off.  Other
    thermometers in the same space were saying 72 or 73 degrees F and the RTC
    was saying 66 degrees F.  Why even add the feature if it's that unreliable.
- [ ] Build an enclosure
