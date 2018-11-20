#!/usr/bin/env python
from __future__ import print_function
#
# Author: Pawel A.Penczek and Edward H. Egelman 05/27/2009 (Pawel.A.Penczek@uth.tmc.edu)
# Copyright (c) 2000-2006 The University of Texas - Houston Medical School
# Copyright (c) 2008-Forever The University of Virginia
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
#

import applications
import global_def
import optparse
import os
import string
import sys
import utilities
from builtins import range


      
def main():
    progname = os.path.basename(sys.argv[0])
    usage = progname + " input_stack start end output_stack  <mask> --rad=mask_radius"
    parser = optparse.OptionParser(usage, version=global_def.SPARXVERSION)
    parser.add_option("--rad",     type="int", default=-1, help="radius of mask")
    parser.add_option("--verbose", type="int", default=0,  help="verbose level (0|1)")


    (options, args) = parser.parse_args()

    

    if len(args) < 4:
        print("usage: " + usage)
        print("Please run '" + progname + " -h' for details")
    else:
        pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT from string import atoi
        input_stack  = args[0]
        imgstart     = string.atoi( args[1] )
        imgend       = string.atoi( args[2] ) +1
        output_stack = args[3]
        if(len(args) == 5):  mask = args[4]
        else:               mask = None

    if global_def.CACHE_DISABLE:
        pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT from utilities import disable_bdb_cache
        utilities.disable_bdb_cache()
    pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT pass#IMPORTIMPORTIMPORT from applications import varimax
    global_def.BATCH = True
    applications.varimax(input_stack, list(range(imgstart, imgend)), output_stack, mask, options.rad, options.verbose)
    global_def.BATCH = False

if __name__ == "__main__":
	main()     
