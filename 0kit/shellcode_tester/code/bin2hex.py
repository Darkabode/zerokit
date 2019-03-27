'''
Created on 28.07.2011

@author: SIN
'''
import sys
import os
import binascii

print "Start Generator"
    
arg = sys.argv

if arg[1] == None:
    print "Please enter argument (file name)\n"
    exit(-1)
filename = arg[1]
if not os.path.isfile(filename):
    print "File not found\n"
    exit(-1)
filesize = os.path.getsize(filename)
readfp = open(filename,'r+b')

data = readfp.read(filesize)
result = ''

sh_name = "bin_data"

if len(arg) > 3:
    if arg[3] != None:
        sh_name = arg[3];

result += "unsigned char " + sh_name + "["
result += str(filesize)
result += "] {\n\t"

k = 0
for i in range(0, filesize):
    result += '0x'+binascii.b2a_hex(data[i])+','
    k = k + 1
    if k >= 16:
        result += "\n\t"
        k = 0

result += "\n"
result += "};\n"

if len(arg) > 2:
    if arg[2] != None:
        wrtiefp = open(arg[2],'w+')
        wrtiefp.write(result)
        wrtiefp.close()
else:
    print result

readfp.close()
