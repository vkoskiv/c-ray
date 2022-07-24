#!/bin/sh

if [ $# -eq 0 ]
then
	echo "Usage: scripts/interactive_debug.sh <path to input json>" && exit 1
fi

while true
do
	find . -name '*.c' -or -name '*.h' -or -name '*.json' | entr -c -d bash -c "make && ./bin/c-ray $1 -s 1 && open output/rendered_0000.png" || echo "You need to have entr installed to use this script." && exit 1
done
