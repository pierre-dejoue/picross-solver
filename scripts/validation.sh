#!/bin/bash
CLI_EXE=$1
OUT_CSV=$2
set -x
$CLI_EXE --validation -v --no-timing --max-nb-solutions 1024 inputs/invalid_input.txt inputs/example_input.txt inputs/test_pattern_*.txt inputs/domino_logic.txt | tee $OUT_CSV
