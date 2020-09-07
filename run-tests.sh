#!/bin/sh

rebuild/testing-on || exit

count=$(./bin/c-ray --tcount)

echo "C-ray test framework v0.2"
echo "Running $count tests."

i=0; while [ $i -le $(($count - 1)) ]; do
	./bin/c-ray --test $i
	i=$(( i + 1))
done
