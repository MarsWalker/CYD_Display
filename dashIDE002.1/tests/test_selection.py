"""Test selection as a local file-generation operation, without an Arduino core."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import os
import json
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SelectionTests(unittest.TestCase):
    def test_all_profiles_and_portable_paths(self):
        with tempfile.TemporaryDirectory(prefix='IDE selection with spaces ') as directory:
            project = Path(directory)/'dashIDE002.1'
            shutil.copytree(ROOT,project,ignore=shutil.ignore_patterns('build','build_out','__pycache__','secrets.h','secrets.json','.git'))
            original={p.relative_to(project):p.read_bytes() for p in project.rglob('*') if p.is_file()}
            generated={'selected_board.h','build_opt.h','board.txt','build_nr.txt','build.h'}
            number=int((project/'build_nr.txt').read_text())
            cases = [
                ('esp32_ili9341_xpt2046',1,'esp32_ili9341_xpt2046'),
                ('esp32_ili9341_cap_touch',2,'esp32_ili9341_cap_touch'),
                ('esp32s3_axs15231b_qspi',3,'esp32s3_axs15231b_qspi'),
                ('esp32_st7789_parallel_cst820',4,'esp32_st7789_parallel_cst820'),
                ('amarela2.8',1,'esp32_ili9341_xpt2046'),
                ('amarela2.2',4,'esp32_st7789_parallel_cst820'),
                ('pink',2,'esp32_ili9341_cap_touch'),
                ('black',3,'esp32s3_axs15231b_qspi'),
                ('preta',3,'esp32s3_axs15231b_qspi'),
                (' PRETA ',3,'esp32s3_axs15231b_qspi'),
            ]
            for board,expected,canonical in cases:
                result=subprocess.run([sys.executable,str(project/'select_board.py'),board],
                                      cwd=directory,capture_output=True,text=True)
                self.assertEqual(result.returncode,0,result.stderr)
                self.assertIn(f'#define DASH_SELECTED_BOARD {expected}',(project/'selected_board.h').read_text())
                self.assertEqual((project/'build_opt.h').read_text(),f'-DDASH_IDE_PROFILE={expected}\n-include ide_build.h\n')
                self.assertEqual((project/'board.txt').read_text(),canonical+'\n')
                number+=1
                self.assertEqual(int((project/'build_nr.txt').read_text()),number)
                self.assertNotIn(directory,(project/'build_opt.h').read_text())
            for relative,contents in original.items():
                if str(relative) not in generated:self.assertEqual((project/relative).read_bytes(),contents,str(relative))
            snapshot={p.relative_to(project):p.read_bytes() for p in project.rglob('*') if p.is_file()}
            result=subprocess.run([sys.executable,str(project/'select_board.py'),'amarela2.4'],capture_output=True)
            self.assertNotEqual(result.returncode,0)
            self.assertEqual(snapshot,{p.relative_to(project):p.read_bytes() for p in project.rglob('*') if p.is_file()})
            self.assertFalse((project/'secrets.h').exists())


    def test_mcu_and_psram_guards(self):
        with tempfile.TemporaryDirectory(prefix='IDE guards ') as directory:
            project=Path(directory)/'dashIDE002.1'
            shutil.copytree(ROOT,project,ignore=shutil.ignore_patterns('build','build_out','__pycache__','secrets.h','secrets.json','.git'))
            sdk=Path(directory)/'sdk';sdk.mkdir();(sdk/'sdkconfig.h').write_text('/* simulated platform defines supplied by test */\n')
            def preprocess(defines):
                return subprocess.run(['gcc','-E','-x','c','-I',str(project),'-I',str(sdk),
                    '-include','ide_build.h',*defines,'-'],input='',capture_output=True,text=True)
            subprocess.run([sys.executable,str(project/'select_board.py'),'black'],check=True,stdout=subprocess.DEVNULL)
            wrong=preprocess(['-DCONFIG_IDF_TARGET_ESP32=1','-DBOARD_HAS_PSRAM'])
            self.assertNotEqual(wrong.returncode,0);self.assertIn('ESP32S3 Dev Module',wrong.stderr)
            missing=preprocess(['-DCONFIG_IDF_TARGET_ESP32S3=1'])
            self.assertNotEqual(missing.returncode,0);self.assertIn('OPI PSRAM',missing.stderr)
            self.assertEqual(preprocess(['-DCONFIG_IDF_TARGET_ESP32S3=1','-DBOARD_HAS_PSRAM']).returncode,0)
            subprocess.run([sys.executable,str(project/'select_board.py'),'amarela2.2'],check=True,stdout=subprocess.DEVNULL)
            self.assertEqual(preprocess(['-DCONFIG_IDF_TARGET_ESP32=1']).returncode,0)
            self.assertNotEqual(preprocess(['-DCONFIG_IDF_TARGET_ESP32S3=1']).returncode,0)
            # CLI's explicit profile overrides the persisted IDE header/marker.
            self.assertEqual(preprocess(['-DDASH_BOARD=3','-DCONFIG_IDF_TARGET_ESP32S3=1','-DBOARD_HAS_PSRAM']).returncode,0)


    @unittest.skipUnless(os.name=='posix', 'Bash builder smoke test')
    def test_build_aliases_with_fake_compiler(self):
        # Exercise the real Bash entry point, artifact paths and metadata without
        # accessing hardware, installing packages or requiring private config.
        import sys
        sys.path.insert(0,str(ROOT))
        from board_profiles import PROFILES, ALIASES
        with tempfile.TemporaryDirectory(prefix='profile build ') as directory:
            base=Path(directory);project=base/'dashIDE002.1'
            shutil.copytree(ROOT,project,ignore=shutil.ignore_patterns('build','build_out','__pycache__','secrets.h','secrets.json','.git'))
            (base/'libraries').mkdir()
            shutil.copyfile(project/'secrets_template.h',project/'secrets.h')
            fake=base/'fake-cli';record=base/'calls.jsonl'
            fake.write_text('#!/usr/bin/env python3\nimport sys,os,json\nfrom pathlib import Path\na=sys.argv[1:]\nwith open(os.environ["PROFILE_TEST_LOG"],"a") as f:f.write(json.dumps(a)+"\\n")\nif a[0]=="compile":Path(a[a.index("--output-dir")+1]).mkdir(parents=True,exist_ok=True)\n')
            fake.chmod(0o755)
            env=dict(os.environ,ARD_CLI=str(fake),PROFILE_TEST_LOG=str(record))
            for requested in [*PROFILES,*ALIASES]:
                canonical=ALIASES.get(requested,requested);board_id,_,fqbn=PROFILES[canonical]
                result=subprocess.run(['bash',str(project/'build.sh'),requested],env=env,capture_output=True,text=True)
                self.assertEqual(result.returncode,0,result.stderr)
                self.assertEqual((project/'board.txt').read_text().strip(),canonical)
                metadata=json.loads((project/'build_out'/canonical/'board-build.json').read_text())
                self.assertEqual((metadata['Board'],metadata['Fqbn']),(canonical,fqbn))
                args=json.loads(record.read_text().splitlines()[-1])
                self.assertEqual(args[args.index('--fqbn')+1],fqbn)
                self.assertIn(f'compiler.c.extra_flags=-DDASH_BOARD={board_id}',args)
            # Saved names are accepted without an explicit board argument too.
            (project/'board.txt').write_text('preta\n')
            result=subprocess.run(['bash',str(project/'build.sh')],env=env,capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual((project/'board.txt').read_text().strip(),ALIASES['preta'])

    def test_powershell_profile_tables_match_registry(self):
        # This checks data consistency only; it is not a PowerShell runtime test.
        import sys
        sys.path.insert(0,str(ROOT))
        from board_profiles import PROFILES, ALIASES
        text=(ROOT/'build.ps1').read_text()
        profile_text=text.split('$profiles = @{',1)[1].split('\n}',1)[0]
        profiles={name:(int(board_id),fqbn) for name,board_id,fqbn in
                  re.findall(r"'([^']+)'\s*=\s*@\{ Id = (\d+); Fqbn = '([^']+)'",profile_text)}
        self.assertEqual(profiles,{name:(data[0],data[2]) for name,data in PROFILES.items()})
        alias_text=text.split('$aliases = @{',1)[1].split('\n}',1)[0]
        self.assertEqual(dict(re.findall(r"'([^']+)'\s*=\s*'([^']+)'",alias_text)),ALIASES)


if __name__=='__main__':unittest.main()
