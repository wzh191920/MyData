#!d:\microblog\flask\scripts\python.exe
# EASY-INSTALL-ENTRY-SCRIPT: 'sqlalchemy-migrate==0.11.0','console_scripts','migrate-repository'
__requires__ = 'sqlalchemy-migrate==0.11.0'
import sys
from pkg_resources import load_entry_point

if __name__ == '__main__':
    sys.exit(
        load_entry_point('sqlalchemy-migrate==0.11.0', 'console_scripts', 'migrate-repository')()
    )
