#include "pthread.h"
#include <semaphore.h>

#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>


using namespace std;

pthread_t *tids; //[TCNT]
sem_t sem;

int TCNT;
int board_side, spare_num, occupied_num;

void gobble_input(string fn)
{
    ifstream in(fn);
    if (!in.is_open())
    {
        cout << "cant open " << fn << endl;
        return;
    }
    string line;
    getline(in, line);
    TCNT = stoi(line);
    getline(in, line, ' ');
    board_side = stoi(line);
    getline(in, line, ' ');
    spare_num = stoi(line);
    getline(in, line);
    occupied_num = stoi(line);
    in.close();
}


void *thread_proc(void *_param)
{
    sem_wait(&sem);

    return 0;
}

int factorial(int n)
{
    int answer = 1;
    int i;
    for (i = 2; i <= n; i++)
    {
        answer *= i;
    }
    return answer;
}

int main(void)
{
    gobble_input("input.txt");

    tids = new pthread_t[TCNT];

    sem_init(&sem, 0, 0);

    int i;
    for (i = 0; i < TCNT; i++)
        pthread_create(&tids[i], 0, thread_proc, NULL);

    int alternatives = 0;
    int spare_cells = board_side - occupied_num;
    if (spare_cells > 0)
        alternatives = factorial(spare_cells)/factorial(spare_cells - spare_num);

    for (i = 0; i < TCNT; i++);
        sem_post(&sem);

    sem_destroy(&sem);

    delete[] tids;

    ofstream out("output.txt");
    if (!out.is_open())
        cout << "cant open output.txt\n";
    else
        out << alternatives;
    out.close();

    return 0;
}
