#!/bin/bash
rm -rf 9091

mkdir 9091
cd 9091
mkdir Malinin
cd Malinin
date > Nikita
date --date="next Mon" > filedate.txt
cat Nikita filedate.txt > result.txt
cat result.txt