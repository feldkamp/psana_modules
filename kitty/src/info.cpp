//--------------------------------------------------------------------------
// File and Version Information:
// 	$Id$
//
// Description:
//	Class info...
//
// Author List:
//      Jan Moritz Feldkamp
//
//------------------------------------------------------------------------

//-----------------------
// This Class's Header --
//-----------------------
#include "kitty/info.h"

//-----------------
// C/C++ Headers --
//-----------------
#include <iomanip>
using std::setw;

#include <sstream>

#include <vector>
using std::vector;

#include <string>
using std::string;

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
PSANA_MODULE_FACTORY(info)

//		----------------------------------------
// 		-- Public Function Member Definitions --
//		----------------------------------------

namespace kitty {

//----------------
// Constructors --
//----------------
info::info (const std::string& name)
	: Module(name)
	, p_outputPrefix("")
	, p_count(0)
	, p_stopflag(false)
	, fout()
	, p_pvNames()
	, p_pvDesc()
{
	p_outputPrefix				= configStr("outputPrefix", 		"out");
}

//--------------
// Destructor --
//--------------
info::~info ()
{
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the beginning of the job
void 
info::beginJob(Event& evt, Env& env)
{
	MsgLog(name(), debug, "beginJob()" );
	
	p_pvNames[pvDetPos] = "CXI:DS1:MMS:06";
	p_pvDesc[pvDetPos] = "detector position";

	p_pvNames[pvLambda] = "SIOC:SYS0:ML00:AO192";
	p_pvDesc[pvLambda] = "wavelength [nm]";
		
	p_pvNames[pvElEnergy] = "BEND:DMP1:400:BDES";
	p_pvDesc[pvElEnergy] = "electron beam energy";
	
	p_pvNames[pvNElectrons] = "BPMS:DMP1:199:TMIT1H";
	p_pvDesc[pvNElectrons] = "particle N_electrons";

	p_pvNames[pvRepRate] = "EVNT:SYS0:1:LCLSBEAMRATE";
	p_pvDesc[pvRepRate] = "LCLS repetition rate [Hz]";
		
	p_pvNames[pvPeakCurrent] = "SIOC:SYS0:ML00:AO195";
	p_pvDesc[pvPeakCurrent] = "peak current after second bunch compressor";
	
	p_pvNames[pvPulseLength] = "SIOC:SYS0:ML00:AO820";
	p_pvDesc[pvPulseLength] = "pulse length";
	
	p_pvNames[pvEbeamLoss] = "SIOC:SYS0:ML00:AO569";
	p_pvDesc[pvEbeamLoss] = "ebeam energy loss converted to photon mJ";
	
	p_pvNames[pvNumPhotons] = "SIOC:SYS0:ML00:AO580";
	p_pvDesc[pvNumPhotons] = "calculated number of photons";
	
	p_pvNames[pvPhotonEnergy] = "SIOC:SYS0:ML00:AO541";
	p_pvDesc[pvPhotonEnergy] = "photon beam energy [eV]";


	ostringstream header;
	header << "run ";
	map<string,string>::iterator it;
	for(it = p_pvNames.begin(); it != p_pvNames.end(); it++){
		header << it->second << " ";
	}
	header << endl;
	header << "run ";
	for(it = p_pvDesc.begin(); it != p_pvDesc.end(); it++){
		header << it->second << " ";
	}
	header << endl;
			
	std::ofstream headerfile("header.txt");
	headerfile << header.str() << endl;
}




/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the run
void 
info::beginRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "beginRun()" );

	bool displayAllAvailablePVs = true;

	ostringstream runinfo;

	//get run rumber for this run
	shared_ptr<EventId> eventId = evt.get();
	int runNumber = 0;
    if (eventId.get()) {
		runNumber = eventId->run();
		MsgLog(name(), trace, name() << ": Using run number " << runNumber);
		runinfo << runNumber << " ";
    } else {
		MsgLog(name(), warning, name() << ": Cannot get eventId.");
    }	

	ostringstream fnosst;
	fnosst << p_outputPrefix << "_r" << runNumber << ".txt";
	fout.open(fnosst.str().c_str());
	if (fout.fail()){
		MsgLog(name(), error,  "could not open output file " << fnosst.str());
	}
	
	if (displayAllAvailablePVs){		
		const vector<string>& all_pvNames = env.epicsStore().pvNames();
		if (all_pvNames.size() == 0){
			MsgLog(name(), info, " --- no stored PVs --- " );
		}else{
			MsgLog(name(), info, "------list of stored PVs------" );
			for (unsigned int i = 0; i < all_pvNames.size(); i++){
				MsgLog(name(), info, all_pvNames.at(i) );
			}
		}
	}
	

