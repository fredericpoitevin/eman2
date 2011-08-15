#!/usr/bin/env python

#
# Author: Steven Ludtke, 07/28/2011 (sludtke@bcm.edu)
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  2111-1307 USA
#
#

from EMAN2 import *
from EMAN2db import db_open_dict, db_close_dict, db_check_dict, db_list_dicts
from optparse import OptionParser
from OpenGL import GL,GLUT
from math import *
import os
import sys
import weakref
import e2ctf

try:
	from PyQt4 import QtCore, QtGui, QtOpenGL
	from PyQt4.QtCore import Qt
	from emshape import *
	from valslider import *
except:
	print "Warning: PyQt4 must be installed to use the --gui option"
	class dummy:
		pass
	class QWidget:
		"A dummy class for use when Qt not installed"
		def __init__(self,parent):
			print "Qt4 has not been loaded"
	QtGui=dummy()
	QtGui.QWidget=QWidget


from Simplex import Simplex



debug=False
logid=None

sfcurve=None		# This will store a global structure factor curve if specified
sfcurve2=None
envelopes=[]		# simplex minimizer needs to use a global at the moment

def main():
	global debug,logid
	progname = os.path.basename(sys.argv[0])
	usage = """%prog [options] <single image file> ...

This program will allow you to evaluate an individual scanned micrograph or CCD frame, by looking at its
power spectrum in various ways."""

	parser = OptionParser(usage=usage,version=EMANVERSION)

	parser.add_option("--gui",action="store_true",help="This is a GUI-only program. This option is provided for self-consistency",default=True)
	parser.add_option("--verbose", "-v", dest="verbose", action="store", metavar="n", type="int", default=0, help="verbose level [0-9], higner number means higher level of verboseness")
	
	(options, args) = parser.parse_args()
	
	logid=E2init(sys.argv)

	from emapplication import EMApp
	app=EMApp()
	gui=GUIEvalImage(args)
	gui.show()
	app.execute()

	E2end(logid)


		
