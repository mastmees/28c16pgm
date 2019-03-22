# MIT License
#
# Copyright (c) 2019 Madis Kaal
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import serial
import sys
from binascii import hexlify
import time

port=serial.Serial(sys.argv[1],9600,timeout=0.2)

bin=open(sys.argv[2],"r").read()
if len(bin)!=2048:
  print "Invalid length"
else:
  a=0
  while a<2048:
    r=bytearray((16,a>>8,a&255,0))
    r.extend(bin[a:a+16])
    c=0
    for b in r:
      c+=b
    r.extend((((~c)+1)&255,))
    port.write(":%s\r\n"%hexlify(r).upper())
    port.flush()
    time.sleep(0.2)
    print(":%s"%hexlify(r).upper())
    a+=16
  port.write(":00000001FF\r\n")
  port.flush()
  
port.close()
