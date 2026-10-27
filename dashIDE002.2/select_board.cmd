@echo off
rem Python 3 selector; forwards the selected board without changing libraries.
where py >nul 2>&1
if not errorlevel 1 (
  py -3 "%~dp0select_board.py" %*
  exit /b
)
python "%~dp0select_board.py" %*
