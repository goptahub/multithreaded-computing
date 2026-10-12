#include <windows.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

#define MIN_SLICE_SZ 5
class Worker;

HANDLE* handles;
Worker* threads;
CRITICAL_SECTION cs; /*slices, waiting_jobs, starting_jobs*/
HANDLE start_sem;

int TCNT;
int N;
vector<int> sequence;

typedef vector<int> slice_t;
vector<slice_t> common_slices;
int start_jobs = 0;

// void show_s(slice_t& v)
// {
// 	for (auto& n : v)
// 		cout << n << " ";
// 	cout << endl;
// }
// //
// void show_slices(vector<slice_t>& ss)
// {
// 	cout << "slices\n";
// 	for (auto& slice : ss)
// 	{
// 		cout << "[";
// 		for (auto& n : slice)
// 			cout << n << ' ';
// 		cout << "]\n";
// 	}
// }

typedef enum
{
	sort_job, merge_job
} job_t;

void create_slices(int min_slice_sz)
{
	int i = 0;
	while (i < N)
	{
		slice_t slice;

		int slice_sz = min_slice_sz;
		if (i + min_slice_sz >= N)
			slice_sz = N - i;

		slice.insert(
			slice.end(),
			sequence.begin() + i,
			sequence.begin() + i + slice_sz);

		common_slices.push_back(slice);
		i += slice_sz;
		start_jobs++;
	}
}

class Worker
{
	//vector<slice_t> woker_slices;
public:
	bool process_job(slice_t& s1, slice_t& s2, job_t& job_type)
	{
		/*cout << "--------------------\n";
		cout << "sz: " << common_slices.size() << endl;
		show_slices(common_slices);
		cout << "++++++++++++++++++++\n";*/
		if (common_slices.size() > 1 || start_jobs > 0)
		{
			s1 = common_slices.front();
			/*cout << "s1.";
			show_s(s1);*/
			common_slices.erase(common_slices.begin());
			if (start_jobs > 0)
			{
				start_jobs--;
				job_type = sort_job;

				return true;
			}
			job_type = merge_job;
			s2 = common_slices.front();
			/*cout << "s2.";
			show_s(s2);*/
			common_slices.erase(common_slices.begin());

			return true;
		}
		return false;
	}

	void merge(slice_t& slice1, slice_t& slice2, slice_t& merged)
	{
		int len1 = slice1.size(), len2 = slice2.size();
		merged.assign(len1 + len2, 0);
		int i = 0, k = 0, j = 0;
		while (i < len1 && k < len2)
		{
			if (slice1[i] <= slice2[k])
				merged[j++] = slice1[i++];
			else
				merged[j++] = slice2[k++];

			//show_s(merged);
		}

		while (i < len1)
			merged[j++] = slice1[i++];

		while (k < len2)
			merged[j++] = slice2[k++];
	}
};

DWORD WINAPI thread_work(void* param)
{
	Worker* wk = (Worker*)param;
	WaitForSingleObject(&start_sem, INFINITE);
	while (1)
	{
		slice_t s1, s2;
		job_t job_type;
		EnterCriticalSection(&cs);
		bool res = wk->process_job(s1, s2, job_type);
		LeaveCriticalSection(&cs);

		if (!res) break;

		if (job_type == merge_job)
		{
			slice_t merged_slice;
			wk->merge(s1, s2, merged_slice);
			EnterCriticalSection(&cs);
			common_slices.push_back(merged_slice);
			/*show_s(merged_slice);
			show_slices(common_slices);*/
			LeaveCriticalSection(&cs);
		}
		else
		{
			sort(s1.begin(), s1.end());
			EnterCriticalSection(&cs);
			common_slices.push_back(s1);
			/*show_s(s1);
			show_slices(common_slices);*/
			LeaveCriticalSection(&cs);
		}
	}
	return 0;
}

void record_time(int t)
{
	ofstream out("time.txt");
	if (!out.is_open())
	{
		cout << "cant create time.txt\n";
		return;
	}

	out << t;
	out.close();
}

void record_output(vector<int>& seq)
{
	ofstream out("output.txt");
	if (!out.is_open())
	{
		cout << "cant create time.txt\n";
		return;
	}

	out << TCNT << endl << N << endl;
	int i;
	for (i = 0; i < N - 1; i++)
		out << seq[i] << ' ';
	out << seq[N - 1] << endl;
	out.close();
}

void gobble_input(string fn)
{
	ifstream in(fn);
	if (!in.is_open())
	{
		cout << "cant open input.txt" << endl;
		exit(1);
	}
	string line;
	getline(in, line);
	TCNT = stoi(line);
	getline(in, line);
	N = stoi(line);
	int i;
	for (i = 0; i < N; i++)
	{
		getline(in, line, ' ');
		sequence.push_back(stoi(line));
	}

	in.close();
}

void threads_init(void)
{
	handles = new HANDLE[TCNT];
	threads = new Worker[TCNT];
}

void threads_del()
{
	int i;
	for (i = 0; i < TCNT; i++)
		CloseHandle(handles[i]);
	delete[] handles;
	delete[] threads;
}

int main(int argc, char** argv)
{
	// gobble_input((string)argv[1]);
	gobble_input("input.txt");
	
	threads_init();
	create_slices(MIN_SLICE_SZ);
	//show_slices(common_slices);
	

	InitializeCriticalSection(&cs);
	start_sem = CreateSemaphore(0, 0, TCNT, 0);

	int i;
	for (i = 0; i < TCNT; i++)
	{
		handles[i] = CreateThread(0, 0, thread_work, (void*)&threads[i], 0, NULL);
		if (0 == handles[i])
		{
			printf("CreateThread failed. GetLastError(): %u\n", GetLastError());
			return -1;
		}
	}

	clock_t start_tick = clock();
	ReleaseSemaphore(&start_sem, TCNT, NULL);
	WaitForMultipleObjects(TCNT, handles, TRUE, INFINITE);
	clock_t end_tick = clock();

	DeleteCriticalSection(&cs);
	CloseHandle(&start_sem);

	threads_del();

	int elapsed_time = (int)(end_tick - start_tick);
	record_time(elapsed_time);
	/*cout << common_slices.size() << endl;
	show_slices(common_slices);*/
	record_output(common_slices.front());

	return EXIT_SUCCESS;
}