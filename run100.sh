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
RUNS=10

> "$TIMES_FILE"

echo "Running $RUNS times..."

for i in $(seq 1 $RUNS); do
    FREQ_LOG=$(mktemp)

    sudo powermetrics --samplers cpu_power -i 500 -o "$FREQ_LOG" &
    POWERMETRICS_PID=$!

    { time ./docs "$INPUT" > output ; } 2> time_tmp.txt

    kill $POWERMETRICS_PID 2>/dev/null
    wait $POWERMETRICS_PID 2>/dev/null

    if ! diff -q output "$EXPECTED" > /dev/null; then
        echo "ERROR: Output mismatch on run $i"
        rm -f output time_tmp.txt "$FREQ_LOG"
        exit 1
    fi

    real_time=$(grep real time_tmp.txt | awk '{print $2}')

    avg=$(grep -i "cluster HW active frequency" "$FREQ_LOG" | awk '
        /E-Cluster/ { e+=$5; en++ }
        /P-Cluster/ { p+=$5; pn++ }
        END { printf "avg E-Cluster=%.0f MHz  avg P-Cluster=%.0f MHz", e/en, p/pn }
    ')

    log="Run $i | time=$real_time | $avg"
    echo "$log" | tee -a "$TIMES_FILE"

    rm -f "$FREQ_LOG"
done

rm -f output time_tmp.txt
echo ""
echo "All $RUNS runs passed!"
echo "Times -> $TIMES_FILE"
