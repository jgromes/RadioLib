#!/usr/bin/python3
# -*- encoding: utf-8 -*-

import argparse
import numpy as np
from PIL import Image
from argparse import RawTextHelpFormatter

def main():
  parser = argparse.ArgumentParser(formatter_class=RawTextHelpFormatter, description='''
        RadioLib image to array conversion tool.

        Input is a PNG image to be transmitted via RadioLib SSTV.
        The image must have correct size for the chose SSTV mode!

        Output is a file (by default named "img.h") which can be included and transmitted.
        The resulting array will be very large (typically 320 kB),
        make sure your platform has sufficient Flash/RAM space.
    ''')
  parser.add_argument('input',
      type=str,
      help='Input PNG file')
  parser.add_argument('output',
      type=str,
      nargs='?',
      default='img',
      help='Output header file')
  args = parser.parse_args()
  outfile = f'{args.output}.h'
  print(f'Converting "{args.input}" to "{outfile}"')

  # open the image as numpy array
  img = Image.open(args.input)
  arr = np.array(img)

  # open the output file
  with open(outfile, 'w') as f:
    print(f'const uint32_t img[{arr.shape[0]}][{arr.shape[1]}] = {{', file=f)
    for row in arr:
      print('  { ', end='', file=f)
      for pix in row:
        rgb = pix[0] << 16 | pix[1] << 8 | pix[2]
        print(hex(rgb), end=', ', file=f)
      print(' },', file=f)
    print('};', file=f)
  
  print('Done!')

if __name__ == "__main__":
    main()
