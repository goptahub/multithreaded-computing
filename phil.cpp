#include <windows.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#define TCNT (5)
using namespace std;

int MAX_RUN_TIME = 0;
int MIN_SWAP_TIME = 0;
int start_tick;
HANDLE handles[TCNT];
HANDLE eat_events[TCNT];
HANDLE think_events[TCNT];
HANDLE write_mutex;
HANDLE vacant_eater;
//ofstream out;

#define RUN_COND(start) (clock() - start < MAX_RUN_TIME)

//int think_quant = 5;

typedef struct
{
	int idx;
	int start;
} thread_t;

thread_t threads[TCNT];

typedef enum
{
	LEFT, RIGHT
} fork_t;

typedef struct
{
	int who;
	fork_t fork;
} job_t;

struct node_st
{
	struct node_st* next;
	job_t job;
};
typedef struct node_st node_t;

class RR
{
	node_t* head;
	node_t* tail;
public:
	RR(void) : head(NULL), tail(NULL)
	{
		this->append(0, LEFT);
		this->append(2, RIGHT);
		this->append(4, LEFT);
		this->append(1, RIGHT);
		this->append(3, LEFT);
		this->append(0, RIGHT);
		this->append(2, LEFT);
		this->append(4, RIGHT);
		this->append(1, LEFT);
		this->append(3, RIGHT);
	}

	void append(int who, fork_t fork)
	{
		node_t* nd = new node_t;
		nd->job.who = who;
		nd->job.fork = fork;
		if (tail != NULL)
			tail->next = nd;
		tail = nd;
		if (head == NULL)
			head = nd;
		tail->next = head;
	}

	void get_item(job_t& job)
	{
		job = head->job;
		head = head->next;
	}

	void clear(void)
	{
		node_t* nd = tail, *prev;
		while (nd != NULL)
		{
			prev = nd;
			nd = prev->next;
			delete prev;
		}
		head = NULL;
		tail = NULL;
	}
};

//log in ms
void log(int who, const char* from, const char* to)
{
	int time = (clock() - start_tick);
	cout << time << ":" << ++who<< ":" << from << "->" << to << endl;
}

DWORD WINAPI thread_work(void* param)
{
	thread_t* t = (thread_t*)param;
	int who = t->idx;
	int start = t->start;
	while (1)
	{
		WaitForSingleObject(eat_events[who], INFINITE);

		WaitForSingleObject(write_mutex, INFINITE);
		//cout << "["<< who <<"]" << clock() - start << endl;
		if (!(RUN_COND(start)))
		{
			ReleaseMutex(write_mutex);
			return 0;
		}
		log(who, "T", "E");
		//cout << clock() - start << endl;
		ReleaseMutex(write_mutex);

		Sleep(MIN_SWAP_TIME);

		WaitForSingleObject(write_mutex, INFINITE);
		log(who, "E", "T");
		//cout << clock() - start << endl;
		//think_quant++;
		ReleaseMutex(write_mutex);

		PulseEvent(think_events[who]);
		SetEvent(vacant_eater);
		
	}
	return 0;
}

//return ms
unsigned int wait_time(int start)
{
	int t = MIN_SWAP_TIME - (clock() - start);
	if (t <= 0)
		return 0;
	return (unsigned int)t;
}

int main(int argc, char** argv)
{
	RR plan;
	int i;

	/*out.open((string)argv[3]);
	if (!out.is_open())
	{
		cout << "cant open output.txt";
		return EXIT_SUCCESS;
	}*/

	MAX_RUN_TIME = stoi(argv[1]);
	MIN_SWAP_TIME = stoi(argv[2]);

	eat_events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	eat_events[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
	eat_events[2] = CreateEvent(NULL, FALSE, FALSE, NULL);
	eat_events[3] = CreateEvent(NULL, FALSE, FALSE, NULL);
	eat_events[4] = CreateEvent(NULL, FALSE, FALSE, NULL);

	think_events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	think_events[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
	think_events[2] = CreateEvent(NULL, FALSE, FALSE, NULL);
	think_events[3] = CreateEvent(NULL, FALSE, FALSE, NULL);
	think_events[4] = CreateEvent(NULL, FALSE, FALSE, NULL);

	vacant_eater = CreateEvent(NULL, FALSE, FALSE, NULL);
	write_mutex = CreateMutex(NULL, FALSE, NULL);

	for (i = 0; i < TCNT; i++)
		threads[i].idx = i;
		

	for (i = 0; i < TCNT; i++)
	{
		handles[i] = CreateThread(0, 0, thread_work, (void*)&threads[i], 0, NULL);
		if (0 == handles[i])
		{
			printf("CreateThread failed. GetLastError(): %u\n", GetLastError());
			return -1;
		}
	}
	start_tick = clock();
	for (i = 0; i < TCNT; i++)
		threads[i].start = start_tick;
	

	Sleep(wait_time(start_tick)); // phils are thinking at the beginning
	
	for (i = 0; i < 2; i++) // phil triple
	{
		job_t job;
		plan.get_item(job);
		SetEvent(eat_events[job.who]);
		Sleep(1);
	}

	while (RUN_COND(start_tick)) // ask the next eater over when some phil stop eating
	{
		WaitForSingleObject(vacant_eater, INFINITE);
		//cout << clock() - start_tick << endl;
		job_t job;
		plan.get_item(job);
		SetEvent(eat_events[job.who]);
	}

	int start = clock();
	for (i = 0; i < TCNT; i++)
	{
		int t = wait_time(start);
		WaitForSingleObject(think_events[i], t);
		//cout << "___t: " << t << "___" << endl;
	}
		

	CloseHandle(write_mutex);
	CloseHandle(vacant_eater);
	for (i = 0; i < TCNT; i++)
		CloseHandle(handles[i]);

	//out.close();

	//cout << think_quant << " " << think_quant-5 << endl;

	return EXIT_SUCCESS;
}
