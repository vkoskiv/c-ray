#!/usr/local/bin/python3

import json
import sys
from subprocess import *
from math import *

filename = 'scripts/animations/hdrseam.json'

with open(filename) as f:
    data = json.load(f)

data["scene"]["outputFilePath"] = "output/hdrseam/"

for i in range(360):
    data["scene"]["count"] = i
    data["camera"]["transforms"][2]["degrees"] = i

    proc = Popen('./bin/c-ray ', stdin=PIPE, shell=True)
    proc.stdin.write(json.dumps(data).encode())
    proc.communicate()
