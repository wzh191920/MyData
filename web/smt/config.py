import os
basedir = os.path.abspath(os.path.dirname(__file__))

class Config:
    DEBUG = True
    SECRET_KEY = 'hard to guess string'
    BOOTSTRAP_SERVE_LOCAL = True

