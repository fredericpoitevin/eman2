#!/usr/bin/env python

#
# Author: Steven Ludtke, 04/10/2003 (sludtke@bcm.edu)
# Copyright (c) 2000-2006 Baylor College of Medicine
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  2111-1307 USA
#
#

# $Id$

# todo: verify the processors who have the same names in proc3d
#	   and proc2d have the same implementation
#
# todo: lp, hp, tlp vs apix

from EMAN2 import *
from optparse import OptionParser
import sys
from math import *
import os.path
import pyemtbx.options
from pyemtbx.options import intvararg_callback
from pyemtbx.options import floatvararg_callback
from time import time

def print_iminfo(data, label):
	print "%s image : %dx%dx%d Mean=%1.3g Sigma=%1.3g Min=%1.3g Max=%1.3g" % \
	(label, data.get_xsize(), data.get_ysize(), data.get_zsize(),
	data.get_attr("mean"), data.get_attr("sigma"),
	data.get_attr("minimum"), data.get_attr("maximum"))

def calcsf(data):
	dataf = data.do_fft()
	curve = dataf.calc_radial_dist(ny/2, 0, 1.0,True)
	curve=[i/(dataf["nx"]*dataf["ny"]*dataf["nz"]) for i in curve]
	Util.save_data(0, 1.0/(apix*ny), curve, options.calcsf);
	return()

def main():
	progname = os.path.basename(sys.argv[0])
	usage = progname + """ [options] <inputfile> <outputfile>
	Generic 3-D image processing and file format conversion program.
        All EMAN2 recognized file formats accepted (see Wiki for list).

        Examples:

        convert MRC format to HDF format:
        e2proc3d.py test.mrc test.hdf

        apply a 10 A low-pass filter to a volume and write output to a new file.
        e2proc3d.py threed.hdf threed.filt.hdf --process=filter.lowpass.gauss:cutoff_freq=0.1

	extract a reconstruction from a refinement directory as an HDF file usable with Chimera
	e2proc3d.py bdb:refine_02#threed_filt_04 map_02_04.hdf

        'e2help.py processors -v 2' for a detailed list of available procesors

"""
	parser = OptionParser(usage)

	parser.add_option("--medianshrink", metavar="n", type="int", action="append",
								help="Downsamples the volume by a factor of n by computing the local median")
	parser.add_option("--meanshrink", metavar="n", type="int", action="append",
								help="Downsamples the volume by a factor of n by computing the local average")
	parser.add_option("--meanshrinkbig", metavar="n", type="int", default=0,
								help="Downsamples the volume by a factor of n without reading the entire volume into RAM. The output file (after shrinking) must fit into RAM. If specified, this must be the ONLY option on the command line. Any other options will be ignored. Output data type will match input data type. Works only on single image files, not stack files.")

