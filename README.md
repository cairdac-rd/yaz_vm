# yaz_vm
Fast register based Virtual Machine without JIT in C++

## Status
----------

A very basic (not finished) virtual machine to study the possibility to build a fast VM without using JIT.
Only the instructions necessary to excute the fibonacci function are coded for the moment.

yaz_vm.cpp use C arrays to store instructions.
yaz_vm2.cpp use std::vector to store instructions and contains more instructions

yaz_vm2.cpp code is slower than yaz_vm.cpp code.

## Building 
-----------
On linux :

``g++ -O3 yaz_vm.cpp -o yaz_vm``

