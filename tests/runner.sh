#!/bin/bash

if [[ $# -eq 1 ]]; then
	suite=$1
	count=$(./tests/testrunner --suite "$suite" --tcount)
else
	count=$(./tests/testrunner --tcount)
fi


echo "c-ray tiny test runner v0.3"
echo "Running $count tests."

failed_tests=0

i=0; while [ $i -le $((count - 1)) ]; do
	if [[ -n "$suite" ]]; then
		output=$(./tests/testrunner --suite "$suite" --test $i)
	else
		output=$(./tests/testrunner --test $i)
	fi
	if [[ $? -eq 0 ]]
	then
		echo "$output"
	else
		echo "$output"
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
