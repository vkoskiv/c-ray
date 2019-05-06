#!/usr/local/bin/python3

import json
import sys
from subprocess import *

filename = 'input/scene.json'

with open(filename) as f:
    data = json.load(f)

data["scene"]["outputFilePath"] = "output/20sample/"
data["renderer"]["sampleCount"] = 100
data["camera"]["transforms"][0]["x"] = 825

for i in range(120):
    data["scene"]["count"] = i
    data["camera"]["transforms"][0]["x"] = data["camera"]["transforms"][0]["x"] + 2.5

    proc = Popen('./bin/c-ray ', stdin=PIPE, shell=True)
    proc.stdin.write(json.dumps(data).encode())
    proc.communicate()
