#!/bin/bash

mkdir ./tests/data
touch ./tests/data/testfile.store
touch ./tests/data/failfile.store
chmod 000 ./tests/data/failfile.store

echo "Running unit tests:"

for i in tests/*_tests
do
    $i 2>&1 >> tests/tests.log
    if [ $? -ne 0  ]
    then
        echo "ERROR in test $i:"
        echo "------"
        tail tests/tests.log
        exit 1
    fi
done

echo ""
