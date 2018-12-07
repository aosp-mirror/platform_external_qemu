import os
from Queue import Queue
import subprocess
from threading import Thread

from absl import logging


def _reader(pipe, queue):
    try:
        with pipe:
            for line in iter(pipe.readline, b''):
                queue.put((pipe, line[:-1]))
    finally:
        queue.put(None)


def log_std_out(proc):
    """Logs the output of the given process."""
    q = Queue()
    Thread(target=_reader, args=[proc.stdout, q]).start()
    Thread(target=_reader, args=[proc.stderr, q]).start()
    for _ in range(2):
        for _, line in iter(q.get, None):
            logging.info(line)


def run(cmd, cwd=None):
    logging.info('Running: %s in %s', ' '.join(cmd), cwd)
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=os.path.abspath(cwd))

    log_std_out(proc)
    proc.wait()
    if proc.returncode != 0:
        raise Exception('Failed to run %s - %s' % (' '.join(cmd), proc.returncode))
