@echo off

REM	----- A Windows 'Makefile' sort of deal because Windows is
REM	----- woefully incompatible with every possible make system in
REM ----- existence.
REM ______________________________________________________________




REM ----- VARIABLES
REM ______________________________________________________________

REM ----- Make settings/variables
set CFLAGS=  /nologo /W0
set LDFLAGS= /link /nologo /subsystem:windows user32.lib shell32.lib advapi32.lib d3d9.lib
set OBJS= rosgb.c cpu.c ppu.c tigr.c




REM ----- BUILD
REM ______________________________________________________________

REM ----- Run this first:
	REM vcvars32

REM ----- Target: 'clean'
if "%1"=="clean" (
	del /q *.obj
	exit /b
)

REM ----- Target: Default/all
(
	REM ----- REM Create objects from *.c
	REM for /r %%x in (%OBJS%) do (
	REM	cl /nologo /c %%x %CFLAGS%
	REM )

	REM ----- Create objects from *.asm
	nasm -fwin32 bt.asm

	REM ----- Build target
	cl %CFLAGS% %OBJS% bt.obj %LDFLAGS%
)
