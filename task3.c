/*
 * 
 * G52OSC CW 
 * Task 3: SJF with Bounded Buffer and Multiple Consumers
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
	long int responseTimeTotal;
	long int turnAroundTimeTotal;
	int jobCreated;
	int jobInQueue;
	int consumerID;
};
typedef struct pthreadDataTransfer PthreadDataTransfer;

/* Declare semaphores and i. */
int i;
sem_t syncReadyQueue, fullReadyQueue, emptySystem, writeTimeMutex, consumerIDMutex;

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
	Process *dataTransfermp = *start;
	*start = dataTransfermp->oNext;
	Process *res = dataTransfermp;
	res->oNext = NULL;
	return res;
}

/* Function adds a process into the ready queue with order of burst time. */
void addToReadyQueue(PthreadDataTransfer *dataTransfer, Process *addedProcess)
{
	(dataTransfer->jobInQueue)++;
	addedProcess->iState = READY;
	addProcessSortedList(dataTransfer->start, addedProcess);
}

/* Function removes the first process from the ready queue. */
Process *removeFromReadyQueue(PthreadDataTransfer *dataTransfer)
{
	(dataTransfer->jobInQueue)--;
	return removeProcess(dataTransfer->start);
}

void runProcessSJF(Process *executeProcess, PthreadDataTransfer *dataTransfer, int consumerID)
{
	/* Allocate memory to store time data. */
	Timeval *oStartTime = (Timeval *) malloc (sizeof(Timeval));
	Timeval *oEndTime = (Timeval *) malloc (sizeof(Timeval));
	int preBurstTime = executeProcess->iBurstTime;

	/* Simulate SJF and save the response time and turn around time. */
	simulateSJFProcess(executeProcess, oStartTime, oEndTime);
	long int responseTime = getDifferenceInMilliSeconds(executeProcess->oTimeCreated, *oStartTime);
	long int turnAroundTime = getDifferenceInMilliSeconds(executeProcess->oTimeCreated, *oEndTime);
	
	/* Print the result of the simulation. */
	printf("Consumer Id = %d, Process Id = %d, Previous Burst Time = %d, New Burst Time = %d, Response Time = %ld, Turn Around Time = %ld\n"
		, consumerID, executeProcess->iProcessId, preBurstTime, executeProcess->iBurstTime, responseTime, turnAroundTime);
	
	/* Save the total number of time with the Write Time Mutex. */
	sem_wait(&writeTimeMutex);
	dataTransfer->responseTimeTotal += responseTime;
	dataTransfer->turnAroundTimeTotal += turnAroundTime;
	sem_post(&writeTimeMutex);

	/* Free the spaces. */
	freePointer(executeProcess);
	freePointer(oStartTime);
	freePointer(oEndTime);
}

void *consumer(void *a)
{
	PthreadDataTransfer *dataTransfer = (PthreadDataTransfer *) a;

	/* Assign the consumer ID with the Consumer ID Mutex. */
	sem_wait(&consumerIDMutex);
	int consumerIDtmp = (dataTransfer->consumerID)++;
	sem_post(&consumerIDMutex);

	while (1)
	{
		/* Leave the loop if all jobs have been created and there is no job in the queue. */
		if (dataTransfer->jobCreated == NUMBER_OF_PROCESSES && dataTransfer->jobInQueue == 0)
			break;

		/* Get the first process in the ready queue with semaphores. */
		sem_wait(&fullReadyQueue);
		sem_wait(&syncReadyQueue);
		Process *executeProcess = removeFromReadyQueue(dataTransfer);
		sem_post(&syncReadyQueue);
		sem_post(&emptySystem);

		/* Simulate run SJF. */
		runProcessSJF(executeProcess, dataTransfer, consumerIDtmp);
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
}

/* Function to destroy semaphores before exit. */
void semsDestroy()
{
	sem_destroy(&syncReadyQueue);
	sem_destroy(&fullReadyQueue);
	sem_destroy(&emptySystem);
	sem_destroy(&writeTimeMutex);
	sem_destroy(&consumerIDMutex);
}

/* Function with data initialization. */
void dataInit(PthreadDataTransfer *dataTransfer)
{
	dataTransfer->responseTimeTotal = 0;
	dataTransfer->turnAroundTimeTotal = 0;
	dataTransfer->jobCreated = 0;
	dataTransfer->jobInQueue = 0;
	dataTransfer->consumerID = 1;
}

int main(int argc, char *argv[])
{
	PthreadDataTransfer *dataTransfer = (PthreadDataTransfer *) malloc (sizeof(PthreadDataTransfer));
	Process *start = NULL;
	dataTransfer->start = &start;
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
	free(dataTransfer);
	semsDestroy();
}