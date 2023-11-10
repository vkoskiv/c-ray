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

data["scene"]["outputFilePath"] = "scripts/scenes/"
data["scene"]["outputFileName"] = "lotsa_instances"

# Yoink primitives
data["scene"]["primitives"] = []

instancelist = data["scene"]["meshes"][1]["pick_instances"]
for i in range(1000000):
	skel = copy.deepcopy(instancelist[0])
	skel["materials"][0]["color"] = {'type': 'hsl', 'h': random.uniform(0,360), 's': 100, 'l': 75}
	skel["transforms"][1]["X"] = random.uniform(0, 10) - 5
	skel["transforms"][1]["Z"] = random.uniform(0, 40) - 2.5
	instancelist.append(skel)

#print("Dumping scene to subprocess...")
#proc = Popen('./bin/c-ray --asset-path input/ -s 25 -c 2 -j 0 --nodes 127.1,127.1:2323', stdin=PIPE, shell=True, bufsize=0)
#proc.stdin.write(json.dumps(data).encode())
#proc.communicate()
print(json.dumps(data))