class GUIEvalImage(QtGui.QWidget):
	def __init__(self,images):
		"""Implements the CTF fitting dialog using various EMImage and EMPlot2D widgets
		'data' is a list of (filename,ctf,im_1d,bg_1d,im_2d,bg_2d)
		"""
		try:
			from emimage2d import EMImage2DWidget
		except:
			print "Cannot import EMAN image GUI objects (EMImage2DWidget)"
			sys.exit(1)
		try: 
			from emplot2d import EMPlot2DWidget
		except:
			print "Cannot import EMAN plot GUI objects (is matplotlib installed?)"
			sys.exit(1)
		
		QtGui.QWidget.__init__(self,None)
		self.setWindowIcon(QtGui.QIcon(get_image_directory() + "ctf.png"))
		
		self.data=None
		self.curset=0
		self.calcmode=0
		self.f2dmode=0
		self.plotmode=0
		
		# Per image parameters to keep track of
		# for each image [box size,ctf,box coord,set of excluded boxnums]
		self.parms=[[512,EMAN2Ctf(),(256,256),set()] for i in images]	
		
		self.wimage=EMImage2DWidget()
		
		self.wfft=EMImage2DWidget()
		self.wplot=EMPlot2DWidget()
		
		self.wimage.connect(self.wimage,QtCore.SIGNAL("mousedown"),self.imgmousedown)
		self.wimage.connect(self.wimage,QtCore.SIGNAL("mousedrag"),self.imgmousedrag)
		self.wimage.connect(self.wimage,QtCore.SIGNAL("mouseup")  ,self.imgmouseup)
		self.wfft.connect(self.wfft,QtCore.SIGNAL("mousedown"),self.fftmousedown)
		self.wfft.connect(self.wfft,QtCore.SIGNAL("mousedrag"),self.fftmousedrag)
		self.wfft.connect(self.wfft,QtCore.SIGNAL("mouseup")  ,self.fftmouseup)
		self.wplot.connect(self.wplot,QtCore.SIGNAL("mousedown"),self.plotmousedown)
		
		self.wimage.mmode="app"
		self.wfft.mmode="app"

		# This object is itself a widget we need to set up
		self.hbl = QtGui.QHBoxLayout(self)
		self.hbl.setMargin(0)
		self.hbl.setSpacing(6)
		self.hbl.setObjectName("hbl")
		
		# plot list and plot mode combobox
		self.vbl2 = QtGui.QVBoxLayout()
		self.setlist=QtGui.QListWidget(self)
		self.setlist.setSizePolicy(QtGui.QSizePolicy.Preferred,QtGui.QSizePolicy.Expanding)
		for i in images:
			self.setlist.addItem(i)
		self.vbl2.addWidget(self.setlist)
		
		self.scalcmode=QtGui.QComboBox(self)
		self.scalcmode.addItem("Single Region")
		self.scalcmode.addItem("Tiled Boxes")
		self.vbl2.addWidget(self.scalcmode)
		
		self.s2dmode=QtGui.QComboBox(self)
		self.s2dmode.addItem("Power Spectrum")
		self.s2dmode.addItem("Bg Subtracted")
		self.s2dmode.addItem("Background")
		self.vbl2.addWidget(self.s2dmode)
		
		self.splotmode=QtGui.QComboBox(self)
		self.splotmode.addItem("Bgsub and Fit")
		self.splotmode.addItem("Fg and Bg")
		self.vbl2.addWidget(self.splotmode)

		self.hbl.addLayout(self.vbl2)
		
		# ValSliders for CTF parameters
		self.vbl = QtGui.QVBoxLayout()
		self.vbl.setMargin(0)
		self.vbl.setSpacing(6)
		self.vbl.setObjectName("vbl")
		self.hbl.addLayout(self.vbl)
		
		#self.samp = ValSlider(self,(0,5.0),"Amp:",0)
		#self.vbl.addWidget(self.samp)
				
		self.sdefocus=ValSlider(self,(0,5),"Defocus:",1.0,90)
		self.vbl.addWidget(self.sdefocus)
		
		self.sbfactor=ValSlider(self,(0,1600),"B factor:",200.0,90)
		self.vbl.addWidget(self.sbfactor)
		
		self.sampcont=ValSlider(self,(0,100),"% AC",10.0,90)
		self.vbl.addWidget(self.sampcont)
		
#		self.sapix=ValSlider(self,(.2,10),"A/Pix:",2,90)
#		self.vbl.addWidget(self.sapix)

		self.hbl2 = QtGui.QHBoxLayout()

		self.sboxsize=ValBox(self,(0,500),"Box Size:",256,90)
		self.sboxsize.intonly=True
		self.hbl2.addWidget(self.sboxsize)

		self.sapix=ValBox(self,(0,500),"A/pix:",1.0,90)
		self.hbl2.addWidget(self.sapix)

		self.svoltage=ValBox(self,(0,500),"Voltage (kV):",200,90)
		self.hbl2.addWidget(self.svoltage)
		
		self.scs=ValBox(self,(0,5),"Cs (mm):",4.1,90)
		self.hbl2.addWidget(self.scs)		
		
		self.vbl.addLayout(self.hbl2)
		
		#self.hbl_buttons = QtGui.QHBoxLayout()
		#self.saveparms = QtGui.QPushButton("Save parms")
		#self.recallparms = QtGui.QPushButton("Recall")
		#self.refit = QtGui.QPushButton("Refit")
		#self.output = QtGui.QPushButton("Output")
		#self.hbl_buttons.addWidget(self.refit)
		#self.hbl_buttons.addWidget(self.saveparms)
		#self.hbl_buttons.addWidget(self.recallparms)
		#self.hbl_buttons2 = QtGui.QHBoxLayout()
		#self.hbl_buttons2.addWidget(self.output)
		#self.vbl.addLayout(self.hbl_buttons)
		#self.vbl.addLayout(self.hbl_buttons2)
		
		QtCore.QObject.connect(self.sdefocus, QtCore.SIGNAL("valueChanged"), self.newCTF)
		QtCore.QObject.connect(self.sbfactor, QtCore.SIGNAL("valueChanged"), self.newCTF)
		QtCore.QObject.connect(self.sapix, QtCore.SIGNAL("valueChanged"), self.newCTF)
		QtCore.QObject.connect(self.sampcont, QtCore.SIGNAL("valueChanged"), self.newCTF)
		QtCore.QObject.connect(self.svoltage, QtCore.SIGNAL("valueChanged"), self.newCTF)
		QtCore.QObject.connect(self.scs, QtCore.SIGNAL("valueChanged"), self.newCTF)
		QtCore.QObject.connect(self.sboxsize, QtCore.SIGNAL("valueChanged"), self.newBox)
		QtCore.QObject.connect(self.setlist,QtCore.SIGNAL("currentRowChanged(int)"),self.newSet)
		QtCore.QObject.connect(self.scalcmode,QtCore.SIGNAL("currentIndexChanged(int)"),self.newCalcMode)
		QtCore.QObject.connect(self.s2dmode,QtCore.SIGNAL("currentIndexChanged(int)"),self.new2DMode)
		QtCore.QObject.connect(self.splotmode,QtCore.SIGNAL("currentIndexChanged(int)"),self.newPlotMode)

	   	#QtCore.QObject.connect(self.saveparms,QtCore.SIGNAL("clicked(bool)"),self.on_save_params)
		#QtCore.QObject.connect(self.recallparms,QtCore.SIGNAL("clicked(bool)"),self.on_recall_params)
		#QtCore.QObject.connect(self.refit,QtCore.SIGNAL("clicked(bool)"),self.on_refit)
		#QtCore.QObject.connect(self.output,QtCore.SIGNAL("clicked(bool)"),self.on_output)
		
		
		self.resize(720,380) # figured these values out by printing the width and height in resize event
		
		
		self.setWindowTitle("CTF")
