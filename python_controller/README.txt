Copyright (c) 2014, Kleydson Stenio (ninloth@gmail.com)

This is a script for controlling the spectrometer using Python (2.7.x) instead of compiling a C binary.

The Python stript is intended for users with less knowledge in C language, users that need to control others devices during
the measurement, or for those that just prefer Python over C.

For this script to work you must install the driver first, and have acess to the libraries "libestrella.so" and "libdll.so".
Be aware that some of the functions inside the libraries are only available if you have root rights.
