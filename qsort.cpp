#include "pthread.h"
#include <semaphore.h>

#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <deque>

using namespace std;

#define v(name, value) cout << name << "=" << value << endl;

class Worker;

Worker* threads; //[TCNT]
pthread_t *tids; //[TCNT]

sem_t give_sem;
sem_t record_sem;

/*memory control*/
sem_t jobs_mem_sem;
sem_t waiting_mem_sem;
sem_t finished_mem_sem;

int TCNT;
int N;
int *sequence;

void v_seq(void)
{
    for (int i = 0; i < N-1; i++){
        cout << sequence[i] << ' ';
    }
    cout << sequence[N-1] << endl;
}

class Slice
{
public:
    int first, last, pivot;

    Slice(int first, int last) : first(first), last(last), pivot(last) {}

    void swap(int idx1, int idx2)
    {
        if (idx1 >= idx2)
        {
            cout << "idx1 >= idx2" << endl;
            return;
        }
        int tmp;
        if (idx1 + 1 == idx2)
        {
            tmp = sequence[idx1];
            sequence[idx1] = sequence[idx2];
            sequence[idx2] = tmp;
            pivot = idx1;
        }
        else
        {
            tmp = sequence[idx1];
            sequence[idx1] = sequence[idx2 - 1];
            sequence[idx2 - 1] = sequence[idx2];
            sequence[idx2] = tmp;
            pivot = idx2 - 1;
        }
    }
};

typedef enum
{
    big_job,
    small_job,
    first_job
} job_type_t;

typedef struct
{
    int first_idx,
        last_idx;
    job_type_t type;
} job_t;

class Distributor
{
    deque<job_t> jobs;
    int waiting_jobs = 0;
    deque<pair<job_t, int>> finished_jobs;

public:
    Distributor()
    {
        job_t start_job = {.first_idx=0, .last_idx=N-1, .type=first_job};
        finished_jobs.push_back({start_job, N-1});
    }

    bool give_job(job_t &job)
    {
        /*wait jobs sem*/
        sem_wait(&jobs_mem_sem);
        if (jobs.empty())
        {
            /*post jobs sem*/
            sem_post(&jobs_mem_sem);
            return false;
        }
        job = jobs.front();
        jobs.pop_front();
        /*post jobs sem*/
        sem_post(&jobs_mem_sem);

        /*wait waiting sem*/
        sem_wait(&waiting_mem_sem);
        waiting_jobs++;
        /*post waiting sem*/
        sem_post(&waiting_mem_sem);

        return true;
    }

    job_type_t define_job_type(int idx1, int idx2)
    {
        int cnt = idx2 - idx1 + 1;
        if (cnt <= 1000)
            return small_job;
        else
            return big_job;
    }

    void append_job(int idx1, int idx2)
    {
        job_type_t type = define_job_type(idx1, idx2);
        job_t new_job = {.first_idx = idx1, .last_idx = idx2, .type = type};
        /*wait jobs sem*/
        sem_wait(&jobs_mem_sem);
        jobs.push_back(new_job);
        /*post jobs sem*/
        sem_post(&jobs_mem_sem);
    }

    int plan_job()
    {
        /*wait finished sem*/
        sem_wait(&finished_mem_sem);
        if (finished_jobs.empty())
        {
            /*post finished sem*/
            cout << "finished_jobs.empty()\n";
            sem_post(&finished_mem_sem);
            return 0;
        }
            
        pair<job_t, int> finished_job = finished_jobs.front();
        finished_jobs.pop_front();
        /*post finished sem*/
        sem_post(&finished_mem_sem);
        if (finished_job.first.type == small_job)
            return 0;
            
        int idx1 = finished_job.first.first_idx;
        int idx2 = finished_job.first.last_idx;

        if (finished_job.first.type == first_job)
        {
            append_job(idx1, idx2);
            return 1;
        }

        int pivot = finished_job.second;

        if (pivot == idx1 || pivot == idx1 + 1)
        {
            append_job(pivot + 1, idx2);
            return 1;
        }
        else if (pivot == idx2 || pivot + 1 == idx2)
        {
            append_job(idx1, pivot - 1);
            return 1;
        }
        else
        {
            append_job(idx1, pivot - 1);
            append_job(pivot + 1, idx2);
            return 2;
        }
    }

    void record_finished_job(job_t &job, int pivot)
    {
        /*wait waiting sem*/
        sem_wait(&waiting_mem_sem);
        waiting_jobs--;
        /*post waiting sem*/
        sem_post(&waiting_mem_sem);

        /*wait finished sem*/
        sem_wait(&finished_mem_sem);
        finished_jobs.push_back({job, pivot});
        /*post finished sem*/
        sem_post(&finished_mem_sem);
    }

    bool task_complete()
    {
        return (jobs.empty() && waiting_jobs == 0 && finished_jobs.empty());
    }
};

class Worker
{
    Distributor *dt;
    job_t job;
    int pivot;
    // int q = 3;

public:
    Worker(void) {}

