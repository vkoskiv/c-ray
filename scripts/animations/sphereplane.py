#!/usr/local/bin/python3

import json
import sys
from subprocess import *
from math import *

filename = 'input/scene.json'

with open(filename) as f:
    data = json.load(f)

data["scene"]["outputFilePath"] = "output/20sample/"
data["renderer"]["sampleCount"] = 10
data["renderer"]["bounces"] = 12
data["camera"]["transforms"][0]["x"] = 675

for i in range(240):
    data["scene"]["count"] = i
    data["camera"]["transforms"][0]["x"] += 2.5
    data["camera"]["transforms"][0]["y"] += 10*sin(data["camera"]["transforms"][0]["x"]/10)

    proc = Popen('./bin/c-ray ', stdin=PIPE, shell=True)
    proc.stdin.write(json.dumps(data).encode())
    proc.communicate()
