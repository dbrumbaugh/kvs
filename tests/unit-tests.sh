#!/bin/bash

rm -r ./tests/data
mkdir ./tests/data
mkdir ./tests/data/block


echo "Running unit tests:"

for i in tests/*_tests
do
    $i 2>&1 | tee -a tests/tests.log
done

echo ""
