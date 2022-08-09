rem gcc -o m68kmake m68kmake.c
rem .\m68kmake
rem g++ -I. -g -c softfloat.cc
rem g++ -I. -g -c fsincos.cc
rem g++ -I. -g -c fyl2x.cc
g++ -I. -g -c m68kcpu.cc
rem g++ -I. -g -c m68kdasm.cc
rem g++ -I. -g -c m68kopac.cc
rem g++ -I. -g -c m68kopac.cc
rem g++ -I. -g -c m68kopdm.cc
rem g++ -I. -g -c m68kopnz.cc
rem g++ -I. -g -c m68kops.cc
g++ -I. -g -c main.cc
g++ -I. -g -c file.cc
g++ -I. -g -c mem.cc
g++ -I. -g -c proc.cc
g++ -I. -g -c trap14.cc
g++ -I. -g -c diag.cc
g++ -I. -g -c util.cc

g++ -g -o pcsrun *.o
del /q /f environ\tmp\*
del /q /f environ\*.o

