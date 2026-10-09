

cl /MT /O2 /c /W4 *.cpp argon2/*.c
pause


rc rsrc.rc


link /subsystem:windows /MACHINE:x64 kernel32.lib user32.lib Shell32.lib gdi32.lib comdlg32.lib rsrc.res *.obj /out:codepad.exe
pause

del *.obj, *.ilk, *.pdb, *.exp, *.lib
del bin\*.ilk

