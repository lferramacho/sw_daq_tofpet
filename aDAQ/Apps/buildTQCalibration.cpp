#include <TFile.h>
#include <TNtuple.h>
#include <TChain.h>
#include <TProfile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TF1.h>
#include <TGraph.h>
#include <TOFPET/RawV2.hpp>
#include <TOFPET/RawV3.hpp>
#include <ENDOTOFPET/Raw.hpp>
#include <ENDOTOFPET/Extract.hpp>
#include <STICv3/sticv3Handler.hpp>
#include <TOFPET/P2Extract.hpp>
#include <TOFPET/P2.hpp>
#include <Common/Constants.hpp>
#include <Common/Utils.hpp>
#include <Core/CrystalPositions.hpp>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <getopt.h>

using namespace DAQ;
using namespace DAQ::Core;
using namespace DAQ::TOFPET;
using namespace DAQ::Common;
using namespace std;


static float		eventStep1;
static float		eventStep2;
static long long 	stepBegin;
static long long 	stepEnd;

static long long	eventTime;
static unsigned short	eventChannel;
static float		eventToT;
static float		eventTQT;
static float		eventTQE;

class TQCorrWriter : public EventSink<Hit>, public EventSource<Hit>{



public:
	TQCorrWriter(FILE *tQcalFile, bool writeBadEvents, P2 *lut, EventSink<Hit> *sink) 
		: EventSource<Hit>(sink), tQcalFile(tQcalFile), lut(lut), writeBadEvents(writeBadEvents){
		
		start_t = 1.;
		end_t = 3.;
		start_e = 1.;
		end_e = 3.;
		char hist_T[128];
		char hist_E[128];
		
		for(int channel=0; channel<SYSTEM_NCHANNELS; channel++){
			sprintf(hist_T,"htqT_ch%d",channel);   
			sprintf(hist_E,"htqE_ch%d",channel);   
			if(int(lut->nBins_tqT[channel]) > 0){
				htqT2D[channel] = new TH2D(hist_T, "tqt vs ToT",int(lut->nBins_tqT[channel]),1,3,6,0,300);
			}
			else{
				htqT2D[channel] = new TH2D(hist_T, "tqt vs ToT",300,1,3,6,0,300);
			}
			if(int(lut->nBins_tqE[channel]) > 0){
				htqE2D[channel] = new TH2D(hist_E, "tqE vs ToT",int(lut->nBins_tqE[channel]),1,3,6,0,300);
			}
			else{
				htqE2D[channel] = new TH2D(hist_E, "tqE vs ToT",300,1,3,6,0,300);
			}
		
		}
	};
	
	~TQCorrWriter() {
		for(int channel=0; channel<SYSTEM_NCHANNELS; channel++){
			delete htqT2D[channel];
			delete htqE2D[channel];
		}
		
		fclose(tQcalFile);
	};

	void pushEvents(EventBuffer<Hit> *buffer) {
		if(buffer == NULL) return;	
		
		unsigned nEvents = buffer->getSize();
		for(unsigned i = 0; i < nEvents; i++) {
			Hit &e = buffer->get(i);
			
			
			bool isBadEvent=e.badEvent;
			if(writeBadEvents==false && isBadEvent)continue;

			long long T = SYSTEM_PERIOD * 1E12;
			
			eventTime = e.time;
			eventChannel = e.raw->channelID;
			eventToT = 1E-3*(e.timeEnd - e.time);
			eventTQT = e.tofpet_TQT;
			eventTQE = e.tofpet_TQE;
			htqT2D[eventChannel]->Fill(eventTQT, eventToT);
			htqE2D[eventChannel]->Fill(eventTQE, eventToT);
		
		}
		
		sink->pushEvents(buffer);
	};
	
