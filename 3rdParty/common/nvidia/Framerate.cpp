#include "Framerate.h"

void FrameRate::UpdateFPS()
{
	if ( calcFPS )
		{
		lastFPSCount++;
		struct timeb newClock;
		ftime( &newClock );
		float tdiff = elapsed_ftime( &lastFPSClock, &newClock );
		if ( tdiff >= FPS_INTEGRATE_INTERVAL )
			{
			lastFPS = (float)(lastFPSCount)/tdiff;
			lastFPSClock.time = newClock.time;
			lastFPSClock.millitm = newClock.millitm;
			lastFPSCount = 0;
			}
		}
}
