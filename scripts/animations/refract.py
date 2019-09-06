#!/usr/local/bin/python3

import json
import sys
from subprocess import *
from math import *

filename = 'scripts/animations/refract.json'

with open(filename) as f:
    data = json.load(f)

data["scene"]["outputFilePath"] = "output/refract/"

for i in range(480):
    data["scene"]["count"] = i
    data["scene"]["primitives"][0]["pos"]["z"] -= 0.005

    proc = Popen('./bin/c-ray ', stdin=PIPE, shell=True)
    proc.stdin.write(json.dumps(data).encode())
    proc.communicate()
