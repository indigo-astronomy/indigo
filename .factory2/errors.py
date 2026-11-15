"""Public application errors shared by the core and its interfaces."""


class FactoryError(Exception):
    def __init__(self, code, message):
        super().__init__(message)
        self.code = code
        self.message = message


def require(condition, code, message):
    if not condition:
        raise FactoryError(code, message)
