/*
 * 
 * G52OSC CW 
 * Task 4: RR with Bounded Buffer and Multiple Consumers
 */

#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include "coursework.h"

/* Create new names for existing structures. */
typedef struct process Process;
typedef struct timeval Timeval;

/* Create a new structure to store data instead of the global variables. */
struct pthreadDataTransfer
{
	Process **start;
	Process **end;
	long int responseTimeTotal;
	long int turnAroundTimeTotal;
	int jobCreated;
	int jobFinished;
	int exitingConsumer;
	int consumerID;
	int isNew[NUMBER_OF_PROCESSES];
};
typedef struct pthreadDataTransfer PthreadDataTransfer;

/* Declare semaphores and i. */
int i;
sem_t syncReadyQueue, fullReadyQueue, emptySystem, writeTimeMutex, consumerIDMutex, printMutex;

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

/* Function adds a process to the end of the ready queue. */
void addToReadyQueue(PthreadDataTransfer *dataTransfer, Process *gProcess)
{
	gProcess->iState = READY;
	addProcess(dataTransfer->start, dataTransfer->end, gProcess);
}

/* Function removes the first process from the ready queue. */
Process *removeFromReadyQueue(PthreadDataTransfer *dataTransfer)
{
	return removeProcess(dataTransfer->start, dataTransfer->end);
}

/* Function adds the response time with the Write Time Mutex. */
void addResponseTimeTotal(PthreadDataTransfer *dataTransfer, long int responseTime)
{
	sem_wait(&writeTimeMutex);
	dataTransfer->responseTimeTotal += responseTime;
	sem_post(&writeTimeMutex);
}

/* Function adds the turn around time with the Write Time Mutex. */
void addTurnAroundTimeTotal(PthreadDataTransfer *dataTransfer, long int turnAroundTime)
{
	sem_wait(&writeTimeMutex);
	dataTransfer->turnAroundTimeTotal += turnAroundTime;
	sem_post(&writeTimeMutex);
}

void runProcessRR(Process *executeProcess, PthreadDataTransfer *dataTransfer, int consumerID)
{
	/* Allocate memory to store time data. */
	Timeval *oStartTime = (Timeval *) malloc (sizeof(Timeval));
	Timeval *oEndTime = (Timeval *) malloc (sizeof(Timeval));
	int preBurstTime = executeProcess->iBurstTime;

	/* Simulate RR and get the response time and turn around time. */
	simulateRoundRobinProcess(executeProcess, oStartTime, oEndTime);
	long int responseTime = getDifferenceInMilliSeconds(executeProcess->oTimeCreated, *oStartTime);
	long int turnAroundTime = getDifferenceInMilliSeconds(executeProcess->oTimeCreated, *oEndTime);

	/* Use the Print Mutex avoid it to be interrupted. */
	sem_wait(&printMutex);

	/* Print the result of the simulation. */
	printf("Consumer Id = %d, Process Id = %d, Previous Burst Time = %d, New Burst Time = %d"
		, consumerID, executeProcess->iProcessId, preBurstTime, executeProcess->iBurstTime);

	/* isNew array is used to see if this is process's first response. */
	if (dataTransfer->isNew[executeProcess->iProcessId] == 1)
	{
		dataTransfer->isNew[executeProcess->iProcessId] = 0;
		printf(", Response Time = %ld", responseTime);
		addResponseTimeTotal(dataTransfer, responseTime);
	}

	/* Print the turn around time if process finished, else, add back to ready queue with semaphores. */
	if (executeProcess->iState == FINISHED)
	{
		printf(", Turn Around Time = %ld", turnAroundTime);
		addTurnAroundTimeTotal(dataTransfer, turnAroundTime);
		freePointer(executeProcess);
		(dataTransfer->jobFinished)++;
		sem_post(&emptySystem);
	}
	else if (executeProcess->iState == READY)
	{
		sem_wait(&syncReadyQueue);
		addToReadyQueue(dataTransfer, executeProcess);
		sem_post(&syncReadyQueue);
		sem_post(&fullReadyQueue);
	}

	printf("\n");
	sem_post(&printMutex);

	/* Free the spaces. */
	freePointer(oStartTime);
	freePointer(oEndTime);
}

