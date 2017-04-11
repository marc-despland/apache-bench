#!/bin/bash

sleep 1
echo "Date : `date +%s`"
netstat -ano |grep 666
echo "========================================================"