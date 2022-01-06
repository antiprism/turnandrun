# FAQ for turnandrun

### What equipment is required

An I2C ADS1115 or ADS1015 board and a 10k Potentiometer.

I use a cheap ADS1115 boards and a $1 10k potentiometers, of the kind
available from eBay, Aliexpress, Amazon, and other online sites. This
setup can handle 100 dial positions.


### Why do the readings jump in increments of 16?

You have an ADS1015 board that is configured as an ADS1115. Some
cheap boards are advertised and marked as having an ADS1115 chip,
but in fact contain an ADS1015 chip. If you are happy with the
performance of the board then it is not a problem that the readings
jump in increments of 16.


### What are the differences between a potentiometer and a rotary encoder?

In general terms, a potentiometer dial reading is absolute, and corresponds
to the angle it is turned to, whereas a rotary encoder dial reading
is relative, and corresponds to the angle it is turned through. This means that
- **potentiometers** are good for selecting a value based on dial position,
  but if the current value is changed by another method then the dial
  position will not reflect the current value.
- **rotary encoders** are good for iterating through a selection starting
  at the current value, and if this value is changed by another method
  the rotary encoder simply starts from the new value, but the rotary
  encoder cannot select anything by turning to a particular position.

### What commands can I run to control a music player?

turnandrun runs commands that can be run from the coomand line.
See the [commands](commands.md) list for example commands to control
Volumio, Moode and MPD. If you are having difficulty finding a particular
command to control a player then I recommend asking on the forum for that
player.
