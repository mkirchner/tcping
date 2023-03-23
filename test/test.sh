#!/bin/bash

LOG=test-log.$$
touch $LOG

# usage
( ./tcping 2>&1 ) >> $LOG

# gethostbyname
( ./tcping -u 1000 this.host.does.not.exist 22 2>&1 ) >> $LOG

# user timeout
( ./tcping -u 1000 10.0.0.1 22 2>&1 ) >> $LOG

# port closed
( ./tcping -u 1000 localhost 22222 2>&1 ) >> $LOG

# port open use cases
( ./tcping -u 1000 localhost 22 2>&1 ) >> $LOG
( ./tcping -u 1000 127.0.0.1 22 2>&1 ) >> $LOG

diff $LOG test/expected
TESTRESULT=`echo $?`
rm -f $LOG
if [ $TESTRESULT -ne 0 ]; then
  echo "Failure in test suite."
  exit 1
else
  echo "All tests completed successfully."
  exit 0
fi
