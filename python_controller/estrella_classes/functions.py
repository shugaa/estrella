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

from ctypes import cdll
from numpy import empty, frombuffer, float32
from pandas import read_csv

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
