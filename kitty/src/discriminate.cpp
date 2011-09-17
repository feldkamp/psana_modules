//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class discriminate...
//
// Author List:
//      Jan Moritz Feldkamp
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "kitty/discriminate.h"

//-----------------
// C/C++ Headers --
//-----------------

//-------------------------------
// Collaborating Class Headers --
//-------------------------------
#include "MsgLogger/MsgLogger.h"
#include "PSEvt/EventId.h"

#include "kitty/constants.h"



//-----------------------------------------------------------------------
// Local Macros, Typedefs, Structures, Unions and Forward Declarations --
//-----------------------------------------------------------------------

// This declares this class as psana module
using namespace kitty;
PSANA_MODULE_FACTORY(discriminate)

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace kitty {

//----------------
// Constructors --
//----------------
discriminate::discriminate (const std::string& name)
	: Module(name)
	, m_src()
	, m_maxEvents()
	, m_filter()
	, m_count(0)
{

	// get the values from configuration or use defaults
	m_source        = configStr("source",        "CxiDs1.0:Cspad.0");
	m_runNumber     = config   ("runNumber",     0);
	m_maxEvents     = config   ("events",        10000000);
	m_filter        = config   ("filter",        false);
	m_src           = m_source;
	
	p_threshold 	= config("threshold", 0);
	
	p_skipcount = 0;
	p_hitcount = 0;
}

//--------------
// Destructor --
//--------------
discriminate::~discriminate ()
{
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
discriminate::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "discriminate::beginJob()" );
	MsgLog(name(), info, "max events = " << m_maxEvents );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
discriminate::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "discriminate::beginRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the calibration cycle
void 
discriminate::beginCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "discriminate::beginCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
discriminate::event(Event& evt, Env& env)
{
	ostringstream evtinfo;
	evtinfo <<  "# " << m_count << " of " << m_maxEvents << ". ";
	
	double datasum = 0.;
	int pxsum = 0;
	
	//get CSPAD data from evt object (out of XTC stream)
	shared_ptr<Psana::CsPad::DataV2> data = evt.get(m_src, "", &m_actualSrc);

	//put data into array1D object, which can then be passed to the following modules
	shared_ptr<array1D> raw1D_sp ( new array1D(nMaxTotalPx) );
	array1D* raw1D = raw1D_sp.get();
	
	if (data.get()){
		int nQuads = data->quads_shape().at(0);
		for (int q = 0; q < nQuads; ++q) {
			const Psana::CsPad::ElementV2& el = data->quads(q);
			
			const uint16_t* quad_data = el.data();
			int quad_count = el.quad();				//quadrant number
			std::vector<int> quad_shape = el.data_shape();
			int actual2x1sPerQuad = quad_shape.at(0);
			int actualPxPerQuad = actual2x1sPerQuad * nPxPer2x1;
			
			//go through the whole data of the quad, copy to array1D
			for (int i = 0; i < actualPxPerQuad; i++){
				datasum += quad_data[i];
				raw1D->set( q*nMaxPxPerQuad +i, (double)quad_data[i] );
			}
			
			pxsum += actualPxPerQuad;
			MsgLog(name(), debug, "quad " << quad_count << " of " << nQuads 
				<< " has " << actual2x1sPerQuad << " 2x1s, " << actualPxPerQuad << " pixels");
		}//for all quads
	}else{
		MsgLog(name(), warning, "could not get CSPAD data" );
	}
	
	double avgPerPix = datasum/(double)pxsum;
	evtinfo << "pixelavg= " << avgPerPix << ", avg diff to threshold=" << avgPerPix-p_threshold << " ";
	
	if ( avgPerPix <= p_threshold ){
		evtinfo << " --> SKIP";
		p_skipcount++;

		// all downstream modules will not be called
		skip();
		
	}else{
		evtinfo << " --> HIT";
		p_hitcount++;
		
		// add the data to the event so that other modules can access it
		evt.put(raw1D_sp);
	}
	
	// gracefully stop analysis job
	if (m_count >= m_maxEvents){
		MsgLog(name(), info, "Stopping analisys job after number of max events (" << m_maxEvents << ") has been reached");
		stop();
	}
	
	// increment event counter
	m_count++;
	
	// output collected information...
	MsgLog(name(), info, evtinfo.str());
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the calibration cycle
void 
discriminate::endCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "discriminate::endCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the run
void 
discriminate::endRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "discriminate::endRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
discriminate::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "discriminate::endJob()" );

	MsgLog(name(), info, "processed events: " << m_count 
		<< ", hits: " << p_hitcount 
		<< ", skipped: " << p_skipcount 
		<< ", hitrate: " << p_hitcount/(double)m_count * 100 << "%." );
}








} // namespace kitty
