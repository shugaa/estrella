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
# Python Controller, needed functions (if you need more, add here).
# 

from ctypes import cdll, byref, c_uint, c_int, c_void_p, c_float
from numpy import empty, frombuffer, float32
from pandas import read_csv
from enumerations import *
from structs import *

##################################################
# Functions for the controller to work properly  #
##################################################

def import_libraries(local_dll, local_estrella):
	libdll = cdll.LoadLibrary(local_dll)
	libestrella = cdll.LoadLibrary(local_estrella)
	return libdll, libestrella

def create_xaxis(parameters_location):
	xaxis = empty(2051)
	calibration = read_csv(parameters_location,sep=' ',skiprows=3)
	for i in range(2051):
		xaxis[i] = (calibration.Parameters[1]/4.0)*(float(i*i)) + (calibration.Parameters[0]/2.0)*(float(i)) + calibration.Parameters[2]
	return xaxis

def create_yaxis(data):
	yaxis = frombuffer(data, float32)
	return yaxis

def estrella_begin(libdll, libestrella):
	
	# create a device list for the libraries
	device = c_void_p(None)
	numdevices = c_uint(0)
	devices = dll_list_t()
	libdll.dll_init(byref(devices))
	
	# create and try to set up an estrella session
	esession = estrella_session_t()
	rc = libestrella.estrella_find_devices(byref(devices))
	if rc != 0:
		print 'It was not possible to look for USB devices.\n(Are you root?)','\n'
		libdll.dll_clear(byref(devices))
		return 1
	
	rc = libdll.dll_count(byref(devices), byref(numdevices))
	if ((rc != 1) or (numdevices == 0)):
		print 'No device found.','\n'
		libdll.dll_clear(byref(devices))
		return 1
	
	rc = libdll.dll_get(byref(devices), byref(device), None, c_int(0))
	if (rc != 1):
		print 'It was not possible to acess the EPP2000 device.','\n'
		libdll.dll_clear(byref(devices))
		return 1
	
	rc = libestrella.estrella_init(byref(esession), device)
	if (rc != 0):
		print 'It was not possible to create the session.','\n'
		libestrella.estrella_close(byref(esession))
		libdll.dll_clear(byref(devices))
		return 1
	
	return devices,esession

def estrella_end(devices, esession, libdll, libestrella):
	libestrella.estrella_close(byref(esession))
	libdll.dll_clear(byref(devices))
	return 0

def estrella_setup(esession, devices, libdll, libestrella, XSMOOTH, XTMODE, TEMPCOMP, RATE, RESOLUTION):
	
	rc = libestrella.estrella_update(byref(esession), c_int(1), XSMOOTH, TEMPCOMP)
	if (rc != 0):
		print 'It was not possible to configure the data.','\n'
		libestrella.estrella_close(byref(esession))
		libdll.dll_clear(byref(devices))
		return 1
	
	rc = libestrella.estrella_rate(byref(esession), RATE, RESOLUTION);
	if (rc != 0):
		print 'It was not possible to configure the rates.','\n'
		libestrella.estrella_close(byref(esession))
		libdll.dll_clear(byref(devices))
		return 1
	
	rc = libestrella.estrella_mode(byref(esession), XTMODE)
	if (rc != 0):
		print 'It was not possible to configure operation mode.','\n'
		libestrella.estrella_close(byref(esession))
		libdll.dll_clear(byref(devices))
		return 1
	
	return 0

def estrella_nscan(devices, esession, libdll, libestrella):
	
	# create a C like float array and do the normal scan
	data = (c_float * 2051)()
	rc = libestrella.estrella_scan(byref(esession), byref(data))
	if (rc != 0):
		print 'The scan failed.','\n'
		libestrella.estrella_close(byref(esession))
		libdll.dll_clear(byref(devices))
		return 1
	
	# convert the scan result into a Numpy array
	counts = create_yaxis(data)
	
	return counts

def estrella_ascan(devices, esession, libdll, libestrella):
	rc = libestrella.estrella_async_scan(byref(esession))
	if (rc != 0):
		print 'Async scan failed.','\n'
		libestrella.estrella_close(byref(esession))
		libdll.dll_clear(byref(devices))
		return 1
	return 0

def estrella_aresult(devices, esession, libdll, libestrella):
	
	# create a C like float array retrieve the async result
	data = (c_float * 2051)()
	rc = libestrella.estrella_async_result(byref(esession), byref(data))
	if (rc != 0):
		print 'It was not possible to retrieve the data from scan.','\n'
		libestrella.estrella_close(byref(esession))
		libdll.dll_clear(byref(devices))
		return 1
	
	# convert the scan result into a Numpy array
	counts = create_yaxis(data)
	
	return counts
