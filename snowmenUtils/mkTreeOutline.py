#! /usr/bin/env python3

# Add an outline to the tree texture image.
# There are no command parameters. Instead, change the variables below.

LINE_WIDTH = "16"

IMAGE_SIZE = 512 # Image is square, so: x = y = IMAGE_SIZE

LINE_COLOR="black"

COUNT_OF_LINES = 16

IMAGE_FILENAME_SRC  = "orig_treeTexture.png"

IMAGE_FILENAME_DEST = "treeTexture.png"


import math
import subprocess

# Given a value in [-1,1], output the value in range [0, IMAGE_SIZE].
def unNormalize(n):
    return ((n+1) * IMAGE_SIZE/2)

paramList = ["convert", IMAGE_FILENAME_SRC]

drawList= []

for i in range(COUNT_OF_LINES):
    startA = math.tau * i / COUNT_OF_LINES
    endA   = math.tau * (i + 1) / COUNT_OF_LINES

    startX = unNormalize(math.sin(startA))
    startY = unNormalize(math.cos(startA))

    endX = unNormalize(math.sin(endA))
    endY = unNormalize(math.cos(endA))

    drawList.append("-draw")
    drawList.append("line %s,%s %s,%s" % (startX,startY , endX,endY))

paramList.extend(["-stroke", LINE_COLOR,
                  "-strokewidth", LINE_WIDTH])

paramList.extend(drawList)

paramList.append(IMAGE_FILENAME_DEST)

subprocess.run(paramList)

# If there is an error then we'll see it. Not going to bother with error-checking
