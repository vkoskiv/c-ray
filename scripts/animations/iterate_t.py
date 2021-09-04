#!/usr/local/bin/python3

import json
import sys
from subprocess import *
from math import *

filename = 'input/hdr.json'

with open(filename) as f:
    data = json.load(f)

data["renderer"]["outputFilePath"] = "output/bezier/"
data["renderer"]["samples"] = 10

for i in range(240):
    data["renderer"]["count"] = i
    data["camera"]["time"] += 1/240

    proc = Popen('./bin/c-ray --asset-path input/ ', stdin=PIPE, shell=True)
    proc.stdin.write(json.dumps(data).encode())
    proc.communicate()
