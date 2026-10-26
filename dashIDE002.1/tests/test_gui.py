"""Test GUI helpers everywhere; run widget/application tests when Tk has a display.

All real selection writes are redirected into a temporary project. The tests
never launch Arduino IDE or access serial ports, credentials or installed libs.
"""
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest import mock

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
import select_board_gui as gui
import select_board
from board_profiles import PROFILES


class GuiHelperTests(unittest.TestCase):
    def test_names_and_ide_settings(self):
        self.assertIn('amarela2.8',gui.profile_label('esp32_ili9341_xpt2046'))
        self.assertEqual(gui.aliases_for('esp32s3_axs15231b_qspi'),['black','preta'])
        self.assertIn('OPI PSRAM',gui.settings_text('esp32s3_axs15231b_qspi'))
        self.assertIn('Default 4MB with spiffs',gui.settings_text('esp32_st7789_parallel_cst820'))

    def test_read_saved_legacy_and_invalid_names_without_writes(self):
        with tempfile.TemporaryDirectory() as directory:
            project=Path(directory)
            self.assertIsNone(gui.read_saved_profile(project))
            path=project/'board.txt';path.write_text('preta\n')
            self.assertEqual(gui.read_saved_profile(project),'esp32s3_axs15231b_qspi')
            self.assertEqual(path.read_text(),'preta\n')
            path.write_text('unknown\n');self.assertIsNone(gui.read_saved_profile(project))
            self.assertEqual(path.read_text(),'unknown\n')


class GuiWidgetTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if gui.tk is None:raise unittest.SkipTest('Tkinter is not installed')
        try:
            probe=gui.tk.Tk();probe.destroy()
        except gui.tk.TclError:raise unittest.SkipTest('No graphical display available')

    def setUp(self):
        self.temporary=tempfile.TemporaryDirectory(prefix='board GUI with spaces ')
        self.addCleanup(self.temporary.cleanup)
        self.project=Path(self.temporary.name)/'dashIDE002.1'
        shutil.copytree(ROOT,self.project,ignore=shutil.ignore_patterns('secrets.h','secrets.json','build','build_out','__pycache__','.git'))
        self.patch=mock.patch.object(select_board,'ROOT',self.project)
        self.patch.start();self.addCleanup(self.patch.stop)
        self.window=gui.tk.Tk();self.addCleanup(self.window.destroy)
        self.app=gui.BoardSelectorApp(self.window,self.project)
        self.window.update()

    def snapshot(self):
        return {p.relative_to(self.project):p.read_bytes() for p in self.project.rglob('*') if p.is_file()}

    def test_open_preview_and_copy_are_read_only(self):
        before=self.snapshot()
        for index,board in enumerate(PROFILES):
            self.app.combo.current(index);self.app.preview();self.window.update()
            self.assertEqual(self.app.selected,board)
            self.app.copy_settings()
            self.assertEqual(self.window.clipboard_get(),gui.settings_text(board))
            self.assertEqual(self.snapshot(),before)
        # Check the largest settings view fits inside the requested window.
        self.app.combo.current(self.app.boards.index('esp32s3_axs15231b_qspi'))
        self.app.preview();self.window.update()
        self.assertLessEqual(self.app.apply_button.winfo_rooty()+self.app.apply_button.winfo_height(),
                             self.window.winfo_rooty()+self.window.winfo_height())

    def test_apply_uses_shared_selector_for_all_profiles(self):
        before=self.snapshot();number=int((self.project/'build_nr.txt').read_text())
        writable={'selected_board.h','build_opt.h','board.txt','build_nr.txt','build.h'}
        for index,board in enumerate(PROFILES):
            self.app.combo.current(index);self.app.preview()
            self.assertTrue(self.app.apply());number+=1
            self.assertEqual((self.project/'board.txt').read_text().strip(),board)
            self.assertEqual(int((self.project/'build_nr.txt').read_text()),number)
            self.assertIn(board,self.app.current.get())
            self.assertIn('Reopen the sketch',self.app.status.get())
            for relative,contents in before.items():
                if str(relative) not in writable:self.assertEqual((self.project/relative).read_bytes(),contents)
        self.assertFalse((self.project/'secrets.h').exists())

    def test_errors_are_reported_without_closing_window(self):
        before=self.snapshot()
        with mock.patch.object(select_board,'select',side_effect=PermissionError('Read-only project')):
            with mock.patch.object(gui.messagebox,'showerror') as error:
                self.assertFalse(self.app.apply());error.assert_called_once()
        self.assertEqual(self.snapshot(),before)
        self.assertEqual(self.window.winfo_exists(),1)
        self.assertIn('Could not save',self.app.status.get())


    def test_default_ide_path_is_visible_and_editable(self):
        before=self.snapshot()
        with mock.patch.object(gui,'find_ide',return_value='/opt/Arduino IDE/ide.AppImage'):
            window=gui.tk.Toplevel(self.window)
            try:
                app=gui.BoardSelectorApp(window,self.project)
                self.assertEqual(app.ide_entry.get(),'/opt/Arduino IDE/ide.AppImage')
                app.ide_entry.delete(0,'end')
                app.ide_entry.insert(0,'/custom/IDE')
                self.assertEqual(app.ide_executable,'/custom/IDE')
                with mock.patch.object(gui.filedialog,'askopenfilename',return_value=''):
                    app.choose_ide()
                self.assertEqual(app.ide_executable,'/custom/IDE')
                self.assertEqual(self.snapshot(),before)
            finally:
                window.destroy()

    def test_save_then_launch_and_failed_save_never_launches(self):
        board='esp32s3_axs15231b_qspi'
        self.app.combo.current(self.app.boards.index(board))
        process=mock.Mock();process.poll.return_value=None
        def launch(project,executable):
            self.assertEqual((self.project/'board.txt').read_text().strip(),board)
            self.assertEqual(project,self.project)
            return process
        with mock.patch.object(gui,'launch_ide',side_effect=launch) as start:
            self.assertTrue(self.app.save_and_execute_ide());start.assert_called_once()
        self.app._check_ide_launch(process)
        self.assertIn('IDE launch requested',self.app.status.get())
        with mock.patch.object(self.app,'apply',return_value=False):
            with mock.patch.object(gui,'launch_ide') as start:
                self.assertFalse(self.app.save_and_execute_ide());start.assert_not_called()

    def test_ide_errors_keep_selection_saved(self):
        with mock.patch.object(gui,'launch_ide',side_effect=OSError('Cannot start executable')):
            with mock.patch.object(gui.messagebox,'showerror') as error:
                self.assertFalse(self.app.save_and_execute_ide());error.assert_called_once()
        self.assertEqual((self.project/'board.txt').read_text().strip(),self.app.selected)
        self.assertIn('Selection saved',self.app.status.get())
        process=mock.Mock();process.poll.return_value=1
        with mock.patch.object(gui.messagebox,'showerror') as error:
            self.app._check_ide_launch(process);error.assert_called_once()
        self.assertIn('exited with code 1',self.app.status.get())

    def test_missing_ide_offers_picker_or_allows_cancel(self):
        process=mock.Mock();process.poll.return_value=0
        with mock.patch.object(gui,'launch_ide',side_effect=[gui.IdeNotFoundError('missing'),process]) as start:
            with mock.patch.object(gui.filedialog,'askopenfilename',return_value='/custom/Arduino IDE'):
                self.assertTrue(self.app.save_and_execute_ide())
                self.assertEqual(start.call_args.args,(self.project,'/custom/Arduino IDE'))
        with mock.patch.object(gui,'launch_ide',side_effect=gui.IdeNotFoundError('missing')):
            with mock.patch.object(gui.filedialog,'askopenfilename',return_value=''):
                self.assertFalse(self.app.save_and_execute_ide())
        self.assertIn('Selection saved',self.app.status.get())
        self.assertIn('cancelled',self.app.status.get())


if __name__=='__main__':unittest.main()
