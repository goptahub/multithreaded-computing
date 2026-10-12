#ifndef _REENTRANT
#define _REENTRANT
#endif

#include "pthread.h"

#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include <time.h>
clock_t start_tick, end_tick;
int elapsed_time;

using namespace std;

#define db(str, val) cout << str << " " << val << endl;


int TCNT;
int N;
int S;
vector<int> num_sequence;
volatile int alternatives = 0;

typedef struct thread_str
{
    int start_mask;
    int end_mask;
    vector<int> seq;
    int N;
    int S;
} thread_t;

thread_t* threads;
pthread_t *tids;
pthread_mutex_t alter_mutex;
pthread_mutex_t cout_mutex;

void print_vec(vector<int> &v)
{
    for (auto &q : v)
        cout << q << " ";
    cout << endl;
}

void *thread_proc(void* _param)
{
    thread_t* td = (thread_t*) _param;
    int mask = td->start_mask;
    int end_mask = td->end_mask;
    int _N = td->N;
    int _S = td->S;
    vector<int> _seq = td->seq;

    int cnt = 0;

    while (mask <= end_mask)
    {
        vector<int> seq = _seq;
        for (int i = 0; i < _N - 1; i++)
            if ((mask >> i) & 1)
                seq[i + 1] *= -1;

        int sum = 0;
        for (int &n : seq)
            sum += n;

        // pthread_mutex_lock(&cout_mutex);
        // cout << "mask:" << mask << endl;
        // print_vec(seq);
        // pthread_mutex_unlock(&cout_mutex);
        
        if (sum == _S)
            cnt++;

        mask++;
    }

    pthread_mutex_lock(&alter_mutex);
    alternatives += cnt;
    pthread_mutex_unlock(&alter_mutex);

    return 0;
}

void threads_init()
{
    tids = new pthread_t[TCNT];
    threads = new thread_t[TCNT];
    int all_vars = (1 << (N - 1));
    // cout << "all_vars " << all_vars << endl;
    int portion = all_vars / TCNT;
    int recess = all_vars % TCNT;
    int mask = 0;
    // cout << "portion " << portion << endl;
    // cout << "recess " << recess << endl;
    for (int i = 0; i < TCNT; i++)
    {
        threads[i].start_mask = mask;
        mask += portion;
        threads[i].end_mask = mask - 1;

        threads[i].seq = num_sequence;
        threads[i].N = N;
        threads[i].S = S;
        // db("i", i);
        // db("s", threads[i].start_mask);
        // db("e", threads[i].end_mask);
    }
    if (recess)
        threads[TCNT - 1].end_mask += recess;
}

void threads_free()
{
    delete[] tids;
    delete[] threads;
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
    for (int i = 0; i < N; i++)
    {
        getline(in, line, ' ');
        num_sequence.push_back(stoi(line));
    }
    getline(in, line);
    S = stoi(line);

    in.close();
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

void record_output(int n)
{
    ofstream out("output.txt");
    if (!out.is_open())
    {
        cout << "cant create output.txt\n";
        return;
    }
    out << TCNT << endl << N << endl << n;
    out.close();
}

int main(int argc, char **argv)
{
    // gobble_input((string)argv[1]);
    gobble_input("input.txt");

    // cout << TCNT << " " << N << " " << S << endl;

    if (N - 1 > sizeof(int)*8)
    {
        cout << "too long sequence\n";
        record_time(-1);
        record_output(-1);
        return 0;
    }
    
    threads_init();
    pthread_mutex_init(&alter_mutex, 0);
    pthread_mutex_init(&cout_mutex, 0);
    for (int i = 0; i < TCNT; i++)
        pthread_create(&tids[i], 0, thread_proc, (void *)&threads[i]);

    start_tick = clock();

    for(int i = 0; i < TCNT; i++)
        pthread_join(tids[i], 0);

    end_tick = clock();
    pthread_mutex_destroy(&alter_mutex);
    pthread_mutex_destroy(&cout_mutex);
    threads_free();
    // cout << "alternatives=" << alternatives << endl;

    elapsed_time = (int)(end_tick - start_tick) / 1000;
    record_time(elapsed_time);
    record_output(alternatives);

    return EXIT_SUCCESS;
}
