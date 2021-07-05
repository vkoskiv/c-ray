#!/bin/bash

echo "Compiling..."
rebuild/testing-on > /dev/null || exit
echo "Done, running tests."
if [[ $# -eq 1 ]]; then
	suite=$1
	count=$(./bin/c-ray --suite $suite --tcount | awk 'FNR==2 {print $0}')
else
	count=$(./bin/c-ray --tcount | awk 'FNR==2 {print $0}')
fi


echo "C-ray test framework v0.2"
echo "Running $count tests."

i=0; while [ $i -le $((count - 1)) ]; do
	if [[ ! -z "$suite" ]]; then
		output=$(./bin/c-ray --suite $suite --test $i)
	else
		output=$(./bin/c-ray --test $i)
	fi
	if [[ $? -eq 0 ]]
	then
		echo "$output" | awk 'FNR==2 { print $0 }'
	else
		echo "$output" | awk 'FNR==2 { printf "%s\n", $0 }'
		#echo -e "[\033[0;31mCRSH\033[0m]"
	fi
	i=$(( i + 1 ))
done
