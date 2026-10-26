@echo off
rem Run the native selector window using Python 3, without changing libraries.
where py >nul 2>&1
if not errorlevel 1 (
  py -3 "%~dp0select_board_gui.py" %*
  exit /b
)
python "%~dp0select_board_gui.py" %*
