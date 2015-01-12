HomeComputer 6502
=================

This is my attempt to build a simple, mobile microcomputer system with an 8-bit MOS 6502 CPU that was used in many popular
home computers of the 1970s and 1980s like the Commodore 64 or the Apple II.

The idea was to design a computer like the C64, almost only using parts that were available when this computer was
manufactured.

The hardware is designed with [Eagle](http://www.cadsoft.de/eagle-pcb-design-software/).

The software is written in 6502 assembler and C, with the help of the [CC65 compiler/assembler](http://cc65.github.io/cc65/).

The case is designed with [Autodesk 123D Design](http://www.123dapp.com/design).

Documentation of this project can be found on my website: http://www.grappendorf.net/projects/6502-homecomputer

This repositiory contains the following files:

* eagle - Eagle schematic and board files
* schematics - PDF schematics
* case - 123D/STL design files for the case
* firmware - Minimal BASIC interpreter written with CC65,  different versions of the firmware during the development process
* terminal -  Small ruby script for loading/saving of BASIC programs, example BASIC programs
