#!/bin/bash

PATH=$ERESSEA/bin:$PATH

function abort() {
  if [ $# -gt 0 ]; then 
    echo $@
  fi
  exit -1
}
