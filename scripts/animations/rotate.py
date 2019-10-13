#!/usr/local/bin/python3

import json
import sys
from subprocess import *
from math import *

filename = 'input/scene.json'

with open(filename) as f:
    data = json.load(f)

data["scene"]["outputFilePath"] = "output/rotate/"
data["renderer"]["sampleCount"] = 3
data["renderer"]["bounces"] = 12
data["display"]["enabled"] = False

for i in range(360):
    data["scene"]["count"] = i
    data["scene"]["meshes"][0]["transforms"][1]["degrees"] -= 1
    proc = Popen('./bin/c-ray ', stdin=PIPE, shell=True)
    proc.stdin.write(json.dumps(data).encode())
    proc.communicate()
