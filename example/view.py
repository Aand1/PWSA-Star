import sys
from PIL import Image, ImageDraw

cellsize = 3

fname = sys.argv[1]
solutionfile = sys.argv[2]
out = sys.argv[3]

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

with open(fname) as f:
  lines = [line.rstrip('\n') for line in f]
  width = int(lines[0].split(' ')[1])
  height = int(lines[1].split(' ')[1])
  grid = [0]*height
  for i in xrange(len(grid)):
    grid[i] = [0]*width

  image = Image.new("RGB", (width*cellsize, height*cellsize), white)
  draw = ImageDraw.Draw(image)

  lines = lines[2:]
  for row in xrange(len(lines)):
    line = lines[row]
    line = line.split(' ')
    for col in xrange(len(line)):

      color = white

      c = line[col]
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

def fileGetLines(filename):
  with open(filename, 'r') as f:
    return [line.rstrip('\n') for line in f.readlines()]

coords = map(lambda x: x.split(), fileGetLines(solutionfile))
half = cellsize / 2
for i in xrange(0, len(coords) - 1):
  [r1,c1] = coords[i]
  [r2,c2] = coords[i+1]
  draw.line([(cellsize*int(c1)+half,cellsize*int(r1)+half),(cellsize*int(c2)+half,cellsize*int(r2)+half)], black)

image.save(out)
