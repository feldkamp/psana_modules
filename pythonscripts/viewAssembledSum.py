#!/usr/bin/python

# Usage:
# In this directory, type:
#    ./viewAssembledSum.py -rxxxx -a (optional)
# For details, type 
#	 python viewAssembledSum.py --help
# where rxxxx is the run number of hits and nonhits found using the hitfinder executable. 
# By default, this script looks into the h5 files that are in the appropriate rxxxx directory
#

########################################################
# Edit this variable accordingly
# Files are read for source_dir/runtag and
# written to write_dir/runtag.
# Be careful of the trailing "/"; 
# ensure you have the necessary read/write permissions.
########################################################
source_dir_default = "/reg/d/psdm/cxi/cxi35711/res/data/"
#source_dir_default = "/reg/d/psdm/cxi/cxi35711/scratch/data/"
write_dir_default = "/reg/neh/home/sellberg/CCA-2011/analysis/psana/figures/"


import os
import sys
import string
import re
from optparse import OptionParser


parser = OptionParser()
parser.add_option("-r", "--run", action="store", type="string", dest="runNumber", 
					help="run number you wish to view", metavar="XXXX", default="")
parser.add_option("-t", "--tag", action="store", type="string", dest="fileTag",
		  			help="file tag for run (default: img)", metavar="FILETAG", default="img")
parser.add_option("-a", "--avg", action="store_true", dest="ang_average", 
					help="flag to compute angular average", default=False)
parser.add_option("-m", "--max", action="store", type="float", dest="maxValue", 
					help="mask out pixels above this value", metavar="MAXVALUE", default="10000000")
parser.add_option("-i", "--input_dir", action="store", type="string", dest="source_dir",
					help="input directory", metavar="INPUTDIR", default=source_dir_default)
parser.add_option("-o", "--output_dir", action="store", type="string", dest="write_dir",
					help="output directory", metavar="OUTPUTDIR", default=write_dir_default)

(options, args) = parser.parse_args()

import numpy as N
import h5py as H

import matplotlib
import matplotlib.pyplot as P


source_dir = options.source_dir
write_dir = options.write_dir

runtag = "r%s"%(options.runNumber)
if os.path.exists(source_dir+runtag+"/"+options.fileTag+"_avg_asm2D.h5"):
	print source_dir+runtag+"/"+options.fileTag+"_avg_asm2D.h5"
	f = H.File(source_dir+runtag+"/"+options.fileTag+"_avg_asm2D.h5","r")
	d = N.array(f['/data/data'])
	d *= (d < options.maxValue)
	f.close()
else:
	print "Source file "+source_dir+runtag+"/"+options.fileTag+"_avg_asm2D.h5 not found, aborting script."
	sys.exit(1)

vx = N.arange(-868,868)
vy = N.arange(-870,870)
X,Y = N.meshgrid(vx,vx)
arr = (N.sqrt(X*X + Y*Y)).astype(int)

lenOfAvg = len(set(arr.flatten()))+5
avg_count = N.zeros(lenOfAvg)
avg_vals = N.zeros(lenOfAvg)
avg_var = N.zeros(lenOfAvg)

intensMask = (d.flatten()>0.)

#blockMask=N.zeros((1731,1731))
#blockMask[:950,950:1300]=1.
#intensMask *= blockMask.flatten()

########################################################
# Imaging class copied from Ingrid Ofte's pyana_misc code
########################################################
class img_class (object):
	def __init__(self, inarr, filename):
		self.inarr = inarr
		self.filename = filename
		self.cmax = self.inarr.max()
		self.cmin = self.inarr.min()
	
	
	def on_keypress(self,event):
		if event.key == 'p':
			if not os.path.exists(write_dir + runtag):
				os.mkdir(write_dir + runtag)
			pngtag = write_dir + runtag + "/%s.png" % (self.filename)	
			print "saving image as " + pngtag 
			P.savefig(pngtag)
		if event.key == 'r':
			P.clim(self.cmin, self.cmax)
			P.draw()


	def on_click(self, event):
		if event.inaxes:
			lims = self.axes.get_clim()
			colmin = lims[0]
			colmax = lims[1]
			range = colmax - colmin
			value = colmin + event.ydata * range
			if event.button is 1 :
				if value > colmin and value < colmax :
					colmin = value
			elif event.button is 2 :
				colmin, colmax = self.orglims
			elif event.button is 3 :
				if value > colmin and value < colmax:
					colmax = value
			P.clim(colmin, colmax)
			P.draw()
				

	def draw_img(self):
		fig = P.figure()
		cid1 = fig.canvas.mpl_connect('key_press_event', self.on_keypress)
		cid2 = fig.canvas.mpl_connect('button_press_event', self.on_click)
		canvas = fig.add_subplot(111)
		canvas.set_title(self.filename)
		self.axes = P.imshow(self.inarr, vmin = 0, vmax = self.cmax)
		self.colbar = P.colorbar(self.axes, pad=0.01)
		self.orglims = self.axes.get_clim()
		P.show()

print "Right-click on colorbar to set maximum scale."
print "Left-click on colorbar to set minimum scale."
print "Center-click on colorbar (or press 'r') to reset color scale."
print "Interactive controls for zooming at the bottom of figure screen (zooming..etc)."
print "Press 'p' to save PNG of image (with the current colorscales) in the appropriately named folder."
print "Hit Ctl-\ or close all windows (Alt-F4) to terminate viewing program."

if (options.ang_average is True):
	compressedIntens = N.compress(intensMask, d.flatten())
	compressedPos = N.compress(intensMask, arr.flatten())

	for i,j in zip(compressedPos,compressedIntens):
		avg_count[i] += 1.
		avg_vals[i] += j
		avg_var[i] += j*j

	for i in range(len(avg_count)):
		if(avg_count[i] > 0.):
			avg_vals[i] /= avg_count[i]
			avg_var[i] /= avg_count[i]

	fig = P.figure()
	P.plot(avg_vals)
	canvas = fig.add_subplot(111)
	canvas.set_title(runtag + "_avg")
	P.xlabel("Q")
	P.ylabel("I(Q)")
	P.draw()
	#avg_vals.tofile(write_dir + runtag + "_ang_avg.dat", sep="\n",format="%e")

currImg = img_class(d, runtag)
currImg.draw_img()

