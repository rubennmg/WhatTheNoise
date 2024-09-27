from flask import Flask
from app.main.routes import main

def create_app():
    app = Flask(__name__)
    app.config['SECRET_KEY'] = 'this_is_a_secret_key'
    app.register_blueprint(main)
    return app