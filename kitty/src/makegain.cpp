//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class makegain...
//
// Author List:
//      Jan Moritz Feldkamp
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "kitty/makegain.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <cmath>

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "PSEvt/EventId.h"

#include "kitty/constants.h"
#include "kitty/util.h"
using ns_cspad_util::create1DFromRawImageCSPAD;
using ns_cspad_util::createAssembledImageCSPAD;
using ns_cspad_util::createRawImageCSPAD;
#include "kitty/crosscorrelator.h"


//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

// This declares this class as psana module
using namespace kitty;
PSANA_MODULE_FACTORY(makegain)

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace kitty {

//----------------
// Constructors --
//----------------
makegain::makegain (const std::string& name)
	: Module(name)
	, p_model_fn("")
	, p_modelDelta(0.)
	, p_outputPrefix("")
	, io(0)
	, p_model(0)
	, p_pixX_q_sp()
	, p_pixY_q_sp()
	, p_pixX_int_sp()
	, p_pixY_int_sp()
	, p_sum_sp()
	, p_count(0)
{
	p_model_fn				= configStr("model", 					"");
	p_modelDelta			= config   ("modelDelta",				1.);
		
	io = new arraydataIO();
	p_sum_sp = shared_ptr<array1D<double> >( new array1D<double>(nMaxTotalPx) );
}

//--------------
// Destructor --
//--------------
makegain::~makegain ()
{
	delete io;
	delete p_model;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
makegain::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "beginJob()" );
	MsgLog(name(), info, "model file = '" << p_model_fn << "'" );
	MsgLog(name(), info, "model delta = '" << p_modelDelta << "'" );
	
	if (p_model_fn != ""){
		int fail = io->readFromASCII( p_model_fn, p_model );
		if (fail){
			MsgLog(name(), error, "Could not read model. Aborting...");
			return;
		}else{
			MsgLog(name(), info, "model read. min:" << p_model->calcMin() << ", max:" << p_model->calcMax() );
		}
	}else{
			MsgLog(name(), warning, "No model file specified in config file. Aborting...");
			return;
	}
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
makegain::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug, "beginRun()" );

	//get calibration information	
	p_pixX_q_sp = evt.get(IDSTRING_PX_X_q);
	p_pixY_q_sp = evt.get(IDSTRING_PX_Y_q);
	p_pixX_int_sp = evt.get(IDSTRING_PX_X_int);
	p_pixY_int_sp = evt.get(IDSTRING_PX_Y_int);
	
	p_outputPrefix = *( (shared_ptr<std::string>) evt.get(IDSTRING_OUTPUT_PREFIX) ).get();
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the calibration cycle
void 
makegain::beginCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug, "beginCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
makegain::event(Event& evt, Env& env)
{
	MsgLog(name(), debug, "event()" );
	shared_ptr<array1D<double> > data_sp = evt.get(IDSTRING_CSPAD_DATA);
	
	if (data_sp){
		p_sum_sp->addArrayElementwise( data_sp.get() );	
		p_count++;
	}else{
		MsgLog(name(), warning, "could not get CSPAD data" );
	}
}
  
  
/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the calibration cycle
void 
makegain::endCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug, "endCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the run
void 
makegain::endRun(Event& evt, Env& env)
{
	MsgLog(name(), debug, "endRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
makegain::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "endJob()" );

	//create average out of raw sum
	p_sum_sp->divideByValue( p_count );
	
	//find out maximum of angular average (by using that function from cross correlator)
	int nPhi = 1024;
	int nQ1 = 500;
	int startQ = 0;
	int stopQ = 1000;
	
	CrossCorrelator *cc = new CrossCorrelator( p_sum_sp.get(), p_pixX_int_sp.get(), p_pixY_int_sp.get(), nPhi, nQ1 );
	cc->calculatePolarCoordinates(startQ, stopQ);
	
	array1D<double> *SAXS = 0;
	cc->polar()->calcAvgCol( SAXS );
	
	io->writeToHDF5( p_outputPrefix+"_saxs_1D.h5", SAXS );
	io->writeToHDF5( p_outputPrefix+"_saxs_2D.h5", cc->polar() );
	double datamax = SAXS->calcMax();
	
	//scale model to the intensity found in the scattering data
	double modelmax = p_model->calcMax();
	double scaling = datamax/modelmax;
	MsgLog(name(), info,  "Calculated model scaling factor: " << scaling << " = datamax(" << datamax << ") / modelmax(" << modelmax << ")");
	if (scaling != INFINITY && scaling != -INFINITY && scaling != 0){
		p_model->multiplyByValue(scaling);
	}else{
		MsgLog(name(), error, "Invalid model scaling value " << scaling << ". Setting scaling to 1." );
		//don't scale model in this case
	}
	io->writeToHDF5( p_outputPrefix+"_model_1D.h5", p_model );
	
	array1D<double> *gain = new array1D<double>(nMaxTotalPx);
	array1D<double> *expected = new array1D<double>(nMaxTotalPx);
	
	//calculate the expected signal for each q-value
	//and create a gainmap from the discrepancy
	for (unsigned int i=0; i < p_sum_sp->size(); i++){
		//find absolute q-value for this data point
		double qx = p_pixX_q_sp->get(i);
		double qy = p_pixY_q_sp->get(i);
		double q_abs = sqrt( qx*qx + qy*qy );
		//compare to model
		int q_model_index = (int) floor( q_abs/p_modelDelta ); 
		double correction = p_sum_sp->get(i) / p_model->get(q_model_index);
		gain->set(i, correction);
		
		expected->set(i, p_model->get(q_model_index));
		if (i > 10000 && i < 10010){
			cout << "example: i=" << i << ", qx=" << qx << ", qy=" << qy << ", |q|=" << q_abs 
				<< ", avg_sp(i)=" << p_sum_sp->get(i) << endl;
			cout << "   model index=" << q_model_index << ", p_model->get(index)=" << p_model->get(q_model_index) 
				<< ", correction=" << correction << endl;
		}
	}
	
	
	//----------------------------model output
	array2D<double> *model_raw2D = 0;
	createRawImageCSPAD( expected, model_raw2D );
	io->writeToHDF5( p_outputPrefix+"_model_raw2D.h5", model_raw2D );

	//assemble ASICs
	array2D<double> *model_asm2D = 0;
	createAssembledImageCSPAD( expected, p_pixX_int_sp.get(), p_pixY_int_sp.get(), model_asm2D );	
	io->writeToHDF5( p_outputPrefix+"_model_asm2D.h5", model_asm2D );	
	
	//----------------------------gain output
	array2D<double> *gain_raw2D = 0;
	createRawImageCSPAD( gain, gain_raw2D );
	io->writeToHDF5( p_outputPrefix+"_gain_raw2D.h5", gain_raw2D );

	//assemble ASICs
	array2D<double> *gain_asm2D = 0;
	createAssembledImageCSPAD( gain, p_pixX_int_sp.get(), p_pixY_int_sp.get(), gain_asm2D );
	io->writeToHDF5( p_outputPrefix+"_gain_asm2D.h5", gain_asm2D );

	delete model_raw2D;
	delete model_asm2D;
	delete gain_raw2D;
	delete gain_asm2D;
	delete cc;	
	delete gain;
	delete expected;
}



} // namespace kitty