#    parser.add_option("--tomoshrink", metavar="n", type="int", action="append",
#                                help="Mean shrinks the image but is careful of memory - reads small pixel blocks from disk and slowly builds up the result")
	parser.add_option("--scale", metavar="n", type="float", action="append",
								help="Rescales the image by 'n', generally used with clip option.")
	parser.add_option("--sym", dest = "sym", action="append",
								help = "Symmetry to impose - choices are: c<n>, d<n>, h<n>, tet, oct, icos")
	parser.add_option("--clip", metavar="x[,y,z[,xc,yc,zc]]", type='string', action="callback", callback=intvararg_callback,
								help="Make the output have this size by padding/clipping. 1, 3 or 6 arguments. ")
	parser.add_option("--fftclip", metavar="x, y, z", type="string", action="callback", callback=floatvararg_callback,
								help="Make the output have this size, rescaling by padding FFT.")
	parser.add_option("--process", metavar="processor_name:param1=value1:param2=value2", type="string",
								action="append", help="apply a processor named 'processorname' with all its parameters/values.")
	parser.add_option("--apix", type="float", help="A/pixel for S scaling")
	parser.add_option("--origin", metavar="x, y, z", type="string", action="callback", callback=floatvararg_callback,
								help="Set the coordinates for the pixel (0,0,0) for Chimera. THIS HAS NO IMPACT ON IMAGE PROCESSING !")
	parser.add_option("--mult", metavar="f", type="float",
								help="Scales the densities by a fixed number in the output")
	parser.add_option("--multfile", type="string", action="append",
								help="Multiplies the volume by another volume of identical size. This can be used to apply masks, etc.")

	parser.add_option("--outmode",type="string", default="float", help="All EMAN2 programs write images with 4-byte floating point values when possible by default. This allows specifying an alternate format when supported (int8, int16, int32, uint8, uint16, uint32). Values are rescaled to fill MIN-MAX range.")
	parser.add_option("--outnorescale",action="store_true",default=False,help="If specified, floating point values will not be rescaled when writing data as integers. Values outside of range are truncated.")
	parser.add_option("--mrc16bit",  action="store_true", default=False, help="(deprecated, use --outmode instead) output as 16 bit MRC file")
	parser.add_option("--mrc8bit",  action="store_true", default=False, help="(deprecated, use --outmode instead) output as 8 bit MRC file")

	parser.add_option("--add", metavar="f", type="float",
								help="Adds a constant 'f' to the densities")
	parser.add_option("--calcfsc", type="string", metavar="with input",
								help="Calculate a FSC curve between two models. Output is a txt file. This option is the name of the second volume.")
	parser.add_option("--calcsf", type="string", metavar="outputfile",
								help="Calculate a radial structure factor. Must specify apix.")
	parser.add_option("--setsf", type="string", metavar="inputfile",
								help="Set the radial structure factor. Must specify apix.")
	parser.add_option("--tophalf", action="store_true",
								help="The output only keeps the top half map")
	parser.add_option("--inputto1", action="store_true",help="All voxels in the input file are set to 1 after reading. This can be used with mask.* processors to produce a mask file of the correct size.")
	parser.add_option("--icos5fhalfmap", action="store_true",
								help="The input is the icos 5f top half map generated by the 'tophalf' option")
	parser.add_option("--outtype", metavar="image-type", type="string",
								help="Set output image format, mrc, imagic, hdf, etc")
	parser.add_option("--first", metavar="n", type="int", default=0,
								help="the first image in the input to process [0 - n-1])")
	parser.add_option("--trans", metavar="dx,dy,dz", type="string", action="append",
								help="Translate map by dx,dy,dz ")
	parser.add_option("--resetxf",action="store_true",help="Reset an existing transform matrix to the identity matrix")
	parser.add_option("--align", metavar="aligner_name:param1=value1:param2=value2", type="string", action="append",
								help="Align input map to reference specified with --alignref. As with processors, a sequence of aligners is permitted")
	parser.add_option("--ralignz", type=str ,action="append",
								help="Refine Z alignment within +-10 pixels (for C symmetries), specify name of alignment reference here not with --alignref")
	parser.add_option("--alignref", metavar="filename", type="string", default=None, help="Alignment reference volume. May only be specified once.")

	parser.add_option("--rot",type=str,metavar="az,alt,phi or convention:par=val:...",help="Rotate map. Specify az,alt,phi or convention:par=val:par=val:...  eg - mrc:psi=22:theta=15:omega=7", action="append",default=None)

	parser.add_option("--icos5to2",action="store_true",help="Rotate an icosahedral map from 5-fold on Z (EMAN standard) to 2-fold on Z (MRC standard) orientation")
	parser.add_option("--icos2to5",action="store_true",help="Rotate an icosahedral map from 2-fold on Z (MRC standard) to 5-fold on Z (EMAN standard)  orientation")
	parser.add_option("--last", metavar="n", type="int", default=-1,
								help="the last image in the input to process")
	parser.add_option("--swap", action="store_true", help="Swap the byte order", default=False)
	parser.add_option("--average", action="store_true", help="Computes the average of a stack of 3D volumes", default=False)
	parser.add_option("--append", action="store_true", help="Append output image, i.e., do not write inplace.")
	parser.add_option("--ppid", type=int, help="Set the PID of the parent process, used for cross platform PPID",default=-2)
	parser.add_option("--unstacking", action="store_true", help="Process a stack of 3D images, then output a a series of numbered single image files", default=False)
	parser.add_option("--verbose", "-v", dest="verbose", action="store", metavar="n", type="int", default=0, help="verbose level [0-9], higner number means higher level of verboseness")
	parser.add_option("--step",type=str,default=None,help="Specify <init>,<step>. Processes only a subset of the input data. For example, 0,2 would process only the even numbered particles")

	append_options = ["clip", "fftclip", "process", "filter", "meanshrink", "medianshrink", "scale", "sym", "multfile", "trans", "rot", "align","ralignz"]

	optionlist = pyemtbx.options.get_optionlist(sys.argv[1:])

	(options, args) = parser.parse_args()


	if len(args) != 2:
		print "usage: " + usage
		print "Please run '" + progname + " -h' for detailed options"
		sys.exit(1)

	if options.step and options.first:
		print 'Invalid options. You used --first and --step. The --step option contains both a step size and the first image to step from. Please use only the --step option rather than --step and --first'
		sys.exit(1)

	if options.mrc16bit:
		print "Deprecated option mrc16bit, please use outmode=int16"
		options.outmode="int16"
	if options.mrc8bit:
		print "Deprecated option mrc8bit, please use outmode=int8"
		options.outmode="int8"

	if not file_mode_map.has_key(options.outmode) :
		print "Invalid output mode, please specify one of :\n",str(file_mode_map.keys()).translate(None,'"[]')
		sys.exit(1)

	infile = args[0]
	outfile = args[1]
	is_new_file = not os.path.isfile(outfile)

	# This is a specilalized option which doesn't play nice with ANY other options in the command
	# it will do piecewise shrinking of a map which is too large for RAM
	if options.meanshrinkbig>0 :
		print "Dedicated large-map shrinking mode. No other operations will be performed."
		hdr=EMData(infile,0,True)
		shrink=options.meanshrinkbig
		if shrink>10 : print "Shrinking by >10x is not recommended"

		nx,ny,nz=hdr["nx"],hdr["ny"],hdr["nz"]
		nnx=nx/shrink
		nny=ny/shrink
		nnz=nz/shrink
		print "%d x %d x %d --> %d x %d x %d    %1.1f GB of RAM required"%(nx,ny,nz,nnx,nny,nnz,(nnx*nny*nnz*4+shrink*4*ny*shrink*4)/1.0e9)

		out=EMData(nnx,nny,nnz)
		out.to_zero()
		ltime=0
		for z in xrange(0,nz,shrink):
			if time()-ltime>0.5 :
				print "  %d/%d\r"%(z,nz),
				sys.stdout.flush()
				ltime=time()
			for y in xrange(0,ny,4*shrink):
				tmp=EMData(infile,0,False,Region(0,y,z,nx,4*shrink,shrink))
				tmp.process_inplace("math.meanshrink",{"n":shrink})
				out.insert_clip(tmp,(0,y/shrink,z/shrink))

		try: stype=tmp["datatype"]
		except: stype=EM_FLOAT
		print "  %d/%d"%(nz,nz),
		print "\nWriting in data mode ",file_mode_imap[int(stype)]

		if stype!=EM_FLOAT:
			out["render_min"]=file_mode_range[stype][0]
			out["render_max"]=file_mode_range[stype][1]

		try: out.write_image(outfile,0,IMAGE_UNKNOWN,0,None,EMUtil.EMDataType(stype))
		except:
			print "Failed to write in file mode matching input, reverting to floating point output"
			out.write_image(outfile,0)

		print "Complete !"
		sys.exit(0)

	n0 = options.first
	n1 = options.last
	nimg = EMUtil.get_image_count(infile)
	if n1 > nimg or n1<0: n1=nimg-1

	if options.step != None:
		n0=int(options.step.split(",")[0])
		n2=int(options.step.split(",")[1])
	else : n2=1

	if options.alignref : alignref=EMData(options.alignref,0)
	else : alignref=None

	if options.average:
		print "Averaging particles from %d to %d stepping by %d. All other options ignored !"%(n0,n1,n2)
		ptcls = []
		for i in range(n0,n1,n2):
			ptcls.append(EMData(infile,i))
		avg = sum(ptcls)/len(ptcls)