	void pushT0(double t0) { };
	void finish() { 
		for(int channel = 0; channel < SYSTEM_NCHANNELS ; channel++){ 
			for(int whichBranch = 0; whichBranch < 2; whichBranch++) {
				for(int tot_bin=0;tot_bin<6;tot_bin++){
					bool isT = (whichBranch == 0);
					
					//sprintf(args2, "tot > %s && tot < %s && channel == %s && step1==1 &&(fmod(time,6.4e6)<3500e3 || fmod(time,6.4e6)>3600e3)",totl_str, toth_str, ch_str);
					

					TH1D *htq = isT ? htqT2D[channel]->ProjectionX("htqT_proj",tot_bin+1,tot_bin+2) : htqE2D[channel]->ProjectionX("htqE_proj",tot_bin+1,tot_bin+2);
								
					Double_t C = isT ? (end_t-start_t)/htq->Integral() : (end_e-start_e)/htq->Integral() ;
					
					//printf("%5d\t%s\t%10.6e\t%10.6e\n",channel, isT ? "T" : "E", C, isT ? htq->Integral() : htq->Integral());

					cumul=0;
					
					int nbins= isT ? int(lut->nBins_tqT[channel]) : int(lut->nBins_tqE[channel]);
		    
					for(int bin = 1; bin < nbins+1; bin++) {
						if(bin != 1)cumul+= htq->GetBinContent(bin); 
					
						fprintf(tQcalFile, "%5d\t%c\t%d\t%d\t%10.6e\t%10.6e\n", channel, isT ? 'T' : 'E', tot_bin, bin-1,  htq->GetBinContent(bin), C*cumul); 
					}
					
				}
				//htq->reset();
			}
			
		}
	};
	void report() { };
private: 
	FILE *tQcalFile;
	P2 *lut;
	TH2D *htqT2D[SYSTEM_NCRYSTALS];
	TH2D *htqE2D[SYSTEM_NCRYSTALS];
	float start_t;
	float start_e;
	float end_t;
	float end_e;
	float cumul;
	bool writeBadEvents;

	

};

void displayHelp(char * program)
{
	fprintf(stderr, "usage: %s setup_file rawfiles_prefix output_file\n", program);
	fprintf(stderr, "\noptional arguments:\n");
	fprintf(stderr,  "  --help \t\t\t Show this help message and exit \n");
#ifndef __ENDOTOFPET__	
	fprintf(stderr,  "  --onlineMode\t Use this flag to process data in real time during acquisition\n");
	fprintf(stderr,  "  --acqDeltaTime=ACQDELTATIME\t If online mode is chosen, this variable defines how much data time (in seconds) to process (default is -1 which selects all data for the current step)\n");
	fprintf(stderr,  "  --raw_version=RAW_VERSION\t The version of the raw file to be processed: 2 or 3 (default) \n");
#endif
	fprintf(stderr, "\npositional arguments:\n");
	fprintf(stderr, "  setup_file \t\t\t File containing paths to tdc calibration file(s) (required), tQ correction file(s) (optional) and Energy calibration file(s) (optional)\n");
	fprintf(stderr, "  rawfiles_prefix \t\t Path to raw data files prefix\n");
	fprintf(stderr, "  output_file_prefix \t\t\t Output file containing tq calibration data (extension .tqcal will be created automatically)\n");
};

void displayUsage( char * program)
{
	fprintf(stderr, "usage: %s setup_file rawfiles_prefix output_file_prefix\n", program);
};

