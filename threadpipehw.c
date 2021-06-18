//
// Thread programming homework
// A simple thread pipeline (multiple single producer and consumer version)
// Student Name : 김형원
// Student Number : B735137
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

int * defectNumbers;

#define ENDMARK -1
struct autoPartBox *AutoBox;
pthread_barrier_t barrier;

long long NSTAGES = 0;
long long BOXSIZE = 0;
long long NPART = 0;

void sendAutoPart(int id, struct autoPart *ap, struct autoPartBox *apBox)
{
	printf("SEND:: Stage %d thread waiting on autPartBox %d full \n", id, apBox->bid);
	printf("SEND:: Stage %d sending autoPart %d to autoPartBox %d\n", id, apBox->lastPart->partNumber, apBox->bid);
	printf("SEND:: Stage %d signals autoBoxPart %d NOT empty\n", id, apBox->bid);
}

struct autoPart *receiveAutoPart(int id, struct autoPartBox *apBox)
{
	autoPart *autoPtr;
	printf("RECEIVE:: Stage %d waiting on autoPartBox %d empty\n", id, apBox->bid);
	printf("RECEIVE:: Stage %d receiving autoPart %d from autoPartBox %d\n", id, autoPtr->partNumber, apBox->bid);
	printf("RECEIVE:: Stage %d signals autoPartBox %d NOT full\n", id, apBox->bid);
	return (autoPtr);
}

// Generate autoParts and put the autoParts into the first autoPartBox
void *startThread(void *ag)
{
	//autoPart 부품을 만들어야함
	autoPart todo_autoPart;
	todo_autoPart.next = NULL;
	todo_autoPart.partNumber = 0;
	autoPart *autoPtr = &todo_autoPart;

	printf("Start Thread Stage %d sending autoPart %d to autoPartBox %d\n", 0, autoPtr->partNumber, 0);
	printf("Start Thread Stage %d sending ENDMARK to autoPartBox %d\n", 0, 0);

	pthread_barrier_wait(&barrier);
}

// Get autoParts from the last autoPartBox and add all of them
void *endThread(void *id)
{
	int tid = 0;
	autoPart *autoPtr;
	autoPart todo_autoPart;
	todo_autoPart.next = NULL;
	todo_autoPart.partNumber = 0;
	autoPtr = &todo_autoPart;
	printf("End Thread Stage %d receiving autoPart %d from autoPartBox %d\n", tid, autoPtr->partNumber, tid - 1);
	printf("End Thread Stage %d receiving ENDMARK from autoPartBox %d\n", id, tid - 1);

	pthread_barrier_wait(&barrier);
}

// Check autoParts from the input box and remove faulty parts
// Add all faulty parts number; Put valid autoParts into the output box
// The faulty part number is a multiple of the stage defect number
void *stageThread(void *ptr)
{

	stageArg todo_stageArg;
	todo_stageArg.defectNumber = 0;
	todo_stageArg.sid = 0;
	autoPart todo_autoPart;
	todo_autoPart.next = NULL;
	todo_autoPart.partNumber = 0;
	stageArg *stArg = &todo_stageArg;
	autoPart *autoPtr = &todo_autoPart;
	printf("Stage %d receiving autoPart %d from autoPartBox %d\n", stArg->sid, autoPtr->partNumber, stArg->sid - 1);
	printf("Stage %d deleting autoPart %d\n", stArg->sid, autoPtr->partNumber);
	printf("Stage %d sending autoPart %d to autoPartBox %d\n", stArg->sid, autoPtr->partNumber, stArg->sid);
	printf("Stage %d receiving ENDMARK from autoPartBox %d\n", stArg->sid, stArg->sid - 1);
	printf("Stage %d sending ENDMARK to autoPartBox %d\n", stArg->sid, stArg->sid);

	pthread_barrier_wait(&barrier);
}

int main(int argc, char *argv[])
{
	//main 인자값들 배정하기
	if (argc < 5)
	{
		printf("Usage: <executable> NSTAGES BOXSIZE NPART defect_numbers\n");
		exit(1);
	}
	
	NSTAGES = atoll(argv[1]);
	BOXSIZE = atoll(argv[2]);
	NPART = atoll(argv[3]);

	AutoBox = malloc(sizeof(struct autoPartBox) * (BOXSIZE + 1));
	if (AutoBox == NULL)
	{
		perror(strerror(errno));
		exit(2);
	}

	defectNumbers = malloc(sizeof(int) * NSTAGES);
	if (defectNumbers == NULL)
	{
		perror(strerror(errno));
		exit(2);
	}
	
	int i;
	for(i=0;i<NSTAGES;i++)
	{
		defectNumbers[i] = atoi(argv[i+4]);
	}

	//배리어 초기화
	pthread_barrier_init(&barrier, NULL, (NSTAGES+3));

	//thread 변수 선언 및 thread 생성
	long int startThreadSum, endThreadSum, stageThreadSum;
	void *status;
	startThreadSum = endThreadSum = stageThreadSum = 0;
	srand(100);
	pthread_t startTid, endTid;
	pthread_t * stageTid;
	stageTid = malloc(sizeof(pthread_t) * NSTAGES);
	int startArg, endArg;
	pthread_attr_t startAttribute, endAttribute;

	int * stageArgs;
	stageArgs = malloc(sizeof(int) * NSTAGES);

	pthread_create(&startThread, &startAttribute, startThread, (void *)&startArg);
	pthread_create(&endTid, &endAttribute, endThread, (void *)&endArg);
	
	for (i = 0; i < NSTAGES; i++)
	{
		pthread_create((stageTid + i), NULL, stageThread, (void *)(stageArgs + i));
	}

	pthread_barrier_wait(&barrier);
	printf("*** Part Sum Information ***\n");
	pthread_join(startTid, &status);
	printf("startThread sum %ld\n", status);
	startThreadSum = status;
	pthread_join(endTid, &status);
	printf("endThread sum %ld\n", status);
	endThreadSum = status;

	for (i = 0; i < NSTAGES; i++)
	{
		pthread_join(stageTid[i], &status);
		stageThreadSum += (long)status;
		printf("Stage %d sum %ld\n", i, status);
	}

	assert(startThreadSum == (endThreadSum + stageThreadSum));

	free(stageTid);
	free(stageArgs);
	free(defectNumbers);
	free(AutoBox);
	pthread_exit(0);
}
