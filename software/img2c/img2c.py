#!/usr/bin/python
# img2c.py - convert a bitmap into a c source file for inclusion into lcr meter
# Copyright 2014 - 2015 Herman Moolenaar 
# This file is part of LCR-Meter.
#
# LCR-Meter is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LCR-Meter is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LCR-Meter.  If not, see <http://www.gnu.org/licenses/>.

import sys
import os
from PIL import Image
from os.path import basename

# Get the total number of args passed to the img2c.py
total = len(sys.argv)

if total != 4:
    print ('img2c.py - convert a bitmap, prefurably in the ppm format, into a c source file for inclusion into the LCR meter project')
    print ('parameter error - invoke with: img2c <characters in image> <bmp image file> <output c file>')
    print ('for example: img2c "0123456789-." font24x20.ppm font24x20.c')
    print ('the width of the image must be dividable by the number of characters in the character string')
    print ('bmp2c will now quit.')
    exit(1)

imagePath = sys.argv[2]
if not os.path.exists(imagePath):
    sys.stderr.write('ERROR: bitmap imagePath not found!\n\r')
    exit(1)

characterstring = sys.argv[1]
NumChars = len(characterstring)

if NumChars < 1:
    sys.stderr.write('ERROR: need at least 1 character in character string!\n\r')
    exit(1)

outputFilename = sys.argv[3]
outputFile = open(outputFilename, 'w')

print ("number of characters to process: {}".format(NumChars))
print ("image to proccess: {}".format(imagePath))
print ("output file: {}".format(outputFilename))
       
fontImage = Image.open(imagePath)

width = fontImage.size[0]
height = fontImage.size[1]

print ("image width = {0}, height = {1}".format(width, height))

if (width % NumChars != 0):
    sys.stderr.write('ERROR: the width of %d pixels does not match the number of characters!\n\r')
    exit(1)

nrChars = height / 8
if height % 8 != 0:
    nrChars += 1

imageData = [[0] * nrChars for i in range(width)]

for x in range(width):
    value = 0
    bitnr = 0
    for y in range(height):
        rgb = fontImage.getpixel((x, height - y - 1))
        test = rgb[0] + rgb[1] + rgb[2]
        value |= (1 << bitnr) if test < 3 * 128 else 0
        bitnr += 1
        if (bitnr >= 8) or (y == height - 1):
            bitnr = 0
            imageData[x][y / 8] = value
            value = 0

outputFile.write('const __flash uint8_t * const __flash {}[] =\n\r'.format(basename(imagePath)))
outputFile.write("   (const __flash uint8_t []){{ {}, 0xff, 0xff, 0xff, 0x00}},\n\r".format(nrChars))                 
for n in range(NumChars):
    outputFile.write("   (const __flash uint8_t []){{ '{}', {}".format(characterstring[n], width / NumChars))
    for x in range((width / NumChars) * n, (width / NumChars) * (n + 1)):
        for y in range(nrChars):
            outputFile.write(", 0x{:02x}".format(imageData[x][y]))
    outputFile.write("},\n\r")

outputFile.write('};\n\r')

print ("ready!") 