    void set_dt(Distributor *_dt){dt = _dt;}

    void take_job()
    {
        dt->give_job(job);
        // cout << "["<< job.first_idx << "," << job.last_idx << "," << job.type << "]\n";
    //    bool res = dt->give_job(job);
    //    if (res)
    //     cout << "got job\n";
    }

    void do_job()
    {
        if (job.type == small_job)
            sort_recursively(job.first_idx, job.last_idx);
        else
            sort();
    }

    void sort()
    {
        Slice slice(job.first_idx, job.last_idx);

        int pre;
        int i = slice.first;
        while (i < slice.pivot)
        {
            pre = i;
            if (sequence[i] >= sequence[slice.pivot])
            {
                slice.swap(i, slice.pivot);
                i = pre;
                continue;
            }
            i++;
        }

        pivot = slice.pivot;
    }

    void sort_recursively(int idx1, int idx2)
    {
        // if (q == 0) return;
        // else q--;
        // cout << "[" << idx1 << "," << idx2 << "]\n";
        if (idx1 < 0 || idx2 < 0 || idx1 >= idx2)
            return;

        Slice slice(idx1, idx2);

        int pre;
        int i = slice.first;
        while (i < slice.pivot)
        {
            // v("pivot", slice.pivot)
            // v("first", slice.first)
            // v("i", i)
            pre = i;
            if (sequence[i] >= sequence[slice.pivot])
            {
                slice.swap(i, slice.pivot);
                i = pre;
                // v_seq();
                continue;
            }
            i++;
        }

        // cout << "--------------------------------\n";
        int pivot = slice.pivot;
        int _idx1 = slice.first;
        int _idx2 = slice.last;
        // v("pivot", pivot)
        // v("_idx1", _idx1);
        // v("_idx2", _idx2);

        if (pivot == _idx1 || pivot == _idx1 + 1)
        {
            // cout << 1 << endl;
            sort_recursively(pivot + 1, _idx2);
        }
        else if (pivot == _idx2 || pivot + 1 == _idx2)
        {
            // cout << 2 << endl;
            sort_recursively(_idx1, pivot - 1);
        }
        else
        {
            sort_recursively(_idx1, pivot - 1);
            sort_recursively(pivot + 1, _idx2);
        }
        // sort_recursively(slice.first, slice.pivot - 1);
        // sort_recursively(slice.pivot + 1, slice.last);
    }

    void report_finished_job()
    {
        dt->record_finished_job(job, pivot);
    }

};

void *thread_proc(void *_param)
{
    Worker *wk = (Worker *)_param;
    while (1)
    {
        sem_wait(&give_sem);
        wk->take_job();
        wk->do_job();
        wk->report_finished_job();
        sem_post(&record_sem);
    }

    return 0;
}

void threads_init(Distributor *dt)
{
    tids = new pthread_t[TCNT];
    threads = new Worker[TCNT];
    for (int i = 0; i < TCNT; i++)
        threads[i].set_dt(dt);
}

void threads_free()
{
    delete[] tids;
    delete[] threads;
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

void record_output(int* seq)
{
    ofstream out("output.txt");
    if (!out.is_open())
    {
        cout << "cant create time.txt\n";
        return;
    }
    
    out << TCNT << endl << N << endl;
    for (int i = 0; i < N-1; i++)
        out << seq[i] << ' ';
    out << seq[N-1];
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
    sequence = new int[N];
    for (int i = 0; i < N; i++)
    {
        getline(in, line, ' ');
        sequence[i] = stoi(line);
    }

    in.close();
}

int main(int argc, char** argv)
{
    // gobble_input((string)argv[1]);
    gobble_input("input.txt");

    sem_init(&give_sem, 0, 0); // 3rd arg = 0
    sem_init(&record_sem, 0, 1);
    sem_init(&jobs_mem_sem, 0, 1);
    sem_init(&waiting_mem_sem, 0, 1);
    sem_init(&finished_mem_sem, 0, 1);

    Distributor dt;
    int planned_jobs;

    threads_init(&dt);

    // cout << "Creating threads...\n";
    for (int i = 0; i < TCNT; i++)
        pthread_create(&tids[i], 0, thread_proc, (void *)&threads[i]);
    // cout << "Waiting for completion...\n";

    clock_t start_tick = clock();

    while (1)
    {
        sem_wait(&record_sem);
        planned_jobs = dt.plan_job();
        // cout << "planned_jobs=" << planned_jobs << endl;

        while (planned_jobs--)
            sem_post(&give_sem);

        if (dt.task_complete())
            break;
    }

    clock_t end_tick = clock();

    sem_destroy(&give_sem);
    sem_destroy(&record_sem);
    sem_destroy(&jobs_mem_sem);
    sem_destroy(&waiting_mem_sem);
    sem_destroy(&finished_mem_sem);

    int elapsed_time = (int)(end_tick - start_tick) / 1000;
    record_time(elapsed_time);
    record_output(sequence);

    delete[] sequence;
    return EXIT_SUCCESS;
}