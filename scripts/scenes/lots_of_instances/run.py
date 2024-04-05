#!/usr/bin/env python3

import copy
import json
import sys
from subprocess import *
from math import *
import random

#print("Generating scene...")
filename = 'scripts/scenes/lots_of_instances/hdr.json'

random.seed(1337)

with open(filename) as f:
	data = json.load(f)

data["renderer"]["outputFilePath"] = "scripts/scenes/"
data["renderer"]["outputFileName"] = "lotsa_instances"

# Yoink primitives
data["scene"]["primitives"] = []

instancelist = data["scene"]["meshes"][1]["pick_instances"]
for i in range(100000):
	skel = copy.deepcopy(instancelist[0])
	skel["for"] = "Venus"
	skel["materials"][0]["color"] = {'type': 'hsl', 'h': random.uniform(0,360), 's': 100, 'l': 75}
	skel["transforms"][1]["X"] = random.uniform(0, 10) - 5
	skel["transforms"][1]["Y"] = random.uniform(0, 10) - 5
	skel["transforms"][1]["Z"] = random.uniform(0, 40) - 2.5
	skel["transforms"].append({"type": "rotateX", "degrees": random.uniform(0, 360)})
	skel["transforms"].append({"type": "rotateY", "degrees": random.uniform(0, 360)})
	skel["transforms"].append({"type": "rotateZ", "degrees": random.uniform(0, 360)})
	instancelist.append(skel)

#print("Dumping scene to subprocess...")
proc = Popen('./bin/c-ray --asset-path input/ -s 25 -c 2', stdin=PIPE, shell=True, bufsize=1024)
proc.stdin.write(json.dumps(data).encode())
proc.communicate()
#print(json.dumps(data))
