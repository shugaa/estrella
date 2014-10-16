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
# Python Controller, enumerations.
# 

from ctypes import c_int

##################################################
# Enumerations for the libraries in python shape #
##################################################

# maximum path string length
ESTRELLA_PATH_MAX = c_int(256)

# spectrometer rate
ESTRELLA_RATE = c_int(50)

# library error codes
ESTROK = c_int(0)
ESTRERR = c_int(1)
ESTRINV = c_int(2)
ESTRNOMEM = c_int(3)
ESTRTIMEOUT = c_int(4)
ESTRNOTIMPL = c_int(5)
ESTRALREADY = c_int(6)

# values for enumeration 'estr_xtmode_t'
ESTR_XTMODE_NORMAL = c_int(0)
ESTR_XTMODE_TRIGGER = c_int(1)
ESTR_XTMODE_TYPES = c_int(2)

# values for enumeration 'estr_xtrate_t'
ESTR_XRES_LOW = c_int(0)
ESTR_XRES_MEDIUM = c_int(1)
ESTR_XRES_HIGH = c_int(2)
ESTR_XRES_TYPES = c_int(3)

# values for enumeration 'estr_xsmooth_t'
ESTR_XSMOOTH_NONE = c_int(0)
ESTR_XSMOOTH_5PX = c_int(1)
ESTR_XSMOOTH_9PX = c_int(2)
ESTR_XSMOOTH_17PX = c_int(3)
ESTR_XSMOOTH_33PX = c_int(4)
ESTR_XSMOOTH_TYPES = c_int(5)

# values for enumeration 'estr_tempcomp_t'
ESTR_TEMPCOMP_OFF = c_int(0)
ESTR_TEMPCOMP_ON = c_int(1)
ESTR_TEMPCOMP_TYPES = c_int(2)

# values for enumeration 'estrella_devicetype_t'
ESTRELLA_DEV_USB = c_int(0)
ESTRELLA_DEV_LPT = c_int(1)
