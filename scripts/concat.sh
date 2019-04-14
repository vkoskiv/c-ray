#!/bin/bash

# vkoskiv, 30.3.2019
# ffmpeg prefers a nice ordered list of files with the following format
# This script creates that, it's then passed to ffmpeg and deleted after
# file '/path/to/file'\n

if [[ "$1" != "" ]]; then
	IMGDIR="$1"
else
	IMGDIR=.
fi

list=$(ls -1 $IMGDIR)

while read -r line; do
	echo "file '$IMGDIR$line'" >> list.txt
done <<< "$list"
