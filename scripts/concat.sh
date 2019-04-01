#!/bin/bash

# vkoskiv, 30.3.2019
# ffmpeg prefers a nice ordered list of files with the following format
# This script creates that, it's then passed to ffmpeg and deleted after
# file '/path/to/file'\n

list=$(ls -1 ../output/animation)

while read -r line; do
	echo "file '../output/animation/$line'" >> list.txt
done <<< "$list"
