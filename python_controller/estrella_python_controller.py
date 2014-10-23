#!/usr/bin/env python
# 
# Copyright (c) 2014, Kleydson Stenio (ninloth@gmail.com)
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name of the author nor the names of its contributors may be
#   used to endorse or promote products derived from this software without
#   specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# *******************************************************************************
# 
# Python Controller, main program.
# 

from estrella_classes import *
from sys import exit
from os import getcwd
import matplotlib.pyplot as plt

def estrella_controller():
	
	print '\n','Initializing Estrella Python Driver','\n'
	
	# saves the root of the program
	root = getcwd()
	
	# import main libraries to control the spectrometer (change the location to match yours)
	dll, estrella = import_libraries('/opt/libdll/lib/libdll.so','/opt/estrella/lib/libestrella.so')
	
	# create x-axis (must pass the location of file with calibration parameters, change if needed)
	wavelength = create_xaxis(root + '/calibration_parameters.txt')
	
	# create the device list and the estrella session
	try:
		devices, esession = estrella_begin(dll, estrella)
	except TypeError:
		print 'Error detected, exiting.','\n'
		exit(1)
	
	# setup estrella session
	setup = estrella_setup(esession, devices, dll, estrella, ESTR_XSMOOTH_NONE, ESTR_XTMODE_NORMAL, ESTR_TEMPCOMP_OFF, ESTRELLA_RATE, ESTR_XRES_HIGH)
	if setup != 0:
		print 'Error detected, exiting.','\n'
		exit(1)
	
	# do the normal scan
	counts = estrella_nscan(devices, esession, dll, estrella)
	if counts == 1:
		print 'Error detected, exiting.','\n'
		exit(1)
	
	# plot the result using Matplotlib
	plt.plot(wavelength,counts)
	plt.show()
	
	# close the session
	estrella_end(devices, esession, dll, estrella)
	
	print '\n','All done without errors.','\n'
	
	return 0

if __name__ == '__main__':
	estrella_controller()
