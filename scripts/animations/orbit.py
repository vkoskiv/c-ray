#!/usr/local/bin/python3

import json
import sys
from subprocess import *
from math import *

filename = 'input/hdr.json'

with open(filename) as f:
    data = json.load(f)

data["scene"]["outputFilePath"] = "output/orbit/"
data["renderer"]["sampleCount"] = 3
#data["camera"]["transforms"][0]["z"] = 0

r = 0.05

for i in range(360):
    data["scene"]["count"] = i
    #rotate
    data["camera"]["transforms"][2]["degrees"] = -i
    #move
    data["camera"]["transforms"][0]["x"] += cos(radians(i))*r
    data["camera"]["transforms"][0]["z"] += sin(radians(i))*r

    proc = Popen('./bin/c-ray ', stdin=PIPE, shell=True)
    proc.stdin.write(json.dumps(data).encode())
    proc.communicate()