#		self.recalc()

		
	def closeEvent(self,event):
#		QtGui.QWidget.closeEvent(self,event)
		event.accept()
		QtGui.qApp.exit(0)
		#app=QtGui.qApp
		#if self.wimage != None:
			#app.close_specific(self.wimage)
			#self.wimage = None 
		#if self.wfft != None:
			#app.close_specific(self.wfft)
		#if self.wplot != None:
			#app.close_specific(self.wplot)
		#app.close_specific(self)
#		self.emit(QtCore.SIGNAL("module_closed")) # this signal is important when e2ctf is being used by a program running its own event loop

	def update_plot(self):
#		if self.wplot == None: return # it's closed/not visible
		parms=self.parms[self.curset]
		apix=self.sapix.getValue()
		ds=1.0/(apix*parms[0])
		ctf=parms[1]
		bg1d=ctf.background
		r=len(ctf.background)
		s=[ds*i for i in range(r)]
		
		# This updates the FFT image circles
		fit=ctf.compute_1d(len(s)*2,ds,Ctf.CtfType.CTF_AMP)
		shp={}
		nz=0
		for i in range(1,len(fit)):
			if fit[i-1]*fit[i]<=0.0: 
				nz+=1
				shp["z%d"%i]=EMShape(("circle",0.0,0.0,1.0/nz,r,r,i,1.0))
		
		self.wfft.del_shapes()
		self.wfft.add_shapes(shp)
		self.wfft.updateGL()
		
		# Now update the plots for the correct plot mode
		if self.plotmode==0: 
			bgsub=[self.fft1d[i]-bg1d[i] for i in range(r)]
			self.wplot.set_data((s,bgsub),"fg-bg",True,True,color=0)
			
			fit=ctf.compute_1d(len(s)*2,ds,Ctf.CtfType.CTF_AMP)		# The fit curve
			fit=[fit[i]**2 for i in range(len(s))]		# squared, no SF

			# auto-amplitude for b-factor adjustment
			rto,nrto=0,0
			for i in range(int(.04/ds)+1,min(int(0.15/ds),len(s)-1)): 
				if bgsub[i]>0 : 
					rto+=fit[i]
					nrto+=fabs(bgsub[i])
			if nrto==0 : rto=1.0
			else : rto/=nrto
			fit=[fit[i]/rto for i in range(len(s))]
			
