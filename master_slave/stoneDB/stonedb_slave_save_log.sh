#!/bin/bash

sh stonedb_slave.sh 2>&1 | tee /tmp/stonedb-slave-log$(date '+%Y%m%d').log
