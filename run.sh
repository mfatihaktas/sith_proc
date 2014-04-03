#!/bin/bash

echo $1

if [ $1  = 'run' ]; then
  make mfa_proc
  ./mfaproc
else
  echo "Argument did not match !"
fi
