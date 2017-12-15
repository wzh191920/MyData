from manage import app
def log_error(error_string):
    def decorator(f):
        def decorated_function(*args, **kwargs):
            err = f(*args, **kwargs)
            if err:
                app.logger("%s, %d", error_string, err)
            return f(*args, **kwargs)
        return decorated_function
    return decorator


