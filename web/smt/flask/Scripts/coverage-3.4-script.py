#!d:\microblog\flask\scripts\python.exe
# EASY-INSTALL-ENTRY-SCRIPT: 'coverage==4.4.1','console_scripts','coverage-3.4'
__requires__ = 'coverage==4.4.1'
import sys
from pkg_resources import load_entry_point

if __name__ == '__main__':
    sys.exit(
        load_entry_point('coverage==4.4.1', 'console_scripts', 'coverage-3.4')()
    )