#		avg.process_inplace('normalize.edgemean')
		avg.write_image(outfile,0)
		sys.exit()

	index_d = {}
	for append_option in append_options:
		index_d[append_option] = 0

	if(n0 < 0 or n0 > nimg):
		print "Your first index is out of range, changed to zero"
		n0 = 0

	if(n1 == -1):
		n1 = nimg-1
	elif(n1 > nimg-1):
		print "Your last index is out of range, changed to %d" % (nimg-1)
		n1 = nimg-1

	# Steve:  why are all of the images being read at once !?!?. This is nuts
	datlst = parse_infile(infile, n0, n1,n2)

	logid=E2init(sys.argv,options.ppid)

	x = datlst[0].get_xsize()
	y = datlst[0].get_ysize()
	z = datlst[0].get_zsize()

	xc = x/2
	yc = y/2
	zc = z/2

	nx = x
	ny = y
	nz = z

	#print_iminfo(datlst[0], "Original")

	apix = datlst[0]["apix_x"]	# default to apix_x from file
	if options.apix:
		apix = options.apix
		for data in datlst:
			data.set_attr('apix_x', apix)
			data.set_attr('apix_y', apix)
			data.set_attr('apix_z', apix)

	if not "outtype" in optionlist:
		optionlist.append("outtype")

	if options.verbose>0:
		print "%d images, processing %d-%d (step %d)......"%(nimg, n0, n1,n2)
	#print 'start.....'
	img_index = 0
	for data in datlst:
		if options.inputto1 : data.to_one()			# replace all voxel values with 1.0
		if options.resetxf : data["xform.align3d"]=Transform()
		
		for option1 in optionlist:
			if option1 == "origin":
				if(len(options.origin)==3):
					(originx, originy, originz) = options.origin
				else:
					print ''
					return

				data.set_xyz_origin(originx, originy, originz)

			elif option1 == "calcfsc" :
				datafsc=EMData(options.calcfsc)
				fsc = data.calc_fourier_shell_correlation(datafsc)
				third = len(fsc)/3
				xaxis = fsc[0:third]
				fsc = fsc[third:2*third]
				saxis = [x/apix for x in xaxis]
				Util.save_data(saxis[1],saxis[1]-saxis[0],fsc[1:],args[1])
				print "Exiting after FSC calculation"
				sys.exit(0)

			elif option1 == "calcsf":
				dataf = data.do_fft()
				curve = dataf.calc_radial_dist(ny/2, 0, 1.0,True)
				curve=[i/(dataf["nx"]*dataf["ny"]*dataf["nz"]) for i in curve]
				Util.save_data(0, 1.0/(apix*ny), curve, options.calcsf);

			elif option1 == "setsf":
				sf=XYData()
				sf.read_file(options.setsf)
				data.process_inplace("filter.setstrucfac",{"apix":apix,"strucfac":sf})
				#dataf = data.do_fft()
				#curve = dataf.calc_radial_dist(ny, 0, 0.5,True)
				#filt=[]
				#norm=data["nx"]*data["ny"]*data["nz"]
				#for i,j in enumerate(curve):
					#if j==0 : filt.append(0.0)
					#else :
						#filt.append(sqrt(norm*sf.get_yatx(i/(apix*2.0*dataf["ny"]))/j))
						#print i,sqrt(norm*sf.get_yatx(i/(apix*2.0*dataf["ny"]))/j)
				#dataf.apply_radial_func(0,0.5/len(filt),filt)
				#data=dataf.do_ift()

				#curve2 = dataf.calc_radial_dist(ny, 0, 0.5,True)
