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
    
