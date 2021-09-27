#!/usr/local/bin/python3

import json
import sys
from subprocess import *
from math import *

def lerp(a, b, t):
	return (1 / t) * a + t * b

filename = 'input/hdr.json'

with open(filename) as f:
    data = json.load(f)

data["renderer"]["outputFilePath"] = "output/bezier/"
data["renderer"]["samples"] = 400
data["camera"]["time"] = 0

frames = 480
time = 0

for i in range(frames):
    time += 1 / frames
    data["renderer"]["count"] = i
    data["camera"]["time"] = time
    #data["camera"]["focalDistance"] = lerp(0.7, 0.4, time)

    proc = Popen('./bin/c-ray --asset-path input/ ', stdin=PIPE, shell=True)
    proc.stdin.write(json.dumps(data).encode())
    proc.communicate()