	map<string,double> pvValue;						// vector for corresponding values
	map<string,bool> pvValid;						// keep track of the validity of this PV
	
	//try to get the PV values
	map<string,string>::iterator it;
	ostringstream osst;
	for (it = p_pvNames.begin(); it != p_pvNames.end(); it++){
		osst << setw(20) << it->first << " " << setw(26) << it->second; 
		try{
			pvValue[it->first] = env.epicsStore().value(it->second);
			pvValid[it->first] = true;
			osst << " = " << setw(12) << pvValue[it->first];
		}catch(...){
			pvValid[it->first] = false;
			osst << setw(12) << " ---- ";
		}
		osst << " (" << p_pvDesc[it->first] << ")" << endl;
	}
	
	MsgLog(name(), info, "------list of read out PVs------\n" << osst.str() );
	//use the values
	MsgLog(name(), info, "run no " << runNumber);
	for (it = p_pvNames.begin(); it != p_pvNames.end(); it++){
		runinfo << it->first << " ";
	}
	runinfo << endl;
	
	fout << runinfo.str();
	
	
//	MsgLog(name(), info, "------quantities derived from selected PVs------" );
//	unsigned int detZ_no = 0;
//	unsigned int lambda_no = 11;	
//	double detZ = 0; 
//	if (runPVvalid.at(detZ_no) ){
//		detZ = detOffset + runVals.at(detZ_no);
//	}
//	double lambda = 0;
//	if (runPVvalid.at(lambda_no) ){
//		lambda = runVals.at(lambda_no);
//	}
//	double two_k = 2*2*M_PI/lambda;								//in units of inv. nanometers
//	MsgLog(name(), info, "detZ = " << detZ << " mm");				
//	MsgLog(name(), info, "2*k = " << two_k << "nm^-1" );
//	MsgLog(name(), info, " ");
	
	std::list<EventKey> keys = evt.keys();
	MsgLog(name(), info, "------" << keys.size() << " keys in event------");
	for ( std::list<EventKey>::iterator it = keys.begin(); it != keys.end(); it++ ){
		ostringstream out;
		it->print(out);
		MsgLog(name(), info, out.str() << ", key: '" << it->key() << "'");
	}
	



	
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the beginning of the calibration cycle
void 
info::beginCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "beginCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called with event data, this is the only required 
/// method, all other methods are optional
void 
info::event(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "event()" );

	if (p_stopflag){
		stop();
		return;
	}

	map<string,double> pvValue;						// vector for corresponding values
	map<string,bool> pvValid;						// keep track of the validity of this PV
	
	//try to get the PV values
	map<string,string>::iterator it;
	ostringstream osst;
	for (it = p_pvNames.begin(); it != p_pvNames.end(); it++){
		osst << setw(20) << it->first << " " << setw(26) << it->second; 
		try{
			pvValue[it->first] = env.epicsStore().value(it->second);
			pvValid[it->first] = true;
			osst << " = " << setw(12) << pvValue[it->first];
		}catch(...){
			pvValid[it->first] = false;
			osst << setw(12) << " ---- ";
		}
		osst << " (" << p_pvDesc[it->first] << ")" << endl;
	}
	MsgLog(name(), info, "------list of read out PVs in event #" << p_count << "------\n" << osst.str() );
		
	//compile a list of PVs that need to be read out for each event 
	// (works, but commented out since it's not needed right now)
//	vector<string> eventPVs;						// vector for list of PVs
//	eventPVs.push_back("BEAM:LCLS:ELEC:Q");
//	
//	vector<double> eventVals;						// vector for corresponding values
//	eventVals.assign(eventPVs.size(), 0.);			// set all to zero
//	
//	for (unsigned int i = 0; i < eventPVs.size(); i++){
//		try{
//			eventVals.at(i) = env.epicsStore().value(eventPVs.at(i));
//			MsgLog(name(), info, eventPVs.at(i) << " = " << eventVals.at(i) );
//		}catch(...){
//			MsgLog(name(), warning, "PV " << eventPVs.at(i) << " doesn't exist." );
//		}
//	}

	//stop after a few events
	if (p_count >= 10){
		p_stopflag = true;
	}
		
	// increment event counter
	p_count++;
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the calibration cycle
void 
info::endCalibCycle(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "endCalibCycle()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called at the end of the run
void 
info::endRun(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "endRun()" );
}


/// ------------------------------------------------------------------------------------------------
/// Method which is called once at the end of the job
void 
info::endJob(Event& evt, Env& env)
{
	MsgLog(name(), debug,  "endJob()" );

	fout.close();
}



} // namespace kitty
