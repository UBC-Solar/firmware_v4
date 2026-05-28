#ifndef __TASKS_H__
#define __TASKS_H__

void AppMain(void);

void StartHexDisplayTask(void *argument);
void StartSteeringOutputsTask(void *argument);

#endif /* __TASKS_H__ */