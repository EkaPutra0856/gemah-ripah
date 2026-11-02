@echo off
rem Generates /Game/GemahRipah/Maps/L_Dev_Sandbox headless. Close the editor first.
set UE=E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe
set ROOT=%~dp0..
"%UE%" "%ROOT%\GemahRipah.uproject" -run=pythonscript -script="%ROOT%\Tools\Python\build_dev_map.py" -unattended -nosplash -nullrhi -stdout
