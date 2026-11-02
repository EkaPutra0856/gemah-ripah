@echo off
rem Compiles the editor target. Close the editor first (or use Live Coding with Ctrl+Alt+F11).
set ROOT=%~dp0..
"E:\UE_5.8\Engine\Build\BatchFiles\Build.bat" GemahRipahEditor Win64 Development "-Project=%ROOT%\GemahRipah.uproject" -WaitMutex
