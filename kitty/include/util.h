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
int create1DFromRawImageCSPAD( const array2D *input, array1D *&output );

int createRawImageCSPAD( const array1D *input, array2D *&output );

int createAssembledImageCSPAD( const array1D *input, const array1D *pixX, const array1D *pixY, array2D *&output );
			


			
#endif
