#ifndef AUDIO_TASK_H
#define AUDIO_TASK_H

#include "FreeRTOS.h"
#include "task.h"

//*****************************************************************************
//
// Audio task handle.
//
//*****************************************************************************
extern TaskHandle_t audio_task_handle;

//*****************************************************************************
//
// External function definitions.
//
//*****************************************************************************
extern void AudioTask(void *pvParameters);

#endif // AUDIO_TASK_H

