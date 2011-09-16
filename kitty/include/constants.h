#ifndef KITTY_CONSTANTS_H
#define KITTY_CONSTANTS_H
//
//  constants.h
//  for kitty module of psana
//
//  Created by Feldkamp on 9/15/11.
//  Copyright 2011 SLAC National Accelerator Laboratory. All rights reserved.
//

#include "psddl_psana/cspad.ddl.h"
// to work with detector data include corresponding 
// header from psddl_psana package
//#include "psddl_psana/acqiris.ddl.h"

namespace kitty {
	//define CSPAD dimensions...
	const int nRowsPer2x1 = Psana::CsPad::MaxRowsPerASIC * 2;				// 388
	const int nColsPer2x1 = Psana::CsPad::ColumnsPerASIC;					// 185
	const int nMax2x1sPerQuad = Psana::CsPad::SectorsPerQuad;				// 8
	const int nMaxQuads = Psana::CsPad::MaxQuadsPerSensor;					// 4
	const int nPxPer2x1 = nColsPer2x1 * nRowsPer2x1;						// 71780
	const int nMaxPxPerQuad = nPxPer2x1 * nMax2x1sPerQuad;					// 574240
	const int nMaxTotalPx = nMaxPxPerQuad * nMaxQuads; 						// 2296960  (2.3e6)
}


#endif
