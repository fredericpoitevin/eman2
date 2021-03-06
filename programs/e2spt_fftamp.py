#!/usr/bin/env python
# Author: Jesus Galaz  19/1/2012; last modified, dec/2017
# Copyright (c) 2011- Baylor College of Medicine
#
# This software is issued under a joint BSD/GNU license. You may use the
# source code in this file under either license. However, note that the
# complete EMAN2 and SPARX software packages have some GPL dependencies,
# so you are responsible for compliance with the licenses of these packages
# if you opt to use BSD licensing. The warranty disclaimer below holds
# in either instance.
#
# This complete copyright notice must be included in any revised version of the
# source code. Additional authorship citations may be added, but existing
# author citations must be preserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston MA 02111-1307 USA
from __future__ import print_function
from __future__ import division
from builtins import range
from sys import argv
import os
from EMAN2 import *
from EMAN2_utils import *

def main():
	progname = os.path.basename(sys.argv[0])
	usage = """prog <Volume file> [options]
	This program writes out the 3D image of the amplitudes of the Fourier transform of a subvolume or a stack of subvolumes."""

	parser = EMArgumentParser(usage=usage,version=EMANVERSION)
	
	parser.add_argument("--input", type=str, default=None, help="default=None. Filename of 3-D image or stack of 3-D images whose FFT amplitudes you want to see.")
	
	parser.add_argument("--ppid", type=int, help="""Default=-1. Set the PID of the parent process, used for cross platform PPID""",default=-1)

	parser.add_argument("--verbose", "-v", dest="verbose", action="store", metavar="n", type=int, default=0, help="verbose level [0-9], higner number means higher level of verboseness")				
	
	(options, args) = parser.parse_args()	

	logger = E2init(sys.argv, options.ppid)
	
	data = argv[1]
	
	if options.input:
		data = options.input
	
	n = EMUtil.get_image_count(data)
	for i in range(n):
		a = EMData(data,i)
		nx=a['nx']
		ny=a['ny']
		nz=a['nz']
		a.process_inplace('math.fft.resample',{'n':0.5})
		b = a.do_fft()
		b.ri2ap()
		c = b.amplitude()
		c.process_inplace('xform.phaseorigin.tocenter')
		#d = c.process('xform.mirror',{'axis':'x'})
		#c.rotate(0,-90,0)
		c.process_inplace('math.fft.resample',{'n':2.0})
		d = c.copy()
		d.rotate(0,0,180)
		
		e = c + d
		e.write_image(data.replace('.','_fftamp.'),i)
	
	E2end(logger)
	
	return
	
	
if __name__ == "__main__":
    main()

