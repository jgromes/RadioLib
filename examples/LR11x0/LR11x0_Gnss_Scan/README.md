LR11x0 GNSS Scan Example Sketch
===============================

This sketch exercises the LR11x0 GNSS functionality and prints information on the console.
It is intended to show how to use the GNSS functions and provide examples on how to interpret
some of the results.

It primarily loops performing GNSS scans but also checks the almanac status and asks the LR11x0
to receive almanac updates from satellite transmissions.

The sketch is configured to work as-is on a Seeed WIO Tracker 1110 dev board (red in color)
and using the sketch on different hardware requires small adaptations.
The LR11x0 almost always uses external RF switches that are controlled through some of its
pins and which pin controls which switch is board dependent.
On the WIO Tracker board this sketch uses the Seeed Arduino core.

Currently the sketch also requires `#define RADIOLIB_GODMODE 1` in `BuildOptUser.h` to access
otherwise private functions in the LR11x0 class.
