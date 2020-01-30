# Lint as: python3
"""Script to figure out which set of iwyu fixes can be applied.


"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from absl import app
from absl import flags
from absl import logging
import subprocess
import os

FLAGS = flags.FLAGS
flags.DEFINE_list('builds', ['objs', 'build-msvc'],
                  'Directories to build for validation')
flags.DEFINE_string('rev', None, 'From revision')


def run_silent(cmd):
  with open(os.devnull, 'w') as fp:
    try:
      logging.debug('Running %s', ' '.join(cmd))
      subprocess.check_call(cmd, stdout=fp, stderr=fp)
      logging.debug('Success!')
      return True
    except subprocess.CalledProcessError as e:
      logging.debug('Failed :-(')
      return False


def get_real_files(rev):
  cmd = 'git diff-tree --no-commit-id --name-only -r'.split(' ')
  cmd.append(rev)
  return subprocess.check_output(cmd).splitlines()


def reset_files(fnames):
  run_silent('git reset HEAD'.split(' ') + fnames)
  run_silent('git checkout'.split(' ') + fnames)


def checkout_from_rev(rev, fnames):
  run_silent('git checkout'.split(' ') + [rev] + fnames)


def compile_dir(bld_dir):
  return run_silent(['ninja', '-C', bld_dir])


def compile_test():
  for bld in FLAGS.builds:
    if not compile_dir(bld):
      return False
  return True

class TmpCheckoutFromGit(object):

  def __init__(self, rev, files):
    self.rev = rev
    self.files = files

  def __enter__(self):
    checkout_from_rev(self.rev, self.files)

  def __exit__(self, exc_type, exc_val, exc_tb):
    reset_files(self.files)


def find_failures(rev, files, good):
  """Recursively find all failures.

  Returns the sets: success, failed
  """
  if len(files) == 0:
    return [], []
  success = []
  failed = []
  logging.debug('Checking: %s', files)
  with TmpCheckoutFromGit(rev, files) as tg:
    tst = compile_test()

  logging.info('-- Checked %s, %s', files, tst)
  if tst:
    success = files
  else:
    # 2 cases. We have 1 file (bingo!)
    if len(files) == 1:
      failed = files
    else:
      mid = len(files) // 2
      success, failed = find_failures(rev, files[:mid], good)
      s, f = find_failures(rev, files[mid:], good)
      success += s
      failed += f

  good.update(success)
  logging.info("Know good: %s", good)
  return success, failed


def main(argv):
  if len(argv) > 1:
    raise app.UsageError('Too many command-line arguments.')

  files = get_real_files(FLAGS.rev)
  success, failed = find_failures(FLAGS.rev, files, set())
  print('Success: ', success)
  print('Failed: ', failed)
  checkout_from_rev(FLAGS.rev, success)


if __name__ == '__main__':
  app.run(main)
