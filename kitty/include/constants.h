#ifndef KITTY_CONSTANTS_H
#define KITTY_CONSTANTS_H
//
//  constants.h
//  for kitty module of psana
//
//  Created by Feldkamp on 9/15/11.
//  Copyright 2011 SLAC National Accelerator Laboratory. All rights reserved.
//

#include <string>

#include "psddl_psana/cspad.ddl.h"
// to work with detector data include corresponding 
// header from psddl_psana package
//#include "psddl_psana/acqiris.ddl.h"

#include "kitty/arrayclasses.h"

namespace kitty {
	//define CSPAD dimensions...
	const int nRowsPerASIC = Psana::CsPad::MaxRowsPerASIC;					// 194
	const int nColsPerASIC = Psana::CsPad::ColumnsPerASIC; 					// 185
	const int nRowsPer2x1 = nRowsPerASIC * 2;								// 388
	const int nColsPer2x1 = nColsPerASIC;									// 185
	const int nMax2x1sPerQuad = Psana::CsPad::SectorsPerQuad;				// 8
	const int nMaxQuads = Psana::CsPad::MaxQuadsPerSensor;					// 4
	const int nPxPer2x1 = nColsPer2x1 * nRowsPer2x1;						// 71780
	const int nMaxPxPerQuad = nPxPer2x1 * nMax2x1sPerQuad;					// 574240
	const int nMaxTotalPx = nMaxPxPerQuad * nMaxQuads; 						// 2296960  (2.3e6)
	
	const std::string IDSTRING_CSPAD_DATA = "CSPAD_DATA";
	const std::string IDSTRING_CSPAD_RUNAVGERAGE = "CSPAD_RUNAVGERAGE";
	const std::string IDSTRING_PX_X_um = "CSPAD_X_um";
	const std::string IDSTRING_PX_Y_um = "CSPAD_Y_um";
	const std::string IDSTRING_PX_X_int = "CSPAD_X_int";
	const std::string IDSTRING_PX_Y_int = "CSPAD_Y_int";
	const std::string IDSTRING_PX_X_pix = "CSPAD_X_pix";
	const std::string IDSTRING_PX_Y_pix = "CSPAD_Y_pix";
	const std::string IDSTRING_PX_X_q = "CSPAD_X_q";
	const std::string IDSTRING_PX_Y_q = "CSPAD_Y_q";
	const std::string IDSTRING_PX_TWOTHETA = "CSPAD_TWOTHETA";
	const std::string IDSTRING_PX_PHI = "CSPAD_PHI";
	const std::string IDSTRING_CUSTOM_EVENTNAME = "CUSTOM_EVENTNAME";
	const std::string IDSTRING_OUTPUT_PREFIX = "OUTPUT_PREFIX";
	
}//namespace


#endif
