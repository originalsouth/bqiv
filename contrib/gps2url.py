#!/usr/bin/python
# coding=utf-8

# small script to extract GPS coordinates from an image and
# call firefox with google maps url if the GPS info exists
# 2012 Thomas Wiegner - GPL

from PIL import Image
from PIL.ExifTags import TAGS
from subprocess import call
import sys,os

def get_exif(fn):
  if os.path.isfile(fn) :
    im = Image.open(fn)
  else :
    return -1
  if im.format == "JPEG" :
    info = im._getexif()
  else :
    return 0
  if info :
    for tag, value in info.items():
      decoded = TAGS.get(tag, tag)
      if decoded == "GPSInfo" :
        return value
  return 0

if len(sys.argv) < 2 :
  print "Usage", sys.argv[0], "<image>"
  exit(0)

exif=get_exif(sys.argv[1])

if exif == 0:
  print "No GPS info available in image"
  exit(0)
if exif == -1:
  print "File", sys.argv[1], "does not exist"
  exit(0)

for k,v in exif.items():
  if k == 1:
    lat_d = v
  if k == 3:
    lng_d = v
  if k == 2:
    lat_v = str(v[0][0]/v[0][1]) + unicode("°", "utf8") + str(v[1][0]/v[1][1]) + "'" + str(1.0* v[2][0]/v[2][1]) + "''"
  if k == 4:
    lng_v = str(v[0][0]/v[0][1]) + unicode("°", "utf8") + str(v[1][0]/v[1][1]) + "'" + str(1.0* v[2][0]/v[2][1]) + "''"

try:
  lat_d and lng_d and lat_v and lng_v
except NameError:
  print "No GPS location data defined"
else:
  # make sure firefox is already running
  url="http://maps.google.de/maps?q="+lat_v+lat_d+lng_v+lng_d
  call(["firefox", url])

