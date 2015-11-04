#!/usr/bin/python

import sys
from PIL import Image, ImageDraw

cellsize = 3

infile = sys.argv[1]
outfile = sys.argv[2]

white = (255, 255, 255)
black = (0,0,0)
red = (255,0,0)
green = (0,255,0)
blue = (0,0,255)
cyan = (0,255,255)
yellow = (255,255,0)
magenta = (255,0,255)
orange = (255,140,0)
pink = (255,105,180)

colors = [green, red, blue, cyan, yellow, magenta, orange, pink]


def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

lines = fileGetLines(infile)
width = int(lines[0].split(' ')[1])
height = int(lines[1].split(' ')[1])

grid = [0]*height
for i in xrange(len(grid)):
  grid[i] = [0]*width

image = Image.new("RGB", (width*cellsize, height*cellsize), white)
draw = ImageDraw.Draw(image)

for row in xrange(0, height):
  line = lines[row + 2]
  cols = line.split(' ')
  for col in xrange(0, len(cols)):
    c = cols[col]
    color = white
    if (c == '@'):
      color = black
    elif (c == "."):
      color = white
    elif (c == 's'):
      color = white
    elif (c == 't'):
      color = white
    else:
      color = colors[int(c)]
    if (c == 's'):
      draw.text([cellsize*col, cellsize*row], "SOURCE", fill=black)
    if (c == 't'):
      draw.text([cellsize*col, cellsize*row], "TARGET", fill=black)
    draw.rectangle([cellsize*col, cellsize*row, cellsize*(col+1), cellsize*(row+1)], color)

coords = map(lambda x: x.split(), lines[(height+2):])
half = cellsize / 2
for i in xrange(0, len(coords) - 1):
  [r1,c1] = coords[i]
  [r2,c2] = coords[i+1]
  draw.line([(cellsize*int(c1)+half,cellsize*int(r1)+half),(cellsize*int(c2)+half,cellsize*int(r2)+half)], black)

image.save(outfile)
