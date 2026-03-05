#!/bin/bash

#INPUT="../../../examples/ex5-1d.in"
#EXPECTED="../../../examples/ex5-1d.out"

#INPUT="../../../examples/ex10-2d.in"
#EXPECTED="../../../examples/ex10-2d.out"

#INPUT="../../../examples/ex1000-50d.in"
#EXPECTED="../../../examples/ex1000-50d.out"

INPUT="../../../examples/ex1000-50000-200.in"
EXPECTED="../../../examples/ex1000-50000-200.out"

TIMES_FILE="times.txt"
RUNS=100

> "$TIMES_FILE"

echo "Running $RUNS times..."

for i in $(seq 1 $RUNS); do
    { time ./docs "$INPUT" > output ; } 2> time_tmp.txt

    if ! diff output "$EXPECTED"; then
        echo "ERROR: Output mismatch on run $i"
        rm -f output time_tmp.txt
        exit 1
    fi

    grep real time_tmp.txt | awk '{print $2}' >> "$TIMES_FILE"
done

rm -f output time_tmp.txt

echo "All $RUNS runs passed!"
echo "Times -> $TIMES_FILE"
