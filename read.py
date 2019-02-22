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
from binascii import unhexlify

rom=bytearray(2048)
counter=1
port=serial.Serial(sys.argv[1],9600)

def clear():
  for i in range(len(rom)):
    rom[i]=0xff
    
while True:
  l=port.readline().strip()
  print l
  if l.startswith(":"):
    bytes=bytearray(unhexlify(l[1:]))
    cs=0
    for b in bytes:
      cs+=b
    if (cs&255)==0:
      if bytes[3]==0:
        a=(bytes[1]<<8)|bytes[2]
        for i in range(bytes[0]):
          rom[a+i]=bytes[i+4]
      elif bytes[3]==1:
        fn="rom-"+sys.argv[2]+".%04d.bin"%counter
        f=open(fn,"w")
        f.write(rom)
        f.close()
        print "Wrote %d bytes to %s"%(len(rom),fn)
        counter+=1
        clear()
    else:
      print("bad checksum")      
    
