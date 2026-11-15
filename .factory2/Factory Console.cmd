@echo off
python -B "%~dp0console.pyz" --desktop-setup
if errorlevel 1 pause
