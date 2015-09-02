#ifndef __HASHGLOBAL_H__
#define __HASHGLOBAL_H__

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define PAGE_SIZE (1 << 18)
#define GROUP_SIZE (PAGE_SIZE / 3)
#define ALIGNMET 8

#define HOST_BUFFER_SIZE (1 << 31)

#define BLOCK_ID (gridDim.y * blockIdx.x + blockIdx.y)
#define THREAD_ID (threadIdx.x)
#define TID (BLOCK_ID * blockDim.x + THREAD_ID)

enum recordType { UNTESTED = 0, SUCCEED = 1, FAILED = 2};

typedef long long int largeInt;

//================ paging structures ================//

typedef struct page_t
{
	struct page_t* next;
	unsigned used;
	short id;
	short needed;
} page_t;

typedef struct
{
	page_t* pages;
	page_t* hpages;

	void* dbuffer;
	//void* hbuffer;
	largeInt hashTableOffset;
	int totalNumPages;

	int initialPageAssignedCounter;
	int initialPageAssignedCap;
} pagingConfig_t;



//================ hashing structures ================//

//Key and value will be appended to the instance of hashBucket_t every time `multipassMalloc` is called in `add`
//NOTE: sizeof(hashBucket_t) should be aligned by ALIGNMET
typedef struct hashBucket_t
{
	struct hashBucket_t* next;
	short isNextDead;
	unsigned short lock;
	short keySize;
	short valueSize;
} hashBucket_t;

typedef struct
{
	hashBucket_t* buckets[GROUP_SIZE];
	unsigned locks[GROUP_SIZE];
	short isNextDead[GROUP_SIZE];
	page_t* parentPage;
	unsigned pageLock;
	volatile int failed;
	int refCount;
	int inactive;
	int failedRequests;
	int needed;

} bucketGroup_t;

typedef struct
{
	bucketGroup_t* groups;
	int numBuckets;
} hashtableConfig_t;

typedef struct
{
	int* hostCompleteFlag;
	int* gpuFlags;
	bool* dfailedFlag;
	pagingConfig_t* pconfig;
	pagingConfig_t* dpconfig;
	hashtableConfig_t* hconfig;
	hashtableConfig_t* dhconfig;
	int* myNumbers;
	int* dmyNumbers;
	void* hhashTableBaseAddr;
	largeInt hhashTableBufferSize;
	size_t availableGPUMemory;
	char* epochSuccessStatus;
	char* depochSuccessStatus;
	char* dstates;
	int numGroups;
	int groupSize;
	int flagSize;
	int numThreads;
	int epochNum;
	int numRecords;
} multipassBookkeeping_t;




void initPaging(largeInt availableGPUMemory, pagingConfig_t* pconfig);
__device__ void* multipassMalloc(unsigned size, bucketGroup_t* myGroup, pagingConfig_t* pconfig, int groupNo);
__device__ page_t* allocateNewPage(pagingConfig_t* pconfig, int groupNo);


void hashtableInit(int numBuckets, hashtableConfig_t* hconfig);
__device__ unsigned int hashFunc(char* str, int len, unsigned numBuckets);
__device__ void resolveSameKeyAddition(void const* key, void* value, void* oldValue);
__device__ hashBucket_t* containsKey(hashBucket_t* bucket, void* key, int keySize, pagingConfig_t* pconfig);
__device__ bool addToHashtable(void* key, int keySize, void* value, int valueSize, hashtableConfig_t* hconfig, pagingConfig_t* pconfig);
__device__ bool atomicAttemptIncRefCount(int* refCount);
__device__ int atomicDecRefCount(int* refCount);
__device__ bool atomicNegateRefCount(int* refCount);

multipassBookkeeping_t* initMultipassBookkeeping(int* hostCompleteFlag, 
						int* gpuFlags, 
						int flagSize,
						int* dmyNumbers, 
						int groupSize,
						int numThreads,
						int epochNum,
						int numRecords);

__global__ void setGroupsPointersDead(bucketGroup_t* groups, int numGroups);
bool checkAndResetPass(multipassBookkeeping_t* mbk);
#endif