void *consumer(void *a)
{
	PthreadDataTransfer *dataTransfer = (PthreadDataTransfer *) a;

	/* Assign the consumer ID with the Consumer ID Mutex. */
	sem_wait(&consumerIDMutex);
	int consumerIDtmp = (dataTransfer->consumerID)++;
	(dataTransfer->exitingConsumer)++;
	sem_post(&consumerIDMutex);

	while (1)
	{
		/* Leave the loop if the number of exiting consumer is large than unfinished jobs' number. */
		if (dataTransfer->exitingConsumer > NUMBER_OF_PROCESSES - dataTransfer->jobFinished)
		{
			sem_wait(&consumerIDMutex);
			(dataTransfer->exitingConsumer)--;
			sem_post(&consumerIDMutex);
			break;
		}

		/* Get the first process in the ready queue with semaphores. */
		sem_wait(&fullReadyQueue);
		sem_wait(&syncReadyQueue);
		Process *executeProcess = removeFromReadyQueue(dataTransfer);
		sem_post(&syncReadyQueue);

		/* Simulate run RR. */
		runProcessRR(executeProcess, dataTransfer, consumerIDtmp);
	}
}

void *producer(void *a)
{
	PthreadDataTransfer *dataTransfer = (PthreadDataTransfer *) a;
	while (1)
	{
		/* Leave the loop if all jobs have been created. */
		if (dataTransfer->jobCreated == NUMBER_OF_PROCESSES)
			break;

		/* Generate process. */
		Process *gProcess = generateProcess();

		/* Add the process to ready queue with semaphores. */
		sem_wait(&emptySystem);
		sem_wait(&syncReadyQueue);
		dataTransfer->isNew[gProcess->iProcessId] = 1;
		(dataTransfer->jobCreated)++;
		addToReadyQueue(dataTransfer, gProcess);
		sem_post(&syncReadyQueue);
		sem_post(&fullReadyQueue);
	}
}

/* Function with semaphores initialization. */
void semsInit()
{
	sem_init(&syncReadyQueue, 0, 1);
	sem_init(&fullReadyQueue, 0, 0);
	sem_init(&emptySystem, 0, BUFFER_SIZE);
	sem_init(&writeTimeMutex, 0, 1);
	sem_init(&consumerIDMutex, 0, 1);
	sem_init(&printMutex, 0, 1);
}

/* Function to destroy semaphores before exit. */
void semsDestroy()
{
	sem_destroy(&syncReadyQueue);
	sem_destroy(&fullReadyQueue);
	sem_destroy(&emptySystem);
	sem_destroy(&writeTimeMutex);
	sem_destroy(&consumerIDMutex);
	sem_destroy(&printMutex);
}

/* Function with data initialization. */
void dataInit(PthreadDataTransfer *dataTransfer)
{
	dataTransfer->responseTimeTotal = 0;
	dataTransfer->turnAroundTimeTotal = 0;
	dataTransfer->jobCreated = 0;
	dataTransfer->jobFinished = 0;
	dataTransfer->exitingConsumer = 0;
	dataTransfer->consumerID = 1;
}

int main(int argc, char *argv[])
{
	PthreadDataTransfer *dataTransfer = (PthreadDataTransfer *) malloc (sizeof(PthreadDataTransfer));
	Process *start = NULL;
	Process *end = NULL;
	dataTransfer->start = &start;
	dataTransfer->end = &end;
	dataInit(dataTransfer);
	semsInit();

	/* Declare, create and join threads. */
	pthread_t producer_thread, consumer_thread[NUMBER_OF_CONSUMERS];
	pthread_create(&producer_thread, NULL, producer, dataTransfer);
	for (i = 0; i < NUMBER_OF_CONSUMERS; i++)
		pthread_create(&consumer_thread[i], NULL, consumer, dataTransfer);

	pthread_join(producer_thread, NULL);
	for (i = 0; i < NUMBER_OF_CONSUMERS; i++)
		pthread_join(consumer_thread[i], NULL);

	/* Print the finial result. */
	printf("Average response time = %.6lf\n", (dataTransfer->responseTimeTotal) * 1.0 / NUMBER_OF_PROCESSES);
	printf("Average turn around time = %.6lf\n", (dataTransfer->turnAroundTimeTotal) * 1.0 / NUMBER_OF_PROCESSES);
	
	/* Free the spaces and destroy semaphores. */
	freePointer(dataTransfer);
	semsDestroy();
}