#				plot((curve,curve2))

			elif option1 == "process":
				fi = index_d[option1]
				#print 'process option = ', options.process
				(filtername, param_dict) = parsemodopt(options.process[fi])
				if not param_dict : param_dict={}

				#Parse the options to convert the image file name to EMData object(for both plain image file and bdb file)
				for key in param_dict.keys():
					if str(param_dict[key]).find('bdb:')!=-1 or not str(param_dict[key]).isdigit():
						try:
							param_dict[key] = EMData(param_dict[key])
						except:
							pass

				data.process_inplace(filtername, param_dict)
				index_d[option1] += 1

			elif option1 == "ralignz":
#				print "ralignz ",options.ralignz[index_d[option1]]
				zalignref=EMData(options.ralignz[index_d[option1]],0)
				ary=[]
				for z in xrange(-10,11):
					zimg=data.process("xform.translate.int",{"trans":(0,0,z)})
					ary.append((zalignref.cmp("ccc",zimg),z))

				data=data.process("xform.translate.int",{"trans":(0,0,min(ary)[1])})
				data["xform.align3d"]=Transform({"type":"eman","tz":min(ary)[1]})
				if options.verbose>0 : print "Z alignment: ",min(ary)[1]

			elif option1 == "align":
				if alignref==None :
					print "Error, no alignment reference specified with --alignref"
					sys.exit(1)

				fi = index_d[option1]
				#print 'process option = ', options.process
				(alignername, param_dict) = parsemodopt(options.align[fi])
				if not param_dict : param_dict={}

				#Parse the options to convert the image file name to EMData object(for both plain image file and bdb file)
				for key in param_dict.keys():
					if str(param_dict[key]).find('bdb:')!=-1 or not str(param_dict[key]).isdigit():
						try:
							param_dict[key] = EMData(param_dict[key])
						except:
							pass

				# For 'refine' aligners, we normally want to provide a starting alignment, presumably from the previous aligner. If we can't find one with start with identity matrix
				if "refine" in alignername and not param_dict.has_key("xform.align3d"):
					try:
						param_dict["xform.align3d"]=data["xform.align3d"]
						print alignername," using xform.align3d from image"
					except:
						param_dict["xform.align3d"]=Transform()
						print alignername," didn't find a starting transform, using identity matrix."

				# actual alignment
				data=data.align(alignername,alignref, param_dict)
				if options.verbose>0 : print "Final alignment:",data["xform.align3d"]
				index_d[option1] += 1

			elif option1 == "mult":
				data.mult(options.mult)

			elif option1 == "multfile":
				mf=EMData(options.multfile[index_d[option1]],0)
				data.mult(mf)
				mf=None
				index_d[option1] += 1

