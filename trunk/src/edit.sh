#!/bin/bash
files=`ls *.c *.h | grep -v interface | grep -v support | grep -v eggtrayicon|grep -v callbacks`
gvim $files
