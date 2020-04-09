#include "AudioDriver.h"
#include "RelaJet.h"

static float g_fBuffer[BUF_SIZE];

void NR_process(int16_t *i16Buffer)
{
	for (int i=0; i < BUF_SIZE; i++) 
		g_fBuffer[i] = i16Buffer[i];

	relajet_nr_go(g_fBuffer);
	
	for (int i = 0; i < BUF_SIZE; ++i)
		i16Buffer[i] = (g_fBuffer[i]);
}
