#!/bin/bash

echo "Compiling..."
rebuild/testing-on > /dev/null || exit
echo "Done, running tests."
if [[ $# -eq 1 ]]; then
	suite=$1
	count=$(./bin/c-ray --suite "$suite" --tcount | awk 'FNR==2 {print $0}')
else
	count=$(./bin/c-ray --tcount | awk 'FNR==2 {print $0}')
fi


echo "C-ray test framework v0.2"
echo "Running $count tests."

failed_tests=0

i=0; while [ $i -le $((count - 1)) ]; do
	if [[ -n "$suite" ]]; then
		output=$(./bin/c-ray --suite "$suite" --test $i)
	else
		output=$(./bin/c-ray --test $i)
	fi
	if [[ $? -eq 0 ]]
	then
		echo "$output" | awk 'FNR==2 { print $0 }'
	else
		echo "$output" | awk 'FNR==2 { printf "%s\n", $0 }'
		failed_tests=$(( failed_tests + 1 ))
	fi
	i=$(( i + 1 ))
done
echo "Test suite finished. $((i - failed_tests))/$i tests passed."
if [[ $failed_tests -ne 0 ]]
then
	exit 1
else
	exit 0
fi
