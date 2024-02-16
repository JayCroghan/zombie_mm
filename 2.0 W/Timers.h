#include "ZombiePlugin.h"
extern ISmmAPI *g_SMAPI;

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#define sleep(num) Sleep(num*1000)
	#define thread_t HANDLE
	#define msleep Sleep
#else
	#define msleep(num) usleep(num*1000)
	#define thread_t pthread_t
	#define __stdcall 
	#define __cdecl
#endif

struct timerInfo
{
	unsigned int id;
	void (*func)(void**);
	float time;
	float started;
	void **params;
	unsigned int paramCount;
	int *tofree;
};



class STimers
{
private:
    unsigned int m_lastTimerId;    
    float m_lastTimerTime;
	float m_fLastTickedTime;
	double g_fUniversalTime;
	double g_fTimerThink;
	bool m_bHasMapTickedYet;
    CUtlVector<timerInfo*> m_timerList;
    bool m_inUse;
    void CheckUse() {
        while (m_inUse) {
            msleep(1);
        }
    }
    void ExecuteTimer(int id) {
        if ((id < 0) || (id >= m_timerList.Count()))
            return;

        timerInfo* timer = m_timerList[id];
        m_timerList.Remove(id);
        if (!timer)
            return;

        if (timer->func && ((timer->paramCount < 1) || timer->params)) {
            (*(timer->func))(timer->params);
        }
        if (timer) {
            DeleteTimerInternal(timer);
        }
    }
    bool DeleteTimerInternal(timerInfo *timer) {
        if (!timer)
                return false;
        if ((timer->paramCount > 0) && timer->params) {
                if (timer->tofree) {
                            for (unsigned int q = 0; q < timer->paramCount; q++) {
                                    if (timer->tofree[q] != 0) {
                                            free(timer->params[q]);
                                    }
                            }
                            free(timer->tofree);
                    }
            free(timer->params);
        }
        free(timer);
        return true;
    }
    bool RemoveTimerInternal(unsigned int timerId) {
        for (int x = 0; x < m_timerList.Count(); x++) {
            if (m_timerList[x] && m_timerList[x]->id == timerId) {
                timerInfo *timer = m_timerList[x];
                m_timerList.Remove(x);
                DeleteTimerInternal(timer);
                return true;
            }
        }
        return false;
    }
public:
    STimers()
	{
        m_inUse = false;
        Reset();
		g_fUniversalTime = 0.0f;
		g_fTimerThink = 0.0f;
		m_bHasMapTickedYet = false;
		m_fLastTickedTime = 0.0f;
    }
    ~STimers() { }

	double TheTime()
	{
		return g_fUniversalTime;
	}

    void Reset()
	{
		m_bHasMapTickedYet = false;
        //CheckUse();
        m_inUse = true;
        for(int x = m_timerList.Count()-1; x >= 0; x--) {
            RemoveTimerInternal(x);
        }
        m_timerList.RemoveAll();
        m_lastTimerTime = g_SMAPI->pGlobals()->realtime;
        m_lastTimerId = 0;
        m_inUse = false;
    }

	double CalcNextThink(double last, float interval)
	{
		if (g_fUniversalTime - last - interval <= 0.1)
		{
			return last + interval;
		}
		else
		{
			return g_fUniversalTime + interval;
		}
	}


    void CheckTimers(bool simulating)
	{
		if (simulating && m_bHasMapTickedYet)
		{
			g_fUniversalTime += g_SMAPI->pGlobals()->curtime - m_fLastTickedTime;
		}
		else 
		{
			g_fUniversalTime += g_SMAPI->pGlobals()->interval_per_tick;
		}

		m_fLastTickedTime = g_SMAPI->pGlobals()->curtime;
		m_bHasMapTickedYet = true;

		if (g_fUniversalTime >= g_fTimerThink)
		{
			for (int x = 0; x < m_timerList.Count(); x++)
			{
                //if ((curtime - m_timerList[x]->started) >= m_timerList[x]->time) {
				if ( ( g_fUniversalTime - m_timerList[x]->started) >= m_timerList[x]->time )
				{
					ExecuteTimer(x);
					x--;
                }
			}

			g_fTimerThink = CalcNextThink(g_fTimerThink, 0.1);
		}

        //CheckUse();
        //m_inUse = true;
        //float curtime = g_SMAPI->pGlobals()->realtime;
		/*float m_fLastTickedTime = gpGlobals->curtime;
        if ((curtime - m_lastTimerTime) < 0.1) {
                m_inUse = false;
                return;
        }*/
        //m_lastTimerTime = curtime;
    }
    unsigned int AddTimer(float time, void (*func)(void**), void **params = NULL, unsigned int paramCount = 0, unsigned int timerId = 0, int *tofree = NULL)
	{
        m_inUse = true;
        if ( params && ( paramCount == 0 ) )
		{
            return 0;
		}
        timerInfo* thetimer = (timerInfo*)calloc(1, sizeof(timerInfo));
        thetimer->started = g_fUniversalTime;
        thetimer->time = max(0.1, time);
        thetimer->paramCount = paramCount;
        if (paramCount > 0)
		{
            thetimer->params = (void**)malloc(sizeof(void*) * paramCount);
            if ( tofree )
			{
                    thetimer->tofree = (int*)malloc(sizeof(int) * paramCount);
			}
            for (unsigned int x = 0; x < paramCount; x++)
			{
                thetimer->params[x] = params[x];
                if (tofree)
				{
                        thetimer->tofree[x] = tofree[x];
				}
            }
        }
		else 
		{
            thetimer->params = NULL;
            thetimer->tofree = NULL;
        }
        thetimer->func = func;
        thetimer->id = 0;
        if ( !timerId )
		{
            m_lastTimerId++;
            timerId = m_lastTimerId;
        }
        thetimer->id = timerId;
        
        RemoveTimerInternal(timerId);
        
        m_timerList.AddToTail(thetimer);
        m_inUse = false;
        return timerId;
    }
    bool RunTimer(unsigned int timerId)
	{
        m_inUse = true;
        for (int x = 0; x < m_timerList.Count(); x++) 
		{
                if (m_timerList[x]->id == timerId)
				{
                        ExecuteTimer(x);
                        m_inUse = false;
                        return true;
                }
        }
        m_inUse = false;
        return false;
    }
    bool RemoveTimer(unsigned int timerId) {
        //CheckUse();
        bool ret;
        m_inUse = true;
        ret = RemoveTimerInternal(timerId);
        m_inUse = false;
        return ret;
    }
	bool IsTimer(unsigned int timerId)
	{
		for (int x = 0; x < m_timerList.Count(); x++)
		{
            if (m_timerList[x] && m_timerList[x]->id == timerId)
			{
                return true;
            }
        }
        return false;
    }
	double StartTimeLeft( unsigned int timerId )
	{
		for (int x = 0; x < m_timerList.Count(); x++)
		{
            if (m_timerList[x] && m_timerList[x]->id == timerId)
			{
                return (double)( m_timerList[x]->time  - ( g_fUniversalTime - m_timerList[x]->started ) );
            }
        }
        return -1.0;
	}
};
