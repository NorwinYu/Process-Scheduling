/* 
 * 
 * G52OSC CW 
 * Task 1: Process scheduling
 * task1a: Implementation of shortest job first (SJF)
 */

#include <stdlib.h>
#include <sys/time.h>
#include "coursework.h"
#include <stdio.h>

/* Create new names for existing structures. */
typedef struct process Process;
typedef struct timeval Timeval;

/* Function frees the pointer and points it to NULL. */
void freePointer(void *pointer)
{
	free(pointer);
	pointer = NULL;
}

/* Function inserts new process into the existing list of processes in 
 * increasing order of burst time.
 */
void addProcessSortedList(Process **start, Process *newProcess)
{
	Process *tmp = *start;
	if (*start == NULL || tmp->iBurstTime >= newProcess->iBurstTime)
	{
		newProcess->oNext = tmp;
		*start = newProcess;
	}
	else
	{
		while (tmp->oNext != NULL && tmp->oNext->iBurstTime < newProcess->iBurstTime)
			tmp = tmp->oNext;
		newProcess->oNext = tmp->oNext;
		tmp->oNext = newProcess;
	}
}

/* Function removes the first process in the list of processes using 
 * the address of the start pointer of the list. If list is empty, return NULL.
 */
Process *removeProcess(Process **start)
{
	if (*start == NULL)
		return NULL;
	Process *startTmp = *start;
	*start = startTmp->oNext;
	Process *res = startTmp;
	res->oNext = NULL;
	return res;
}

int main(int argc, char *argv[]) 
{
	Process *start = NULL;
	int i;

	/* Generate processes and admit them to the Ready Queue with order of burst time. */
	for (i = 0; i < NUMBER_OF_PROCESSES; i++)
	{
		Process *gProcess = generateProcess();
		gProcess->iState = READY;
		addProcessSortedList(&start, gProcess);
	}

	/* Allocate memory to store time data. */
	Timeval *oStartTime = (Timeval *) malloc (sizeof(Timeval));
	Timeval *oEndTime = (Timeval *) malloc (sizeof(Timeval));

	long int responseTimeTotal = 0, turnAroundTimeTotal = 0;

	/* Keep loop until the list is empty. */
	while (start != NULL)
	{
		/* Remove the first process from the list and save its previous burst time. */
		Process *executeProcess = removeProcess(&start);
		int preBurstTime = executeProcess->iBurstTime;

		/* Simulate SJF and save the response time and turn around time. */
		simulateSJFProcess(executeProcess, oStartTime, oEndTime);
		long int responseTime = getDifferenceInMilliSeconds(executeProcess->oTimeCreated, *oStartTime);
		long int turnAroundTime = getDifferenceInMilliSeconds(executeProcess->oTimeCreated, *oEndTime);

		/* Print the result of the simulation. */
		printf("Process Id = %d, Previous Burst Time = %d, New Burst Time = %d, Response Time = %ld, Turn Around Time = %ld\n"
			, executeProcess->iProcessId, preBurstTime, executeProcess->iBurstTime, responseTime, turnAroundTime);

		responseTimeTotal += responseTime;
		turnAroundTimeTotal += turnAroundTime;

		/* Free the process after simulation. */
		freePointer(executeProcess);
	}

	/* Print the finial result. */
	printf("Average response time = %.6lf\n", responseTimeTotal * 1.0 / NUMBER_OF_PROCESSES);
	printf("Average turn around time = %.6lf\n", turnAroundTimeTotal * 1.0 / NUMBER_OF_PROCESSES);

	/* Free the spaces. */
	freePointer(oStartTime);
	freePointer(oEndTime);
}