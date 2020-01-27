/* 
 * 
 * G52OSC CW 
 * Task 1: Process scheduling
 * task1b: Implementation of round robin (RR)
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

/* Function inserts new process into the end of the existing list using the
 * addresses of the start pointer and the end pointer of this list.
 */
void addProcess(Process **start, Process **end, Process *newProcess)
{
	Process *endTmp = *end;
	if (*start == NULL && *end == NULL)
		*start = newProcess;
	else
		endTmp->oNext = newProcess;

	*end = newProcess;
}

/* Function removes the first process in the list of processes using the addresses of the
 * start pointer and the end pointer of this list. If list is empty, return NULL.
 */
Process *removeProcess(Process **start, Process **end)
{
	if (*start == NULL)
		return NULL;
	Process *startTmp = *start;
	*start = startTmp->oNext;
	if (startTmp->oNext == NULL)
		*end = NULL;
	Process *res = startTmp;
	res->oNext = NULL;
	return res;
}

int main(int argc, char *argv[]) 
{
	Process *start = NULL;
	Process *end = NULL;
	int i;

	/* Generate processes and append them to the end of the list. */
	for (i = 0; i < NUMBER_OF_PROCESSES; i++)
	{
		Process *gProcess = generateProcess();
		gProcess->iState = READY;
		addProcess(&start, &end, gProcess);
	}

	/* Allocate memory to store time data. */
	Timeval *oStartTime = (Timeval *) malloc (sizeof(Timeval));
	Timeval *oEndTime = (Timeval *) malloc (sizeof(Timeval));

	long int responseTimeTotal = 0, turnAroundTimeTotal = 0;

	/* Save maximum serviced process ID to see if it is a first response . */
	long int maxServicedProcessId = -1;

	/* Keep loop until the list is empty. */
	while (start != NULL)
	{
		/* Remove the first process from the list and save its previous burst time. */
		Process *executeProcess = removeProcess(&start, &end);
		int preBurstTime = executeProcess->iBurstTime;

		/* Simulate RR and save the response time and turn around time. */
		simulateRoundRobinProcess(executeProcess, oStartTime, oEndTime);
		long int responseTime = getDifferenceInMilliSeconds(executeProcess->oTimeCreated, *oStartTime);
		long int turnAroundTime = getDifferenceInMilliSeconds(executeProcess->oTimeCreated, *oEndTime);

		/* Print the result of the simulation. */
		printf("Process Id = %d, Previous Burst Time = %d, New Burst Time = %d"
			, executeProcess->iProcessId, preBurstTime, executeProcess->iBurstTime);

		/* Print the response time if this is process's first response. */
		if (executeProcess->iProcessId > maxServicedProcessId)
		{
			printf(", Response Time = %ld", responseTime);
			responseTimeTotal += responseTime;
			maxServicedProcessId = executeProcess->iProcessId;
		}

		/* Print the turn around time if process finished, else, add back to list. */
		if (executeProcess->iState == FINISHED)
		{
			printf(", Turn Around Time = %ld", turnAroundTime);
			turnAroundTimeTotal += turnAroundTime;
			freePointer(executeProcess);
		}
		else if (executeProcess->iState == READY)
			addProcess(&start, &end, executeProcess);

		printf("\n");
	}

	/* Print the finial result. */
	printf("Average response time = %.6lf\n", responseTimeTotal * 1.0 / NUMBER_OF_PROCESSES);
	printf("Average turn around time = %.6lf\n", turnAroundTimeTotal * 1.0 / NUMBER_OF_PROCESSES);

	/* Free the spaces. */
	freePointer(oStartTime);
	freePointer(oEndTime);
}