#!/usr/bin/python

# Usage:
# In this directory, type:
#    ./viewAngAvgSum.py -rxxxx
# For details, type 
#	 python viewAngAvgSum.py --help
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
source_dir_default = "/reg/d/psdm/cxi/cxi35711/scratch/data/"
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
parser.add_option("-m", "--min", action="store", type="float", dest="MINVALUE", 
					help="ignore intensities below this q-value", metavar="min_value", default="0")
parser.add_option("-x", "--max", action="store", type="float", dest="MAXVALUE", 
					help="ignore intensities above this q-value", default="100000")
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
if os.path.exists(source_dir+runtag+"/"+options.fileTag+"_avg_iAvg.h5"):
	print source_dir+runtag+"/"+options.fileTag+"_avg_iAvg.h5"
	f = H.File(source_dir+runtag+"/"+options.fileTag+"_avg_iAvg.h5","r")
	iAvg = N.array(f['/data/data'])
	f.close()
else:
	print "Source file "+source_dir+runtag+"/"+options.fileTag+"_avg_iAvg.h5 not found, aborting script."
	sys.exit(1)

if os.path.exists(source_dir+runtag+"/"+options.fileTag+"_avg_qAvg.h5"):
        print source_dir+runtag+"/"+options.fileTag+"_avg_qAvg.h5"
        f = H.File(source_dir+runtag+"/"+options.fileTag+"_avg_qAvg.h5","r")
        d = N.array(f['/data/data'])
	if len(d[0]) == len(iAvg):
		qAvg = d[0]
	else:
		qAvg = False
        f.close()
else:
	print "Source file "+source_dir+runtag+"/"+options.fileTag+"_avg_qAvg.h5 not found."

#for i in N.arange(len(iAvg)):
#    print "Q: %f, I: %f"%(qAvg[i], iAvg[i])

print "Interactive controls for zooming at the bottom of figure screen (zooming..etc)."
print "Hit Ctl-\ or close all windows (Alt-F4) to terminate viewing program."

def on_keypress(event):
	if event.key == 'p':
		if not os.path.exists(write_dir + runtag):
			os.mkdir(write_dir + runtag)
		pngtag = write_dir + runtag + "/%s_angavg.png" % (runtag)
		print "saving image as " + pngtag
		P.savefig(pngtag)

fig = P.figure()
cid1 = fig.canvas.mpl_connect('key_press_event', on_keypress)
if (qAvg is not False):
	P.plot(qAvg, iAvg)
	canvas = fig.add_subplot(111)
	canvas.set_title(runtag + "_angavg")
	P.xlabel("Q (pixels)")
else:
	P.plot(N.arange(100,802,2), iAvg)
	canvas = fig.add_subplot(111)
	canvas.set_title(runtag + "_angavg")
	P.xlabel("Q (pixels)")
P.ylabel("I(Q)")
P.show()
