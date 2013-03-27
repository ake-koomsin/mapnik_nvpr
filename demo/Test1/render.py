from mapnik import *
from time import clock
import os 

print "Start = " , clock()
time1 = clock();
mapfile = 'osm.xml'
map_output = 'mymap.png'

m = Map(800,600)
load_map(m, mapfile)
time2 = clock();
print "Load XML style = " , (time2-time1)

bbox=(Envelope( 10.0,47.5,11.1,48.1 ))
m.background = Color('white')

m.zoom_to_box(bbox)
time3 = clock();
print "draw = " , (time3-time2) 
render_to_file(m, map_output)
time4 = clock();
print "render = " , (time4-time3)
