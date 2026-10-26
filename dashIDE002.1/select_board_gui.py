#!/usr/bin/env python3
"""Tkinter front end for the existing sketch-local board selector.

Previewing a profile, copying instructions or closing the window never writes
configuration. Apply and Save and Execute IDE call the same select_board.select().
This program neither detects USB devices nor compiles/uploads firmware or edits
installed Arduino libraries. Keep all deployment credentials out of this file.
"""
from pathlib import Path
import argparse
import sys

from board_profiles import PROFILES, ALIASES, canonical_name, ide_settings
import select_board
from ide_launcher import find_ide, launch_ide, IdeNotFoundError

# Keep helpers importable on minimal/headless systems. The launcher reports the
# missing optional desktop dependency instead of printing an import traceback.
try:
    import tkinter as tk
    from tkinter import ttk, messagebox, filedialog
except ImportError:
    tk = ttk = messagebox = filedialog = None

ROOT = Path(__file__).resolve().parent


def aliases_for(board):
    """List familiar names without creating a second hardware-profile table."""
    return [alias for alias,target in ALIASES.items() if target==board]


def profile_label(board):
    """Include technical and familiar names in the same read-only menu entry."""
    return '%s  (%s)' % (board, ', '.join(aliases_for(board)))


def read_saved_profile(project):
    """Read legacy aliases too; report invalid/missing data without rewriting it."""
    try:
        return canonical_name((project/'board.txt').read_text().strip())
    except (OSError, ValueError, argparse.ArgumentTypeError):
        return None


def settings_text(board):
    """Plain-text instructions suitable for a clipboard or test assertion."""
    return '\n'.join(['Profile: '+board, '']+
                     [key+': '+value for key,value in ide_settings(board)]+
                     ['', 'Set these options manually in Arduino IDE > Tools.',
                      'Reopen dashIDE002.1.ino after applying the selection.'])