#			print ctf_cmp((self.sdefocus.value,self.sbfactor.value,rto),(ctf,bgsub,int(.04/ds)+1,min(int(0.15/ds),len(s)-1),ds,self.sdefocus.value))
			
			self.wplot.set_data((s,fit),"fit",color=1)
			self.wplot.setAxisParms("s (1/"+ u"\u212B" + ")","Intensity (a.u)")
		elif self.plotmode==1:
			self.wplot.set_data((s[1:],self.fft1d[1:]),"fg",True,True,color=1)
			self.wplot.set_data((s[1:],bg1d[1:]),"bg",color=0)
			self.wplot.setAxisParms("s (1/"+ u"\u212B" +")","Intensity (a.u)")


	def newSet(self,val):
		"called when a new data set is selected from the list"
		self.curset=val
		self.data=EMData(str(self.setlist.item(val).text()),0)	# read the image from disk
		self.wimage.set_data(self.data)

		ctf=self.parms[val][1]
		# if voltage is 0, we need to initialize
		if ctf.voltage==0:
			ctf.voltage=self.data["microscope_voltage"]
			if ctf.voltage==0 : ctf.voltage=200.0
			ctf.cs=self.data["microscope_cs"]
			if ctf.cs==0 : ctf.cs=4.1
			ctf.apix=self.data["apix_x"]
			ctf.defocus=1.0
			ctf.bfactor=200.0
			ctf.ampcont=10.0

		self.sdefocus.setValue(ctf.defocus,True)
		self.sbfactor.setValue(ctf.bfactor,True)
		self.sapix.setValue(ctf.apix,True)
		self.sampcont.setValue(ctf.ampcont,True)
		self.svoltage.setValue(ctf.voltage,True)
		self.scs.setValue(ctf.cs,True)
		self.sboxsize.setValue(self.parms[val][0],True)
		
		#if self.guiim != None: 
##			print self.data
			#self.guiim.set_data(self.data[val][4])
			#if self.guiiminit:
				#self.guiim.optimally_resize()
				#self.guiiminit = False
			#self.guiim.updateGL()
		self.recalc()

	def recalc(self):
		"Called to recompute the power spectra, also updates plot"
	
		self.wimage.show()
		self.wfft.show()
		self.wplot.show()

	
		# To simplify expressions
		parms=self.parms[self.curset]
		apix=self.sapix.getValue()
		ds=1.0/(apix*parms[0])

		# Mode where user drags the box around the parent image
		if self.calcmode==0:
			# update the box display on the image 
			self.wimage.del_shapes()
			self.wimage.add_shape("box",EMShape(("rect",.3,.9,.3,parms[2][0],parms[2][1],parms[2][0]+parms[0],parms[2][1]+parms[0],1)))
			self.wimage.updateGL()
			
			# extract the data and do an fft
			clip=self.data.get_clip(Region(parms[2][0],parms[2][1],parms[0],parms[0]))
			self.fft=clip.do_fft()
			
		# mode where user selects/deselcts tiled image set
		elif self.calcmode==1:
			# update the box display on the image
			nx=self.data["nx"]/parms[0]-1
			self.fft=None
			shp={}
			for x in range(nx):
				for y in range(self.data["ny"]/parms[0]-1):
					# User deselected this one
					if int(x+y*nx) in parms[3] : continue
					
					# Make a shape for this box
					shp["box%02d%02d"%(x,y)]=EMShape(("rect",.3,.9,.3,(x+.5)*parms[0],(y+.5)*parms[0],(x+1.5)*parms[0],(y+1.5)*parms[0],1))
					
					# read the data and make the FFT
					clip=self.data.get_clip(Region(x*parms[0]+parms[0]/2,y*parms[0]+parms[0]/2,parms[0],parms[0]))
					fft=clip.do_fft()
					fft.ri2inten()
					if self.fft==None: self.fft=fft
					else: self.fft+=fft
			
			self.fft.process_inplace("math.sqrt")
			self.fft["is_intensity"]=0				# These 2 steps are done so the 2-D display of the FFT looks better. Things would still work properly in 1-D without it
			self.fft.process_inplace("normalize")
			
			self.wimage.del_shapes()
			self.wimage.add_shapes(shp)
			self.wimage.updateGL()
			

		self.fft1d=self.fft.calc_radial_dist(self.fft.get_ysize()/2,0.0,1.0,1)	# note that this handles the ri2inten averages properly

		# Compute 1-D curve and background
		bg_1d=e2ctf.low_bg_curve(self.fft1d,ds)
		parms[1].background=bg_1d
		parms[1].dsbg=ds
		
		if self.f2dmode>0 :
			self.fftbg=self.fft.process("math.nonconvex")
			if self.f2dmode==1 : self.wfft.set_data(self.fft-self.fftbg)
			else : self.wfft.set_data(self.fftbg)

		else :
			self.wfft.set_data(self.fft)

		# Fitting not done yet. Need to make 2D background somehow
