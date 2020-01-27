/* 
 * 
 * G52OSC CW 
 * Task 2: SJF with Bounded Buffer
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
};
typedef struct pthreadDataTransfer PthreadDataTransfer;

/* Declare semaphores. */
sem_t syncReadyQueue, fullReadyQueue, emptySystem;

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
void addToReadyQueue(PthreadDataTransfer *dataTransfer, Process *gProcess)
{
	(dataTransfer->jobInQueue)++;
	gProcess->iState = READY;
	addProcessSortedList(dataTransfer->start, gProcess);
}

/* Function removes the first process from the ready queue. */
Process *removeFromReadyQueue(PthreadDataTransfer *dataTransfer)
{
	(dataTransfer->jobInQueue)--;
	return removeProcess(dataTransfer->start);
}

void runProcessSJF(Process *executeProcess, PthreadDataTransfer *dataTransfer)
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
	printf("Process Id = %d, Previous Burst Time = %d, New Burst Time = %d, Response Time = %ld, Turn Around Time = %ld\n"
		, executeProcess->iProcessId, preBurstTime, executeProcess->iBurstTime, responseTime, turnAroundTime);
	
	dataTransfer->responseTimeTotal += responseTime;
	dataTransfer->turnAroundTimeTotal += turnAroundTime;

	/* Free the spaces. */
	freePointer(executeProcess);
	freePointer(oStartTime);
	freePointer(oEndTime);
}

void *consumer(void *a)
{
	PthreadDataTransfer *dataTransfer = (PthreadDataTransfer *) a;
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
		runProcessSJF(executeProcess, dataTransfer);
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
}

/* Function to destroy semaphores before exit. */
void semsDestroy()
{
	sem_destroy(&syncReadyQueue);
	sem_destroy(&fullReadyQueue);
	sem_destroy(&emptySystem);
}

/* Function with data initialization. */
void dataInit(PthreadDataTransfer *dataTransfer)
{
	dataTransfer->responseTimeTotal = 0;
	dataTransfer->turnAroundTimeTotal = 0;
	dataTransfer->jobCreated = 0;
	dataTransfer->jobInQueue = 0;
}

int main(int argc, char *argv[])
{
	PthreadDataTransfer *dataTransfer = (PthreadDataTransfer *) malloc (sizeof(PthreadDataTransfer));
	Process *start = NULL;
	dataTransfer->start = &start;
	dataInit(dataTransfer);
	semsInit();

	/* Declare, create and join threads. */
	pthread_t producer_thread, consumer_thread;
	pthread_create(&producer_thread, NULL, producer, dataTransfer);
	pthread_create(&consumer_thread, NULL, consumer, dataTransfer);

	pthread_join(producer_thread, NULL);
	pthread_join(consumer_thread, NULL);

	/* Print the finial result. */
	printf("Average response time = %.6lf\n", (dataTransfer->responseTimeTotal) * 1.0 / NUMBER_OF_PROCESSES);
	printf("Average turn around time = %.6lf\n", (dataTransfer->turnAroundTimeTotal) * 1.0 / NUMBER_OF_PROCESSES);
	
	/* Free the spaces and destroy semaphores. */
	free(dataTransfer);
	semsDestroy();
}