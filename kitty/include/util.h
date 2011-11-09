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
		

//#define WAIT usleep(1000000)
#define WAIT std::cerr << "continue by pressing the <return> key..."; std::cin.get();


//-------------functions for the CSPAD detector-------------
int create1DFromRawImageCSPAD( array2D *input, array1D *&output );

int createRawImageCSPAD( array1D *input, array2D *&output );

int createAssembledImageCSPAD( array1D *input, array1D *pixX, array1D *pixY, array2D *&output );
			


			
#endif