class BoardSelectorApp:
    """Small synchronous view/controller; file generation stays in select_board.

    project is injectable for temporary-directory tests. The selector module's
    ROOT owns the actual write location; production uses the script directory
    for both. Tests patch that shared ROOT rather than reproduce selection logic.
    """
    def __init__(self, root, project=ROOT):
        self.root=root
        self.project=Path(project)
        self.boards=list(PROFILES)
        self.saved=read_saved_profile(self.project)
        self.applied_messages=[]
        # Discovery only reads the installation; it never starts the IDE.
        try:
            default_ide=find_ide()
        except IdeNotFoundError:
            default_ide=''
        self.ide_path=tk.StringVar(value=default_ide)
        root.title('select_board_gui - dashIDE002.1')
        root.minsize(820, 620)
        root.geometry('920x670')
        root.columnconfigure(0, weight=1)
        root.rowconfigure(0, weight=1)
        style=ttk.Style(root)
        style.configure('Title.TLabel', font=('TkDefaultFont',18,'bold'))
        style.configure('Heading.TLabel', font=('TkDefaultFont',10,'bold'))
        style.configure('Success.TLabel', foreground='#18733b')
        style.configure('Status.TLabel', foreground='#355570')
        style.configure('Error.TLabel', foreground='#b42318')

        body=ttk.Frame(root,padding=24)
        body.grid(row=0,column=0,sticky='nsew')
        body.columnconfigure(0,weight=1)
        ttk.Label(body,text='Choose your board',style='Title.TLabel').grid(row=0,column=0,sticky='w')
        ttk.Label(body,text='Prepare the sketch, then compile in Arduino IDE.').grid(row=1,column=0,sticky='w',pady=(5,17))
        self.current=tk.StringVar()
        ttk.Label(body,textvariable=self.current,style='Heading.TLabel').grid(row=2,column=0,sticky='w',pady=(0,15))
        self._show_current()

        ttk.Label(body,text='Hardware profile',style='Heading.TLabel').grid(row=3,column=0,sticky='w',pady=(0,5))
        self.combo=ttk.Combobox(body,state='readonly',values=[profile_label(b) for b in self.boards],width=76)
        self.combo.grid(row=4,column=0,sticky='ew')
        self.combo.current(self.boards.index(self.saved) if self.saved else 0)
        self.combo.bind('<<ComboboxSelected>>',self.preview)

        self.options=ttk.LabelFrame(body,text='Arduino IDE > Tools',padding=(15,10))
        self.options.grid(row=5,column=0,sticky='ew',pady=18)
        self.options.columnconfigure(1,weight=1)
        self.option_widgets=[]
        self.status=tk.StringVar(value='Choose a profile and click Apply selection to save it.')
        self.status_label=ttk.Label(body,textvariable=self.status,style='Status.TLabel',wraplength=750)
        self.status_label.grid(row=8,column=0,sticky='ew',pady=(0,15))
        ttk.Label(body,text='Set the Tools options above manually in the IDE.\nSave and close the sketch before applying; reopen it afterwards.',
                  wraplength=750).grid(row=7,column=0,sticky='w',pady=(0,15))
        ide_location=ttk.LabelFrame(body,text='Arduino IDE executable',padding=(10,8))
        ide_location.grid(row=6,column=0,sticky='ew',pady=(0,15))
        ide_location.columnconfigure(0,weight=1)
        self.ide_entry=ttk.Entry(ide_location,textvariable=self.ide_path)
        self.ide_entry.grid(row=0,column=0,sticky='ew')
        ttk.Button(ide_location,text='Browse...',command=self.choose_ide).grid(row=0,column=1,padx=(8,0))
        buttons=ttk.Frame(body)
        buttons.grid(row=9,column=0,sticky='ew')
        buttons.columnconfigure(2,weight=1)
        ttk.Button(buttons,text='Copy IDE settings',command=self.copy_settings).grid(row=0,column=0,sticky='w')
        self.apply_button=ttk.Button(buttons,text='Apply selection',command=self.apply)
        self.apply_button.grid(row=0,column=3,padx=(8,8))
        self.execute_button=ttk.Button(buttons,text="Save and Execute IDE",command=self.save_and_execute_ide)
        self.execute_button.grid(row=0,column=4,padx=(0,8))
        ttk.Button(buttons,text='Close',command=root.destroy).grid(row=0,column=5)
        root.bind('<Escape>',lambda _event:root.destroy())
        root.bind('<Control-Return>',lambda _event:self.apply())
        self.preview()
        if not self.saved:
            self.status.set('No valid saved profile found. Choose one and apply it.')
        self.combo.focus_set()

    @property
    def selected(self):
        """The menu is read-only, so its index maps directly to the registry."""
        index=self.combo.current()
        return self.boards[index] if 0<=index<len(self.boards) else None

    def _show_current(self):
        self.current.set('Saved profile: '+(self.saved or 'not configured'))

    def preview(self, _event=None):
        """Refresh the menu instructions only; do not apply a selection."""
        for widget in self.option_widgets:
            widget.destroy()
        self.option_widgets=[]
        if not self.selected:
            return
        for row,(key,value) in enumerate(ide_settings(self.selected)):
            title=ttk.Label(self.options,text=key+':')
            title.grid(row=row,column=0,sticky='nw',padx=(0,22),pady=4)
            content=ttk.Label(self.options,text=value,style='Heading.TLabel',wraplength=510)
            content.grid(row=row,column=1,sticky='w',pady=4)
            self.option_widgets.extend([title,content])

    def copy_settings(self):
        """Copy user-facing menu choices, with no write to the source tree."""
        if self.selected:
            self.root.clipboard_clear()
            self.root.clipboard_append(settings_text(self.selected))
            self.status_label.configure(style='Status.TLabel')
            self.status.set('IDE settings copied. The saved profile has not changed.')

    def apply(self):
        """Use the real selector and keep the window usable after file errors."""
        if not self.selected:
            return False
        messages=[]
        try:
            select_board.select(self.selected,emit=messages.append)
        except (OSError,ValueError) as exc:
            self.status_label.configure(style='Error.TLabel')
            self.status.set('Could not save the selection. Check the project files and permissions.')
            messagebox.showerror('Could not apply selection',str(exc),parent=self.root)
            return False
        self.applied_messages=messages
        self.saved=self.selected
        self._show_current()
        self.status_label.configure(style='Success.TLabel')
        self.status.set(messages[0]+'. Reopen the sketch in Arduino IDE.')
        return True


    @property
    def ide_executable(self):
        """Use the editable path for launch, including manually typed changes."""
        return self.ide_path.get().strip() or None

    @ide_executable.setter
    def ide_executable(self,value):
        self.ide_path.set(value or '')

    def choose_ide(self):
        """Choose a non-standard IDE installation for this window only."""
        options={'title':'Choose Arduino IDE executable','parent':self.root}
        # Open the picker beside the detected/edited executable when possible.
        if self.ide_executable:
            current=Path(self.ide_executable).expanduser()
            if current.parent.is_dir():
                options.update(initialdir=str(current.parent),initialfile=current.name)
        chosen=filedialog.askopenfilename(**options)
        if chosen:
            self.ide_executable=chosen
            self.status_label.configure(style='Status.TLabel')
            self.status.set('Arduino IDE executable: '+chosen)
        return chosen

    def save_and_execute_ide(self):
        """Save first; never start the IDE if writing the selection failed."""
        if not self.apply():return False
        try:
            try:
                process=launch_ide(self.project,self.ide_executable)
            except IdeNotFoundError:
                if not self.choose_ide():
                    self.status.set('Selection saved. Arduino IDE launch cancelled.')
                    return False
                process=launch_ide(self.project,self.ide_executable)
        except OSError as exc:
            self.status_label.configure(style='Error.TLabel')
            self.status.set('Selection saved, but Arduino IDE could not be started.')
            messagebox.showerror('Could not open Arduino IDE',str(exc),parent=self.root)
            return False
        self.status_label.configure(style='Success.TLabel')
        self.status.set('Selection saved. Starting Arduino IDE with this project...')
        # Normal exit can mean that an already-running IDE accepted the sketch.
        # Only an explicit nonzero exit is an immediate startup failure.
        self.root.after(1200,self._check_ide_launch,process)
        return True

    def _check_ide_launch(self,process):
        """Report immediate process failures without reverting a saved selection."""
        code=process.poll()
        if code is not None and code!=0:
            self.status_label.configure(style='Error.TLabel')
            self.status.set('Selection saved. Arduino IDE exited with code '+str(code)+'.')
            messagebox.showerror('Arduino IDE startup failed',
                'Check the IDE installation or choose another IDE executable. The selection remains saved.',parent=self.root)
        else:
            self.status.set('Selection saved. IDE launch requested; check its Tools settings.')


def main():
    """Start a native desktop window, with actionable dependency/display errors."""
    if tk is None:
        print('Tkinter is not installed. On Debian/Ubuntu: sudo apt install python3-tk\n'
              'On Windows, install Python 3 with Tcl/Tk support.\n'
              'The command-line select_board script remains available.',file=sys.stderr)
        return 1
    try:
        root=tk.Tk()
    except tk.TclError as exc:
        print('Cannot open a desktop window: '+str(exc)+'\n'
              'Run this tool from a graphical desktop, or use select_board.py.',file=sys.stderr)
        return 1
    BoardSelectorApp(root)
    root.mainloop()
    return 0


if __name__=='__main__':
    raise SystemExit(main())
