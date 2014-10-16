This is a script for controlling the spectrometer using Python (2.7.x) instead of compiling a C binary.

The Python stript is intended for users with less knowledge in C language, users that need to control others devices during
the measurement, or for those that just prefer Python over C.

For this script to work you must install the driver first, and have acess to the libraries "libestrella.so" and "libdll.so". Be aware that some of the functions inside the libraries are only available if you have root rights.

It is recommended (but not mandatory) to have Numpy (http://www.numpy.org/) and Matplotlib (http://matplotlib.org/) installed, since they are used here to plot the values from the measurement.
