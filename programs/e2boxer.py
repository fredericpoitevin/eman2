#!/bin/env python
# e2boxer.py  07/27/2004  Steven Ludtke
# This program is used to box out particles from micrographs/CCD frames

from EMAN2 import *
from optparse import OptionParser
from math import *
import os
import sys

def main():
	progname = os.path.basename(sys.argv[0])
	usage = """Usage: %prog [options] <image>
	
Automatic and manual particle selection. This version is specifically aimed at square boxes
for single particle analysis."""

	parser = OptionParser(usage=usage,version=EMANVERSION)

	parser.add_option("--gui",action="store_true",help="Start the GUI for interactive boxing",default=False)
	parser.add_option("--box","-B",type="int",help="Box size in pixels",default=-1)
	parser.add_option("--ptclsize","-P",type="int",help="Approximate size (diameter) of the particle in pixels. Not required if reference particles are provided.",default=-1)
	parser.add_option("--refptcl","-R",type="string",help="A stack of reference images. Must have the same scale as the image being boxed.",default=None)
	parser.add_option("--auto","-A",type="string",action="append",help="Autobox using specified method: circle, ref",default=[])
			
	(options, args) = parser.parse_args()
	if len(args)<1 : parser.error("Input image required")

	image=EMData()
	image.read_image(args[0])
	
	refptcl=None
	if options.refptcl :
		refptcl=EMData.read_images(options.refptcl)
		print "%d reference particles read"%len(refptcl)
	
	if options.box<5 :
		if options.refptcl : options.box=refptcl[0].get_xsize()
		elif options.ptclsize : 
			options.box=good_boxsize(options.ptclsize*1.2)
		else : parser.error("Please specify a box size")
	else:
		if not options.box in good_box_sizes:
			print "Note: EMAN2 processing would be more efficient with a boxsize of %d"%good_boxsize(options.box)
	
	shrinkfactor=int(ceil(options.box/16))
	print "Shrink factor = ",shrinkfactor
	#shrinkfactor=int(ceil(image.get_ysize()/1024.0))
	#if options.box/shrinkfactor<12 : shrinkfactor/=2
	
	shrink=image.copy(0)
	shrink.mean_shrink(shrinkfactor)		# shrunken original image
	
	print "Autobox mode ",options.auto
	
	if "ref" in options.auto:
		if not refptcl: error_exit("Reference particles required")
		
		# refptcls will contain shrunken normalized reference particles
		refptcls=[]
		for n,i in enumerate(refptcl):
			ic=i.copy()
			refptcls.append(ic)
			ic.mean_shrink(shrinkfactor)
			# first a circular mask
			ic.filter("mask.sharp",{"outer_radius":ic.get_xsize()/2-1})
			
			# make the unmasked portion mean -> 0
			ic.add(ic.get_attr("mean_nonzero"),keepzero=1)
			
			ic.write_image("scaled_refs.hdf",-1)
			
	
	if "circle" in options.auto:
		shrinksq=shrink.copy(0)
		shrinksq*=shrinksq			# shrunken original image squared
		
		# outer and inner ring mask
		outer=EMData()
		sbox=int(options.box/shrinkfactor)
		outer.set_size(shrink.get_xsize(),shrink.get_ysize(),1)
		outer.to_one()
		inner=outer.copy(0)
		
		outer.filter("mask.sharp",{"inner_radius":sbox*2/5,"outer_radius":sbox/2})
		inner.filter("mask.sharp",{"outer_radius":sbox*2/5})
		
		outer.write_image("b_outer.mrc")
		inner.write_image("b_inner.mrc")

		ccf1=shrinksq.calc_ccf(inner,True,None)
		ccf2=shrinksq.calc_ccf(outer,True,None)
		
		ccf1.write_image("b_ccf1.mrc")
		ccf2.write_image("b_ccf2.mrc")
		
		ccf1/=ccf2
		
		ccf1.write_image("b_result.mrc")
	
if __name__ == "__main__":
	main()
