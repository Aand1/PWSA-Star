import sys
import random

def generate(seed, size, prob, outfile):
  random.seed(seed)
  with open(outfile, "w") as f:
    f.write("type octile\nheight %d\nwidth %d\nmap\n" % (size, size))
    f.write("." * size + "\n")
    for i in xrange(1, size-1):
      for j in xrange(0, size-1):
        if random.random() < prob:
          f.write("@")
        else:
          f.write(".")
      f.write(".")
      if i != size - 2:
        f.write("\n")

if __name__ == "__main__":
  generate(int(sys.argv[1]), int(sys.argv[2]), float(sys.argv[3]), sys.argv[4])
