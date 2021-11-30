// A simple thread pipeline (multiple single producer and consumer version)
// Name : 김형원
//
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <error.h>
#include <errno.h>
#include <string.h>

struct autoPart
{
	int partNumber;
	struct autoPart *next;
};
typedef struct autoPart autoPart;

struct autoPartBox
{
	int bid;					// autoPartBox id
	int SIZE;					// SIZE Of autoPartBox
	int count;					// the number of autoParts in the Box
	struct autoPart *lastPart;	// Pointer to the last auto part
	struct autoPart *firstPart; // Pointer to the first auto part
	pthread_mutex_t mutex;
	pthread_cond_t full;
	pthread_cond_t empty;
};
typedef struct autoPartBox autoPartBox;

struct stageArg
{
	int sid;
	int defectNumber;
};
typedef struct stageArg stageArg;

#define ENDMARK -1
struct autoPartBox *AutoBox;
pthread_barrier_t barrier;

void sendAutoPart(int id, struct autoPart *ap, struct autoPartBox *apBox)
{
	pthread_mutex_lock(&(apBox->mutex));
	if (apBox->count == apBox->SIZE)
	{
		printf("SEND:: Stage %d thread waiting on autPartBox %d full \n", id, apBox->bid);
		pthread_cond_wait(&(apBox->full), &(apBox->mutex));
	}
	if (apBox->count == 0)
	{
		(apBox->count) += 1;
		apBox->firstPart = apBox->lastPart = ap;
		printf("SEND:: Stage %d sending autoPart %d to autoPartBox %d\n", id, apBox->lastPart->partNumber, apBox->bid);
		printf("SEND:: Stage %d signals autoBoxPart %d NOT empty\n", id, apBox->bid);
		pthread_cond_signal(&(apBox->empty));
	}
	else
	{
		(apBox->count) += 1;
		apBox->lastPart->next = ap;
		apBox->lastPart = ap;
		printf("SEND:: Stage %d sending autoPart %d to autoPartBox %d\n", id, apBox->lastPart->partNumber, apBox->bid);
	}
	pthread_mutex_unlock(&(apBox->mutex));
}

struct autoPart *receiveAutoPart(int id, struct autoPartBox *apBox)
{
	autoPart *autoPtr;
	pthread_mutex_lock(&(apBox->mutex));
	if (apBox->count == 0)
	{
		printf("RECEIVE:: Stage %d waiting on autoPartBox %d empty\n", id, apBox->bid);
		pthread_cond_wait(&(apBox->empty), &(apBox->mutex));
	}

	autoPtr = apBox->firstPart;
	apBox->firstPart = apBox->firstPart->next;
	if (apBox->count == 1)
	{
		apBox->lastPart = NULL;
	}
	printf("RECEIVE:: Stage %d receiving autoPart %d from autoPartBox %d\n", id, autoPtr->partNumber, apBox->bid);
	if (apBox->count == apBox->SIZE)
	{
		apBox->count -= 1;
		printf("RECEIVE:: Stage %d signals autoPartBox %d NOT full\n", id, apBox->bid);
		pthread_cond_signal(&(apBox->full));
	}
	else
	{
		apBox->count -= 1;
	}
	pthread_mutex_unlock(&(apBox->mutex));
	return (autoPtr);
}

// Generate autoParts and put the autoParts into the first autoPartBox
void *startThread(void *ag)
{
	int count;
	long sum = 0;
	long NPART = (long) ag;
	autoPart *autoPtr = NULL;
	for (count = 0; count < NPART; count++)
	{
		//create new autoPart
		autoPtr = malloc(sizeof(struct autoPart));
		autoPtr->next = NULL;
		autoPtr->partNumber = abs(rand());
		sum += autoPtr->partNumber;
		printf("Start Thread Stage %d sending autoPart %d to autoPartBox %d\n", 0, autoPtr->partNumber, 0);
		sendAutoPart(0, autoPtr, AutoBox);
	}
	autoPtr = malloc(sizeof(struct autoPart));
	autoPtr->next = NULL;
	autoPtr->partNumber = ENDMARK;
	printf("Start Thread Stage %d sending ENDMARK to autoPartBox %d\n", 0, 0);
	sendAutoPart(0, autoPtr, AutoBox);

	pthread_barrier_wait(&barrier);
	pthread_exit((void *)sum);
}

// Get autoParts from the last autoPartBox and add all of them
void *endThread(void *id)
{
	int count;
	long sum = 0;
	int tid = (int)((long)id);
	struct autoPart *autoPtr = NULL;

	while (1)
	{
		autoPtr = receiveAutoPart(tid, AutoBox + (tid - 1));
		if (autoPtr->partNumber != ENDMARK)
		{
			printf("End Thread Stage %d receiving autoPart %d from autoPartBox %d\n", tid, autoPtr->partNumber, tid - 1);
			sum += (autoPtr->partNumber);
		}
		else
		{
			printf("End Thread Stage %d receiving ENDMARK from autoPartBox %d\n", tid, tid - 1);
			break;
		}
	}
	pthread_barrier_wait(&barrier);
	pthread_exit((void *)sum);
}

