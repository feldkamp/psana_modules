//
//  util.cpp
//  ana
//
//  Created by Feldkamp on 9/28/11.
//  Copyright 2011 SLAC National Accelerator Laboratory. All rights reserved.
//

#include "kitty/util.h"
using namespace kitty;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <string>
using std::string;

#include <cmath>

//------------------------------------------------------------- create1DFromRawImageCSPAD
// inverse function of "createRawImageCSPAD"
// make a 1D array from a written 2D "raw" image
//
//-------------------------------------------------------------
int create1DFromRawImageCSPAD( array2D *input, array1D *&output ){
	if (!input){
		cerr << "Error in create1DFromRawImageCSPAD. No input. Aborting..." << endl;
		return 1;
	}
	
	delete output;
	output = new array1D( nMaxTotalPx );

	//transpose to be conform with psana's convention
	input->transpose();

	//sort into 1D data
	for (int q = 0; q < nMaxQuads; q++){
		int superrow = q*nRowsPer2x1;
		for (int s = 0; s < nMax2x1sPerQuad; s++){
			int supercol = s*nColsPer2x1;
			for (int c = 0; c < nColsPer2x1; c++){
				for (int r = 0; r < nRowsPer2x1; r++){
					output->set( q*nMaxPxPerQuad + s*nPxPer2x1 + c*nRowsPer2x1 + r,
						input->get( superrow+r, supercol+c ) );
				}
			}
		}
	}
	
	//transpose again to not screw up the original array....
	//the two transposes are obviously not the smart way to do it... needs a fix at a certain point
	input->transpose();
	return 0;
}


//------------------------------------------------------------- createRawImageCSPAD
// function to create 'raw' CSPAD images from plain 1D data, where
// dim1 : 388 : rows of a 2x1
// dim2 : 185 : columns of a 2x1
// dim3 :   8 : 2x1 sections in a quadrant (align as super-columns)
// dim4 :   4 : quadrants (align as super-rows)
//
//    +--+--+--+--+--+--+--+--+
// q0 |  |  |  |  |  |  |  |  |
//    |  |  |  |  |  |  |  |  |
//    +--+--+--+--+--+--+--+--+
// q1 |  |  |  |  |  |  |  |  |
//    |  |  |  |  |  |  |  |  |
//    +--+--+--+--+--+--+--+--+
// q2 |  |  |  |  |  |  |  |  |
//    |  |  |  |  |  |  |  |  |
//    +--+--+--+--+--+--+--+--+
// q3 |  |  |  |  |  |  |  |  |
//    |  |  |  |  |  |  |  |  |
//    +--+--+--+--+--+--+--+--+
//     s0 s1 s2 s3 s4 s5 s6 s7
//-------------------------------------------------------------
int createRawImageCSPAD( array1D *input, array2D *&output ){
	if (!input){
		cerr << "Error in createRawImageCSPAD. No input. Aborting..." << endl;
		return 1;
	}

	delete output;
	output = new array2D( nMaxQuads*nRowsPer2x1, nMax2x1sPerQuad*nColsPer2x1 );
	
	//sort into ordered 2D data
	for (int q = 0; q < nMaxQuads; q++){
		int superrow = q*nRowsPer2x1;
		for (int s = 0; s < nMax2x1sPerQuad; s++){
			int supercol = s*nColsPer2x1;
			for (int c = 0; c < nColsPer2x1; c++){
				for (int r = 0; r < nRowsPer2x1; r++){
					output->set( superrow+r, supercol+c, 
						input->get( q*nMaxPxPerQuad + s*nPxPer2x1 + c*nRowsPer2x1 + r ) );
				}
			}
		}
	}
	
	//transpose to be conform with cheetah's convention
	output->transpose();
	return 0;
}



//------------------------------------------------------------- createAssembledImageCSPAD
// expects 'pixX/Y' arrays to contain pixel count coordinates for each value in 'input'
//-------------------------------------------------------------
int createAssembledImageCSPAD( array1D *input, array1D *pixX, array1D *pixY, array2D *&output ){
	if (!input){
		cerr << "Error in createAssembledImageCSPAD. No input. Aborting..." << endl;
		return 1;
	}
	
	const double xmax = pixX->calcMax();
	const double xmin = pixX->calcMin();
	const double ymax = pixY->calcMax();
	const double ymin = pixY->calcMin();
	
	// calculate range of values in pixel arrays 
	// --> this will be the number of pixels in assembled image (add a safety margin)
	const int NX_CSPAD = (int) ceil(xmax-xmin) + 1;
	const int NY_CSPAD = (int) ceil(ymax-ymin) + 1;
	
	if (input->size() != pixX->size() || input->size() != pixY->size() ){
		cerr << "Error in array2D::createAssembledImageCSPAD! Array sizes don't match. Aborting!" << endl;
		cerr << "size() of data:" << input->size() << ", pixX:" << pixX->size() << ", pixY:" << pixY->size() << endl;
		cerr << "dim1() of data:" << input->dim1() << ", pixX:" << pixX->dim1() << ", pixY:" << pixY->dim1() << endl;
		return 2;
	}else{
		//cout << "Assembling CSPAD image. Output (" << NX_CSPAD << ", " << NY_CSPAD << ")" << endl;
	}
	
	//shift output arrays, if necessary, so that they start at zero
	pixX->subtractValue( xmin );
	pixY->subtractValue( ymin );
	
	delete output;
	output = new array2D( NY_CSPAD, NX_CSPAD );

	//sort into ordered 2D data
	for (int q = 0; q < nMaxQuads; q++){
		for (int s = 0; s < nMax2x1sPerQuad; s++){
			for (int c = 0; c < nColsPer2x1; c++){
				for (int r = 0; r < nRowsPer2x1; r++){
					int index = q*nMaxPxPerQuad + s*nPxPer2x1 + c*nRowsPer2x1 + r;
					output->set( (int)pixY->get(index), (int)pixX->get(index), input->get(index) );
				}
			}
		}
	}
	
	output->transpose();
	return 0;
}


//-------------------------------------------------------------getExt

//-------------------------------------------------------------
std::string getExt(std::string filename){
	char separator = '.';
	//find separator dot from the end of the string
	size_t pos = filename.rfind( separator );
	//get extension without the '.'
	string ext = filename.substr( pos+1, filename.length() );
	return ext;
}


