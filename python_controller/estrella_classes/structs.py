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
# Python Controller, structures.
# 

from ctypes import c_ubyte, c_ushort, c_uint, c_int, c_char, c_char_p, c_void_p, c_size_t, Structure, Union, POINTER

#########################################
# Specific enumetations for the Classes #
#########################################

estr_xtmode_t = c_int
estr_xtrate_t = c_int
estr_xsmooth_t = c_int
estr_tempcomp_t = c_int
estrella_devicetype_t = c_int
estr_lock_t = c_int

################################################
# Structs for LIBDLL in Classes (python shape) #
################################################

class dll_item(Structure):
	pass
dll_item._fields_=[("data",c_void_p),
                   ("prev",POINTER(POINTER(dll_item))),
                   ("next",POINTER(POINTER(dll_item))),
                   ("datasize",c_size_t)]

class dll_list_t(Structure):
	_fields_= [("count",c_uint),
               ("first",POINTER(POINTER(dll_item))),
               ("last",POINTER(POINTER(dll_item)))]

##################################################
# Structs for ESTRELLA in Classes (python shape) #
##################################################

class estrella_usbdev_t(Structure):
	pass
estrella_usbdev_t._fields_ = [('bus', c_char * 256),
                              ('devnum', c_ubyte),
                              ('vendorid', c_ushort),
                              ('productid', c_ushort),
                              ('manufacturer', c_char * 128),
                              ('product', c_char * 128),
                              ('serialnumber', c_char * 32),]

class estrella_dev_t_u(Union):
	pass
estrella_dev_t_u._fields_ = [('usb', estrella_usbdev_t)]

class estrella_dev_t(Structure):
	pass
estrella_dev_t._fields_ = [('devicetype', estrella_devicetype_t),
                           ('spec', estrella_dev_t_u)]

class usb_dev_handle(Structure):
	pass
usb_dev_handle._fields_ = []

class estrella_session_t_u(Union):
	pass
estrella_session_t_u._fields_ = [('usb_dev_handle', POINTER(usb_dev_handle))]

class estrella_session_t(Structure):
	pass
estrella_session_t._fields_ = [('rate', c_int),
                               ('scanstoavg', c_int),
                               ('xtmode', estr_xtmode_t),
                               ('xtrate', estr_xtrate_t),
                               ('xsmooth', estr_xsmooth_t),
                               ('tempcomp', estr_tempcomp_t),
                               ('dev', estrella_dev_t),
                               ('spec', estrella_session_t_u),
                               ('lock', estr_lock_t)]

###################################################
# Structs for ESTRELLA USB Classes (python shape) #
###################################################

class estrella_usb_request_t(Structure):
	_fields_= [("requesttype",c_uint),
               ("request",c_uint),
               ("value",c_uint),
               ("index",c_uint),
               ("size",c_uint),
               ("data",c_char_p)]
