@echo off
gcc -o mkconfig mkconfig.c
.\mkconfig %1 %2 %3 %4 %5
