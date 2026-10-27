"""Locate and start Arduino IDE with the project, without a shell or build/upload.

An explicitly chosen executable or ARDUINO_IDE environment variable takes
priority. Otherwise check PATH and common desktop installation locations.
The executable path is one argument, not a shell command with flags: spaces
in both the IDE path and sketch path therefore remain unambiguous.
"""
import os
from pathlib import Path
import shutil
import subprocess
import sys


class IdeNotFoundError(FileNotFoundError):
    """No usable IDE executable was found; a GUI may offer a file chooser."""


def _usable(path, platform):
    """Ignore inaccessible candidate directories rather than fail discovery."""
    try:
        return path.is_file() and (platform=='win32' or os.access(path,os.X_OK))
    except OSError:
        return False


def find_ide(executable=None, *, platform=None, environ=None):
    """Resolve an executable without launching apps or changing the project."""
    platform=platform or sys.platform
    environ=os.environ if environ is None else environ
    explicit=executable or environ.get('ARDUINO_IDE')
    if explicit:
        path=Path(os.path.expandvars(str(explicit))).expanduser()
        if _usable(path,platform):return str(path.resolve())
        found=shutil.which(str(explicit))
        if found:return found
        raise IdeNotFoundError('Arduino IDE executable not found or not executable: '+str(explicit))
    for name in ('arduino-ide','Arduino IDE','arduino'):
        found=shutil.which(name)
        if found:return found
    candidates=[]
    if platform=='win32':
        for variable in ('LOCALAPPDATA','ProgramFiles','ProgramFiles(x86)'):
            base=environ.get(variable)
            if not base:continue
            root=Path(base)
            candidates.extend([
                root/'Arduino IDE'/'Arduino IDE.exe',
                root/'Programs'/'Arduino IDE'/'Arduino IDE.exe',
                root/'Arduino'/'arduino.exe',
            ])
    elif platform=='darwin':
        candidates.append(Path('/Applications/Arduino IDE.app/Contents/MacOS/Arduino IDE'))
    else:
        candidates.extend([
            Path('/opt/arduino-ide/arduino-ide'),
            Path('/opt/arduino-ide/arduino-ide.AppImage'),
            Path('/opt/arduino-ide.AppImage'),
        ])
    for path in candidates:
        if _usable(path,platform):return str(path.resolve())
    raise IdeNotFoundError('Arduino IDE was not found. Use IDE executable... to choose it, or set ARDUINO_IDE.')


def launch_ide(project, executable=None):
    """Request opening the sketch and return its process handle for status checks.

    The IDE may forward the request to an existing instance and exit normally.
    Launching never chooses a serial port, compiles, flashes, or alters Tools
    settings. A separate process group allows the selector to close afterwards.
    """
    project=Path(project).resolve()
    sketch=project/'dashIDE002.2.ino'
    if not sketch.is_file():raise FileNotFoundError('Sketch not found: '+str(sketch))
    ide=find_ide(executable)
    options={'cwd':str(project),'stdin':subprocess.DEVNULL,
             'stdout':subprocess.DEVNULL,'stderr':subprocess.DEVNULL}
    if os.name=='nt':
        options['creationflags']=subprocess.CREATE_NEW_PROCESS_GROUP
    else:
        options['start_new_session']=True
    return subprocess.Popen([ide,str(sketch)],**options)
