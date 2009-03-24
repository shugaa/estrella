Estrella device driver for the StellarNet EPP2000 series of spectrometers.

This driver is implemented as a userspace library on top of libusb. It has been
written with Linux as a target OS in mind but porting to any libusb supported
platform should be easy. If it takes any effort at all.

Estrella can only handle USB devices at the moment but should be easily
extensible to drive IEEE-1284 connected equipment.

For documentation have a look at the 'doc' directory. The provided sample
application in 'test' should give you a pretty good idea of how things work. The
driver API resembles the semantics of the windows version quite closely, you
should feel right at home.

Installation instructions can be found in INSTALL.txt.
