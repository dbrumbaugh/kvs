#!/bin/bash

rm -r ./tests/data
mkdir ./tests/data
touch ./tests/data/testfile.store
touch ./tests/data/failfile.store
touch ./tests/data/readtest.store
touch ./tests/data/table.store

echo "This is a load of test data" >> ./tests/data/readtest.store
echo "1 2 3 4 5 6 7 8 9 0" >> ./tests/data/readtest.store
echo "And some more data to read..." >> ./tests/data/readtest.store
echo "11 12 13 14 15 16 17 18 19 20" >> ./tests/data/readtest.store
mkdir ./tests/data/faildir
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
