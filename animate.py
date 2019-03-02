#!/usr/bin/python

import json
import os

filename = './input/animate.json'

with open(filename) as f:
    data = json.load(f)

data["scene"]["outputFilePath"] = "output/instanssi/"
data["renderer"]["sampleCount"] = 5

for i in range(50):
    data["scene"]["count"] = i
    data["camera"]["transforms"][0]["x"] = data["camera"]["transforms"][0]["x"] + 5

    os.remove(filename)
    with open(filename, 'w') as f:
        json.dump(data, f, indent=4)

    os.system('./bin/c-ray ' + filename)
