#!/bin/bash

source ../functions.sh

#################################################
# Execute Solver Scripts
#################################################

logEcho "[Korali] Beginning solver tests"

for file in *.py
do
  logEcho "-------------------------------------"
  logEcho "Running File: ${file%.*}"
  
  python3 ./$file >> $logFile 2>&1
  check_result

  log "[Korali] Removing results..."
  rm -rf "_korali_result" >> $logFile 2>&1
  check_result

  logEcho "-------------------------------------"
done

