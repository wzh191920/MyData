from flask import Flask
from flask_bootstrap import Bootstrap
from config import Config
import logging




def create_app():
    app = Flask(__name__, static_url_path='')
    app.config.from_object(Config)

    handler = logging.FileHandler('MydataWeb.log', encoding='UTF-8')
    handler.setLevel(logging.DEBUG)
    logging_format = logging.Formatter('%(asctime)s - %(levelname)s - %(filename)s - %(funcName)s - %(lineno)s - %(message)s')
    handler.setFormatter(logging_format)
    app.logger.addHandler(handler)
    import Mydata
    Mydata.MydataConnect(app.logger)
    
    bootstrap = Bootstrap(app)
    from .main import main as main_blueprint
    app.register_blueprint(main_blueprint)
    from .api_1 import api as api_blueprint
    app.register_blueprint(api_blueprint)
    return app
    