#!/bin/sh

rebuild/testing-on 2>&1 > /dev/null || exit

count=$(./bin/c-ray --tcount | awk 'FNR==2 {print $0}')

echo "C-ray test framework v0.2"
echo "Running $count tests."

i=0; while [ $i -le $(($count - 1)) ]; do
	./bin/c-ray --test $i | awk 'FNR==2 {print $0}'
	i=$(( i + 1))
done
