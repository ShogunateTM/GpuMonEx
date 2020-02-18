#ifndef _FRAMERATE_FPS
#define _FRAMERATE_FPS

// Minimum number of seconds to integrate framerate over.
#define FPS_INTEGRATE_INTERVAL 1.0f

#include <sys/timeb.h>

class FrameRate {
private:
    struct timeb lastFPSClock;
    int calcFPS;
    float lastFPS;
    int lastFPSCount;
	inline float elapsed_ftime(timeb *tstart, timeb *tend)
		{
		return (float)(tend->time - tstart->time)
				+ ((float)(tend->millitm - tstart->millitm))/1000.0f;
		}
public:
	inline void  StartFPSClock() { calcFPS = 1; ftime(&lastFPSClock); lastFPSCount=0;}
	void         UpdateFPS();
	inline void  StopFPSClock() { calcFPS = 0; }
	inline float GetFPS() { return lastFPS; }
};

#endif