int main(int argc, char *argv[])
{


   	static struct option longOptions[] = {
		{ "help", no_argument, 0, 0 },
		{ "onlineMode", no_argument,0,0 },
		{ "acqDeltaTime", required_argument,0,0 },
		{ "raw_version", required_argument,0,0 }
	};

#ifndef __ENDOTOFPET__
	char rawV[128];
	rawV[0]='3';
	float readBackTime=-1;
#endif
	bool onlineMode=false;
	int nOptArgs=0;
	while(1) {
		int optionIndex = 0;
		int c=getopt_long(argc, argv, "",longOptions, &optionIndex);
		if(c==-1) break;
		
		if(optionIndex==0){
			displayHelp(argv[0]);
			return(1);
			
		}
#ifndef __ENDOTOFPET__	
		else if(optionIndex==1){
			nOptArgs++;
			onlineMode=true;
		}
		else if(optionIndex==2){
			nOptArgs++;
			readBackTime=atof(optarg);
		}
		else if(optionIndex==3){
			nOptArgs++;
			sprintf(rawV,optarg);
			if(rawV[0]!='2' && rawV[0]!='3'){
				fprintf(stderr, "\n%s: error: Raw version not valid! Please choose 2 or 3\n", argv[0]);
				return(1);
			}
		}	
#endif	
		else{
			displayUsage(argv[0]);
			fprintf(stderr, "\n%s: error: Unknown option!\n", argv[0]);
			return(1);
		}
	}
   
	if(argc - optind < 3){
		displayUsage(argv[0]);
		fprintf(stderr, "\n%s: error: too few positional arguments!\n", argv[0]);
		return(1);
	}
	else if(argc - optind > 3){
		displayUsage(argv[0]);
		fprintf(stderr, "\n%s: error: too many positional arguments!\n", argv[0]);
		return(1);
	}

	char * setupFileName=argv[optind];
	char *inputFilePrefix = argv[optind+1];
	char *outputFilePrefix = argv[optind+2];


	DAQ::TOFPET::RawScanner *scanner = NULL;
#ifndef __ENDOTOFPET__ 
	if(rawV[0]=='3')
		scanner = new DAQ::TOFPET::RawScannerV3(inputFilePrefix);
	else if(rawV[0]=='2')
		scanner = new DAQ::TOFPET::RawScannerV2(inputFilePrefix);
#else 
	scanner = new DAQ::ENDOTOFPET::RawScannerE(inputFilePrefix);
#endif

	TOFPET::P2 *P2 = new TOFPET::P2(SYSTEM_NCRYSTALS);
	if (strcmp(setupFileName, "none") == 0) {
		P2->setAll(2.0);
		printf("BIG FAT WARNING: no calibration\n");
	} 
	else {
		P2->loadFiles(setupFileName, false, false, 0, 0);
	}
	

	char filename_tq[256];
	FILE *f;
	stepBegin = 0;
	stepEnd = 0;
	int N = scanner->getNSteps();

	for(int step = 0; step < N; step++) {
		unsigned long long eventsBegin;
		unsigned long long eventsEnd;
		if(onlineMode)step=N-1;
		scanner->getStep(step, eventStep1, eventStep2, eventsBegin, eventsEnd);
		if(eventsBegin==eventsEnd)continue;
		if(!onlineMode)printf("Step %3d of %3d: %f %f (%llu to %llu)\n", step+1, scanner->getNSteps(), eventStep1, eventStep2, eventsBegin, eventsEnd);
		
       
		if(N==1){
			sprintf(filename_tq,"%s.tqcal",outputFilePrefix);
			printf(filename_tq);
		}
		else{
			sprintf(filename_tq,"%s_stp1_%f_stp2_%f.tqcal",outputFilePrefix,eventStep1, eventStep2); 
		}
		f = fopen(filename_tq, "w");

		DAQ::TOFPET::RawReader *reader=NULL;
#ifndef __ENDOTOFPET__	
		EventSink<RawHit> * pipeSink = 
				new P2Extract(P2, false, 0.0, 0.20, true,
				new TQCorrWriter(f, false, P2,
				new NullSink<Hit>()
        ));

		if(rawV[0]=='3') 
			reader = new DAQ::TOFPET::RawReaderV3(inputFilePrefix, SYSTEM_PERIOD,  eventsBegin, eventsEnd, readBackTime, onlineMode, pipeSink);
		else if(rawV[0]=='2')
		    reader = new DAQ::TOFPET::RawReaderV2(inputFilePrefix, SYSTEM_PERIOD,  eventsBegin, eventsEnd, pipeSink);
#else
		reader = new DAQ::ENDOTOFPET::RawReaderE(inputFilePrefix, SYSTEM_PERIOD,  eventsBegin, eventsEnd,
				new DAQ::ENDOTOFPET::Extract(new P2Extract(P2, false, 0.0, 0.20, true, NULL), new DAQ::STICv3::Sticv3Handler() , NULL,
				new TQCorrWriter(f, false, P2,
				new NullSink<Hit>()
				)));
#endif
	
		reader->wait();
		delete reader;
	
	}
	
	delete scanner;
	return 0;
	
}
