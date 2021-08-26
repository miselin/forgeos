#!/usr/bin/env python3

import sys


def def_to_define(d):
  if '=' in d:
    l, r = d.split('=', 1)
  else:
    l, r = d, None

  if not l.startswith('-D'):
    print('Invalid define %s' % (d,), file=sys.stderr)
    return

  l = l[2:]

  if r is None:
    r = ''
  else:
    r = ' ' + r

  return '// %s\n#define %s%s\n' % (d, l, r)


def main():
  if len(sys.argv) < 2:
    print(
        'Usage: genconfig.py [output] [-DDEFINITION=val...]', file=sys.stderr)
    exit(1)

  outfile = sys.argv[1]
  defs = sys.argv[2:]

  output = '#ifndef _KCONFIG_H\n#define _KCONFIG_H\n'
  for d in defs:
    v = def_to_define(d)
    if v is None:
      exit(1)

    output += v

  output += '#endif // _KCONFIG_H\n'

  with open(outfile, 'w') as f:
    f.write(output)


if __name__ == '__main__':
  main()