#            elif option1 == "tomoshrink":
#                from e2spt_boxer import ShrunkenTomogram
#                st = ShrunkenTomogram(args[0])
#                st.set_cache_to_db(False)
#                tmp = st.get_image()

			elif option1 == "add":
				data.add(options.add)

			elif option1 == "icos5to2" :
				xf=Transform.icos_5_to_2()
				data.process_inplace("xform",{"transform":xf})

			elif option1 == "icos2to5" :
				xf=Transform.icos_5_to_2()
				xf.invert()
				data.process_inplace("xform",{"transform":xf})


			elif option1 == "trans":
				fi = index_d[option1]
				dx,dy,dz=options.trans[fi].split(",")
				data.translate(float(dx),float(dy),float(dz))
				index_d[option1] += 1

			elif option1 == "rot":
				fi = index_d[option1]
				#try:
				xform=parse_transform(options.rot[fi])
				#except:
				#	print "Invalid rotation specified: ",options.rot[fi]
				#	print "Please see e2help.py transform"

				data.transform(xform)
				index_d[option1] += 1

			elif option1 == "clip":
				if(len(options.clip) == 6):
					(nx, ny, nz, xc, yc, zc) = options.clip
				elif(len(options.clip) == 3):
					(nx, ny, nz) = options.clip
					xc = x/2
					yc = y/2
					zc = z/2
				elif(len(options.clip) ==1):
					nx = options.clip[0]
					ny=nx
					nz=nx
					xc = x/2
					yc = y/2
					zc = z/2
				else:
					print 'clip option takes 1, 3 or 6 arguments. --clip=x[,y,z[,xc,yc,zc]]'
					return

				if not (xc>=0 and yc>=0 and zc>=0 and xc<x and yc<y and zc<z):
					xc = x/2
					yc = y/2
					zc = z/2

				if x != nx or y != ny or z != nz:
					data.clip_inplace(Region(xc-nx/2, yc-ny/2, zc-nz/2, nx, ny, nz))
					index_d[option1] += 1

			elif option1 == "sym":
				sym = options.sym[index_d[option1]]
				xf = Transform()
				xf.to_identity()
				nsym=xf.get_nsym(sym)
				ref=data.copy()
				for i in range(1,nsym):
					dc=ref.copy()
					dc.transform(xf.get_sym(sym,i))
					data.add(dc)
				data.mult(1.0/nsym)

			elif option1 == "scale":
				scale_f = options.scale[index_d[option1]]
				if scale_f != 1.0:
					data.scale(scale_f)
				index_d[option1] += 1

			elif option1 == "medianshrink":
				shrink_f = options.medianshrink[index_d[option1]]
				if shrink_f > 1:
					data.process_inplace("math.medianshrink",{"n":shrink_f})
					nx = data.get_xsize()
					ny = data.get_ysize()
					nz = data.get_zsize()
				index_d[option1] += 1

			elif option1 == "meanshrink":
				shrink_f = options.meanshrink[index_d[option1]]
				if shrink_f > 1:
					data.process_inplace("math.meanshrink",{"n":shrink_f})
					nx = data.get_xsize()
					ny = data.get_ysize()
					nz = data.get_zsize()
				index_d[option1] += 1

			elif option1 == "fftclip":
				if(len(options.fftclip)==3):
					(fnx, fny, fnz) = options.fftclip
				else:
					print 'fftclip option takes either 3 arguments. --fftclip=x,y,z'
					return

				fft = data.do_fft()
				padfft = fft.get_clip(Region(0, 0, 0, fnx+2, fny, fnz))
				data = padfft.do_ift()
				index_d[option1] += 1

			elif option1 == "icos5fhalfmap":
				print "not implemented yet"

			elif option1 == "tophalf":
				half = data.get_top_half()
				data = half

			elif option1 == "outtype":
				if not options.outtype:
					options.outtype = "unknown"

		#print_iminfo(data, "Final")
		if options.outmode!="float":
			if options.outnorescale :
				# This sets the minimum and maximum values to the range for the specified type, which should result in no rescaling
				outmode=file_mode_map[options.outmode]
				data["render_min"]=file_mode_range[outmode][0]
				data["render_max"]=file_mode_range[outmode][1]
			else:
				data["render_min"]=data["minimum"]
				data["render_max"]=data["maximum"]

		if options.unstacking:	#output a series numbered single image files
			data.write_image(outfile.split('.')[0]+'-'+str(img_index+1).zfill(len(str(nimg)))+ '.' + outfile.split('.')[-1], -1, EMUtil.ImageType.IMAGE_UNKNOWN, False, None, file_mode_map[options.outmode], not(options.swap))
		else:   #output a single 2D image or a 2D stack
			if options.append:
				data.write_image(outfile, -1, EMUtil.get_image_ext_type(options.outtype), False, None, file_mode_map[options.outmode], not(options.swap))
			else:
				data.write_image(outfile, img_index, EMUtil.get_image_ext_type(options.outtype), False, None, file_mode_map[options.outmode], not(options.swap))

		img_index += 1
		for append_option in append_options:	#clean up the multi-option counter for next image
			index_d[append_option] = 0

	E2end(logid)


#parse_file() wil read the input image file and return a list of EMData() object
def parse_infile(infile, first, last, step):
	nimg = EMUtil.get_image_count(infile)

	if (nimg > 1):
		#print "it appears %s contains %d image" % (infile, nimg)
		d = EMData()
		d.read_image(infile, 0)

		x = d.get_xsize()
		y = d.get_ysize()
		z = d.get_zsize()
		if (z == 1):
			print "the images are 2D - I will now make a 3D image out of the 2D images"
			data = []
			return_data = EMData()
			return_data.set_size(x, y, nimg)
			for i in range(0, nimg):
				d.read_image(infile, i)
				return_data.insert_clip(d, (0, 0, i))
			data.append(return_data)
			return data
		else:
			print "the image is a 3D stack - I will process images from %d to %d" % (first, last)
			data = []
			for i in xrange(first, last+1, step):
				d = EMData()
				d.read_image(infile, i)
				data.append(d)
			return data
	else:
		data = []
		d = EMData()
		d.read_image(infile,0)
		data.append(d)
		return data


if __name__ == "__main__":
	main()
