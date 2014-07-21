WS2811 multiple strips library
==============================

The WS2811 RGB LED strips have extremely many potential uses.  In some
of them, like POV displays, very high refresh rates are needed.  If all
your LEDs are connected serially, the time required to update all of the
pixel values or any subset of them is 30us times the total number of
LEDs connected -- even if you don't need to update all of their colours.
At, say, 100 LEDs that's already 3ms per refresh, which limits your
total frame rate to 300 Hz and on an Arduino if you add all other
overheads you may end up with even lower rates.

To improve on this a little, you can connect various strips in parallel
instead of serially, i.e. with their data lines connected to different
Atmega GPIO pins.  The routines in this version of WS2811.h allow
outputting the pixel data to up to 4 strips at the same time, if
connected to GPIOs on the same Atmega port (e.g. PC0, PC1) of 2 strips
if connected to different ports (e.g. PC2 and PB4).

For example 4 strips of 25 LEDs connected to PC0 - PC3 (A0 to A3 in
Arduino naming) can be updated in about 750us.  Not an order of
magnitude jump but enough to make some projects practical.

Further improvement
===================

With some further assumptions you can probably drive up to at least 16
different LED strips from an Arduino, this library doesn't have any
support for that.  It would require some preprocessing of the pixel
data before outputting it to the AVR digital pins, so if you're
animating something that may not be viable, but if you've got static
data it might work.  This also requires a good amount of memory, so
your strips probably can't be too long if you're on an Atmega328P.
Someone has already implemented that and explains the details here
with the 8 LED strips case:
http://rurandom.org/justintime/index.php?title=Driving_the_WS2811_at_800_kHz_with_an_8_MHz_AVR

Installation
============

Like with every Arduino library, put the files in a "libraries"
directory in one of the various standard paths, and restart the
Arduino IDE.  The name of the subdirectory inside "libraries" needs to
be WS2811.  For example, clone this repository, cd inside, and do
ln -s `pwd` ~/Arduino/libraries/WS2811
