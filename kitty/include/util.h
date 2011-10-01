#ifndef KITTY_UTIL_H
#define KITTY_UTIL_H
//
//  util.h
//  -- a collection of useful utility functions that are not specific to a certain module
//
//  Created by Feldkamp on 9/28/11.
//  Copyright 2011 SLAC National Accelerator Laboratory. All rights reserved.
//

#include "constants.h"
#include "kitty/arrayclasses.h"

#include <string>
		

//-------------functions for the CSPAD detector-------------
void create1DFromRawImageCSPAD( array2D *input, array1D *output );

void createRawImageCSPAD( array1D *input, array2D *output );

void createAssembledImageCSPAD( array1D *input, array1D *pixX, array1D *pixY, array2D *output );
			


std::string getExt(std::string filename);
			
#endif
