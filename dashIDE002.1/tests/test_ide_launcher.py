"""Validate IDE discovery and argv handling without opening a desktop app."""
from pathlib import Path
import os
import sys
import tempfile
import unittest
from unittest import mock

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
import ide_launcher


class IdeLauncherTests(unittest.TestCase):
    def test_explicit_executable_with_spaces(self):
        with tempfile.TemporaryDirectory(prefix='Arduino executable ') as directory:
            executable=Path(directory)/'Arduino IDE'
            executable.write_text('placeholder');executable.chmod(0o755)
            self.assertEqual(ide_launcher.find_ide(executable),str(executable.resolve()))
            self.assertEqual(ide_launcher.find_ide(environ={'ARDUINO_IDE':str(executable)}),str(executable.resolve()))

    def test_path_and_windows_installation(self):
        with mock.patch.object(ide_launcher.shutil,'which',return_value='/test/arduino-ide'):
            self.assertEqual(ide_launcher.find_ide(environ={}),'/test/arduino-ide')
        with tempfile.TemporaryDirectory() as directory:
            executable=Path(directory)/'Programs'/'Arduino IDE'/'Arduino IDE.exe'
            executable.parent.mkdir(parents=True);executable.write_text('placeholder')
            with mock.patch.object(ide_launcher.shutil,'which',return_value=None):
                self.assertEqual(ide_launcher.find_ide(platform='win32',environ={'LOCALAPPDATA':directory}),str(executable.resolve()))
                with self.assertRaises(ide_launcher.IdeNotFoundError):
                    ide_launcher.find_ide('missing-IDE',environ={})

    def test_missing_sketch_does_not_launch(self):
        with tempfile.TemporaryDirectory() as directory:
            with mock.patch.object(ide_launcher,'find_ide') as find:
                with self.assertRaises(FileNotFoundError):ide_launcher.launch_ide(directory)
                find.assert_not_called()

    @unittest.skipUnless(os.name=='posix','Executable stub uses a POSIX shebang')
    def test_launch_passes_exact_sketch_path_without_shell(self):
        with tempfile.TemporaryDirectory(prefix='IDE launch spaces ') as directory:
            project=Path(directory)/'project with spaces';project.mkdir()
            sketch=project/'dashIDE002.1.ino';sketch.write_text('// test sketch')
            executable=Path(directory)/'Arduino IDE'
            executable.write_text('#!/usr/bin/env python3\nimport sys\nfrom pathlib import Path\nPath("opened-sketch.txt").write_text(sys.argv[1])\n')
            executable.chmod(0o755)
            process=ide_launcher.launch_ide(project,executable)
            self.assertEqual(process.wait(timeout=5),0)
            self.assertEqual((project/'opened-sketch.txt').read_text(),str(sketch.resolve()))


if __name__=='__main__':unittest.main()
