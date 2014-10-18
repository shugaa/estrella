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
from ctypes import byref, c_float
import matplotlib.pyplot as plt
from os import getcwd

def estrella_controller():
	
	print '\n','Initializing Estrella Python Driver','\n'
	
	# saves the root of the program
	root = getcwd()
	
	# import main libraries to control the spectrometer (change the location to match yours)
	dll, estrella = import_libraries('/opt/libdll/lib/libdll.so','/opt/estrella/lib/libestrella.so')
	
	# create x-axis (must pass the location of file with calibration parameters, change if needed)
	wavelength = create_xaxis(root + '/calibration_parameters.txt')
	
	# create a device list for the libraries
	device = c_void_p(None)
	numdevices = c_uint(0)
	devices = dll_list_t()
	dll.dll_init(byref(devices))
	
	# create a session for Estrella
	esession = estrella_session_t()
	
	# try to set up an estrella session
	rc = estrella.estrella_find_devices(byref(devices))
	if rc != 0:
		print 'It was not possible to look for USB devices.\n(Are you root?)','\n'
		dll.dll_clear(byref(devices))
		return 1
	
	rc = dll.dll_count(byref(devices), byref(numdevices))
	if ((rc != 1) or (numdevices == 0)):
		print 'No device found.','\n'
		dll.dll_clear(byref(devices))
		return 1
	
	rc = dll.dll_get(byref(devices), byref(device), None, c_int(0))
	if (rc != 1):
		print 'It was not possible to acess the EPP2000 device.','\n'
		dll.dll_clear(byref(devices))
		return 1
	
	rc = estrella.estrella_init(byref(esession), device)
	if (rc != 0):
		print 'It was not possible to create the session.','\n'
		estrella.estrella_close(byref(esession))
		dll.dll_clear(byref(devices))
		return 1
	
	rc = estrella.estrella_update(byref(esession), c_int(1), ESTR_XSMOOTH_NONE, ESTR_TEMPCOMP_OFF)
	if (rc != 0):
		print 'It was not possible to configure the data.','\n'
		estrella.estrella_close(byref(esession))
		dll.dll_clear(byref(devices))
		return 1
	
	rc = estrella.estrella_rate(byref(esession), ESTRELLA_RATE, ESTR_XRES_HIGH);
	if (rc != 0):
		print 'It was not possible to configure the rates.','\n'
		estrella.estrella_close(byref(esession))
		dll.dll_clear(byref(devices))
		return 1
	
	rc = estrella.estrella_mode(byref(esession), ESTR_XTMODE_NORMAL)
	if (rc != 0):
		print 'It was not possible to configure operation mode.','\n'
		estrella.estrella_close(byref(esession))
		dll.dll_clear(byref(devices))
		return 1
	
	# create a C like float array and do the scan
	data = (c_float * 2051)()
	rc = estrella.estrella_scan(byref(esession), byref(data))
	if (rc != 0):
		print 'The scan failed.','\n'
		estrella.estrella_close(byref(esession))
		dll.dll_clear(byref(devices))
		return 1
	
	# convert the scan result into a Numpy array
	counts = create_yaxis(data)
	
	# plot the result using Matplotlib
	plt.plot(wavelength,counts)
	plt.show()
	
	print '\n','All done without errors.','\n'
	
	return 0

if __name__ == '__main__':
	estrella_controller()
