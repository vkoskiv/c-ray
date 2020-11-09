#!/bin/sh

echo "Compiling..."
rebuild/testing-on 2>&1 > /dev/null || exit
echo "Done, running tests."
count=$(./bin/c-ray --tcount | awk 'FNR==2 {print $0}')

echo "C-ray test framework v0.2"
echo "Running $count tests."

i=0; while [ $i -le $(($count - 1)) ]; do
	output=$(./bin/c-ray --test $i)
	if [[ $? -eq 0 ]]
	then
		echo "$output" | awk 'FNR==2 { print $0 }'
	else
		echo "$output" | awk 'FNR==2 { printf "%s", $0 }'
		echo "[\033[0;31mCRSH\033[0m]"
	fi
	i=$(( i + 1 ))
done