#		if parms[1].defocus==0:
#			parms[1]=e2ctf.ctf_fit(self.fft1d,bg_1d,bg_1d,self.fft,bg_2d,options.voltage,options.cs,options.ac,apix,bgadj=not options.nosmooth,autohp=options.autohp,dfhint=dfhint,verbose=options.verbose)
			
		self.update_plot()

	def newCalcMode(self,mode):
		self.calcmode=mode
		self.recalc()

	def new2DMode(self,mode):
		self.f2dmode=mode
		self.recalc()

	def newPlotMode(self,mode):
		self.plotmode=mode
		self.recalc()

	def newBox(self):
		parms=self.parms[self.curset]
		parms[0]=self.sboxsize.value
		self.recalc()

	def newCTF(self) :
		parms=self.parms[self.curset]
		parms[1].defocus=self.sdefocus.value
		parms[1].bfactor=self.sbfactor.value
		parms[1].apix=self.sapix.value
		parms[1].ampcont=self.sampcont.value
		parms[1].voltage=self.svoltage.value
		parms[1].cs=self.scs.value
		self.update_plot()
		

	def imgmousedown(self,event) :
		if self.calcmode==0:
			m=self.wimage.scr_to_img((event.x(),event.y()))
			parms=self.parms[self.curset]
			parms[2]=(m[0]-parms[0]/2,m[1]-parms[0]/2)
			self.recalc()
		#self.guiim.add_shape("cen",["rect",.9,.9,.4,x0,y0,x0+2,y0+2,1.0])
		
	def imgmousedrag(self,event) :
		if self.calcmode==0:
			m=self.wimage.scr_to_img((event.x(),event.y()))
			parms=self.parms[self.curset]
			parms[2]=(m[0]-parms[0]/2,m[1]-parms[0]/2)
			self.recalc()
		
		# box deletion when shift held down
		#if event.modifiers()&Qt.ShiftModifier:
			#for i,j in enumerate(self.boxes):
		
	def imgmouseup(self,event) :
		m=self.wimage.scr_to_img((event.x(),event.y()))
		if self.calcmode==1:
			parms=self.parms[self.curset]
			nx=self.data["nx"]/parms[0]-1
			grid=int((m[0]-parms[0]/2)/parms[0])+int((m[1]-parms[0]/2)/parms[0])*nx
			if grid in parms[3] : parms[3].remove(grid)
			else: parms[3].add(grid)
			self.recalc()
			
			
	def fftmousedown(self,event) :
		m=self.wfft.scr_to_img((event.x(),event.y()))
		#self.guiim.add_shape("cen",["rect",.9,.9,.4,x0,y0,x0+2,y0+2,1.0])
		
	def fftmousedrag(self,event) :
		m=self.wfft.scr_to_img((event.x(),event.y()))
		
		# box deletion when shift held down
		#if event.modifiers()&Qt.ShiftModifier:
			#for i,j in enumerate(self.boxes):
		
	def fftmouseup(self,event) :
		m=self.wfft.scr_to_img((event.x(),event.y()))


	def plotmousedown(self,event) :
		"mousedown in plot"
#		m=self.guiim.scr_to_img((event.x(),event.y()))



if __name__ == "__main__":
	main()
