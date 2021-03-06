#include "CoincidenceFilter.hpp"
#include <Common/Constants.hpp>
#include <vector>
#include <assert.h>

using namespace std;
using namespace DAQ::Common;
using namespace DAQ::Core;

CoincidenceFilter::CoincidenceFilter(SystemInformation *systemInformation, float cWindow, float minToT,
	EventSink<RawHit> *sink)
	: OverlappedEventHandler<RawHit, RawHit>(sink), systemInformation(systemInformation), 
	cWindow((long long)(cWindow*1E12)), minToT((long long)(minToT*1E12))
{	
	nEventsIn = 0;
	nTriggersIn = 0;
	nEventsOut = 0;
}

CoincidenceFilter::~CoincidenceFilter()
{
}

void CoincidenceFilter::report()
{
	fprintf(stderr, ">> CoincidenceFilter report\n");
	fprintf(stderr, "  cWindow: %lld\n", cWindow);
	fprintf(stderr, "  min ToT: %lld\n", minToT);
	fprintf(stderr, "  %10u events received\n", nEventsIn);
	fprintf(stderr, "  %10u (%5.1f%%) events met minimum ToT \n", nTriggersIn, 100.0 * nTriggersIn / nEventsIn);
	fprintf(stderr, "  %10u (%5.1f%%) events passed\n", nEventsOut, 100.0 * nEventsOut / nEventsIn);
	OverlappedEventHandler<RawHit, RawHit>::report();
}

EventBuffer<RawHit> * CoincidenceFilter::handleEvents(EventBuffer<RawHit> *inBuffer)
{
	long long tMin = inBuffer->getTMin();
	long long tMax = inBuffer->getTMax();
	unsigned nEvents =  inBuffer->getSize();
	
	u_int32_t lEventsIn = 0;
	u_int32_t lTriggersIn = 0;
	u_int32_t lEventsOut = 0;
	
	vector<bool> meetsMinToT(nEvents, false);
	vector<bool> coincidenceMatched(nEvents, false);
	vector<short> region(nEvents, -1);

	for(unsigned i = 0; i < nEvents; i++) {
		RawHit &p1 = inBuffer->get(i);
		region[i] = systemInformation->getChannelInformation(p1.channelID).region;
	}

	for(unsigned i = 0; i < nEvents; i++) {
		RawHit &p1 = inBuffer->get(i);

		if((p1.timeEnd - p1.time) >= minToT) {
			meetsMinToT[i] = true;
		}
		else {
			meetsMinToT[i] = false;
			continue;
		}
		
		for (unsigned j = i+1; j < nEvents; j++) {
			RawHit &p2 = inBuffer->get(j);
			if((p2.time - p1.time) > (overlap + cWindow)) break;		// No point in looking further
			if((p2.timeEnd - p2.time) < minToT) continue;			// Does not meet min ToT
			if(tAbs(p2.time - p1.time) > cWindow) continue;			// Does not meet cWindow
			
			if(!systemInformation->isCoincidenceAllowed(region[i], region[j])) continue;

			coincidenceMatched[i] = true;
			coincidenceMatched[j] = true;
		}
	}
	
	vector<bool> accepted(nEvents, false);	
	const long long dWindow = 100000; // 100 ns acceptance window for events which come after the first
	for(unsigned i = 0; i < nEvents; i++) {
		RawHit &p1 = inBuffer->get(i);

		if(coincidenceMatched[i]) {
			accepted[i] = true;
		}
		else {
			continue;
		}

		for(unsigned j = i; j > 0; j--) {	// Look for events before p1
			RawHit &p2 = inBuffer->get(j);
			if(!systemInformation->isMultihitAllowed(region[i], region[j])) continue;
			if(p2.time < (p1.time - cWindow - overlap)) break;	// No point in looking further
			if(p2.time < (p1.time - cWindow)) continue;		// Doesn't meet cWindow
			accepted[j] = true;
		}
		
		for (unsigned j = i; j < nEvents; j++) {// Look for events after p1
			RawHit &p2 = inBuffer->get(j);
			if(!systemInformation->isMultihitAllowed(region[i], region[j])) continue;
			if(p2.time > (p1.time + dWindow + overlap)) break;	// No point in looking further
			if(p2.time > (p1.time + dWindow)) continue;		// Doesn't meet dWindow
			accepted[j] = true;
		}
	}
	
	// Filter unaccepted events by setting their time to -1
	for(unsigned i = 0; i < nEvents; i++) {
		RawHit &p1 = inBuffer->get(i);
		if(p1.time < tMin || p1.time >= tMax) {
			p1.time = -1;
			continue;
		}
		lEventsIn += 1;
		
		if(meetsMinToT[i]) lTriggersIn += 1;
		
		if (!accepted[i]) {
			p1.time = -1;
			continue;
		}
		lEventsOut += 1;
	}
	
	atomicAdd(nEventsIn, lEventsIn);
	atomicAdd(nTriggersIn, lTriggersIn);
	atomicAdd(nEventsOut, lEventsOut);
	return inBuffer;
}