// Check autoParts from the input box and remove faulty parts
// Add all faulty parts number; Put valid autoParts into the output box
// The faulty part number is a multiple of the stage defect number
void *stageThread(void *ptr)
{
	long sum = 0;
	stageArg *stArg = (stageArg *)ptr;
	int defectNumber = stArg->defectNumber;
	autoPart *autoPtr = NULL;

	while (1)
	{
		autoPtr = receiveAutoPart(stArg->sid, AutoBox + ((stArg->sid) - 1));
		if (autoPtr->partNumber != ENDMARK)
		{
			printf("Stage %d receiving autoPart %d from autoPartBox %d\n", stArg->sid, autoPtr->partNumber, stArg->sid - 1);
			if (((autoPtr->partNumber) % defectNumber) == 0)
			{
				printf("Stage %d deleting autoPart %d\n", stArg->sid, autoPtr->partNumber);
				sum += autoPtr->partNumber;
				continue;
			}
			else
			{
				printf("Stage %d sending autoPart %d to autoPartBox %d\n", stArg->sid, autoPtr->partNumber, stArg->sid);
				sendAutoPart(stArg->sid, autoPtr, AutoBox + (stArg->sid));
			}
		}
		else
		{
			printf("Stage %d receiving ENDMARK from autoPartBox %d\n", stArg->sid, stArg->sid - 1);
			printf("Stage %d sending ENDMARK to autoPartBox %d\n", stArg->sid, stArg->sid);
			sendAutoPart(stArg->sid, autoPtr, AutoBox + (stArg->sid));
			break;
		}
	}
	pthread_barrier_wait(&barrier);
	pthread_exit((void *)sum);
}

int main(int argc, char *argv[])
{
	long long nStages, BOXSIZE, NPART;
	if (argc < 5)
	{
		printf("Usage: <executable> NSTAGES BOXSIZE NPART defect_numbers\n");
		exit(1);
	}
	nStages = atoll(argv[1]);
	BOXSIZE = atoll(argv[2]);
	NPART = atoll(argv[3]);
	AutoBox = malloc(sizeof(struct autoPartBox) * (nStages + 1));
	if (AutoBox == NULL)
	{
		perror(strerror(errno));
		exit(2);
	}
	int i, j;
	j = nStages + 1;
	struct autoPartBox *temp;
	for (i = 0; i < j; i++)
	{
		temp = (AutoBox + i);
		temp->bid = i;
		temp->count = 0;
		temp->firstPart = NULL;
		temp->lastPart = NULL;
		pthread_mutex_init(&(temp->mutex), NULL);
		pthread_cond_init(&(temp->empty), NULL);
		pthread_cond_init(&(temp->full), NULL);
		temp->SIZE = BOXSIZE;
	}
	pthread_barrier_init(&barrier, NULL, (nStages + 3)); //initialize barrier
	//creating threads
	long int startThreadSum, endThreadSum, stageThreadSum;
	startThreadSum = endThreadSum = stageThreadSum = 0;
	srand(100);
	pthread_t startTid, endTid;
	pthread_t *stageTid;
	stageTid = malloc(sizeof(pthread_t) * nStages);
	pthread_create(&startTid, NULL, startThread, (void *)(NPART));
	pthread_create(&endTid, NULL, endThread, (void *)(nStages + 1));
	stageArg *stageArgs = malloc(sizeof(struct stageArg) * nStages);
	for (i = 0; i < nStages; i++)
	{
		stageArgs[i].sid = i + 1;
		stageArgs[i].defectNumber = atoi(argv[i + 4]);
		pthread_create((stageTid + i), NULL, stageThread, (void *)(stageArgs + i));
	}
	long status;
	pthread_barrier_wait(&barrier);
	printf("*** Part Sum Information ***\n");
	pthread_join(startTid, (void **)&status);
	printf("startThread sum %ld\n", status);
	startThreadSum = (long)status;
	pthread_join(endTid, (void **)&status);
	printf("endThread sum %ld\n", status);
	endThreadSum = (long)status;
	for (i = 0; i < nStages; i++){
		pthread_join(stageTid[i], (void **)&status);
		stageThreadSum += (long)status;
		printf("Stage %d sum %ld\n", i, status);
	}
	assert(startThreadSum == (endThreadSum + stageThreadSum));
	free(stageTid);
	free(stageArgs);
	free(AutoBox);
	pthread_exit(0);
}
