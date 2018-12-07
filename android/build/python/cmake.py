from absl import app

from aemu import cmake

def launch():
    app.run(cmake.main)

if __name__ == '__main__':
    launch()
