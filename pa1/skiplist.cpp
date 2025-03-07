/*
 * main.cpp
 *
 * Serial version
 *
 * Compile with -O3
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <queue>
#include <condition_variable>

// 파일 버퍼 크기 (4MB)
#define BUFFER_SIZE 4194304

#include "skiplist.h"

using namespace std;

pthread_t *threads;
pthread_mutex_t readFile = PTHREAD_MUTEX_INITIALIZER, print = PTHREAD_MUTEX_INITIALIZER;

// minkey = 0, maxkey = INT_MAX
skiplist<int> list(0,INT_MAX);

// 총 명령어 수
int count = 0;

// 스레드 동작 상태
enum thvar {READY, EXECUTE, EXIT};
int threadVar = READY;


pthread_barrier_t barrier;


/*
 * Q 구조체:
 *  - action: 'i'일 때 insert, 'q'일 때 query, 'p'일 때 print
 *  - value: insert/query value
 *  - count: insert 작업에 대한 순서 번호
 */
struct Q {
    char action;
    long value; 
    int count;

    Q() : action(0), value(0), count(0) {}
    Q(char action, long value, int count) : action(action), value(value), count(count) {}
};

// 스레드별 작업을 저장하는 pool
vector<vector<Q>> pool;

// 'p' 작업(print)을 실행할 시점(인덱스)를 저장
vector<int> pLoc;
int pli = 0;    // pLoc index 관리 변수
int plmt = -1;  // 현재 작업 인덱스 상한 값 (각 스레드가 작업할 작업 범위 결정)
int numThread;  // 생성하는 스레드 수

void* worker(void* arg) {
    int tid = *((int*) arg); // 스레드 식별자

    // 검색(query) 작업의 인덱스를 보관하는 큐
    queue<int> qq;

    int i = 0;
    int len = pool[tid].size();
    while(1) {
        // 각 스레드가 처리할 작업 개수를 계산 (출력 작업 인덱스에 따라)
        // 'p'가 나오기 전까지 insert/query를 진행
        int mlen = (plmt + (numThread - tid)) / numThread;
        for(; i < mlen; ++i) {
            Q& query = pool[tid][i];

            if(query.action == 'i') {
                list.insert(query.value, query.count);
            }
            else if(query.action == 'q') {
                qq.push(i);
            }
            
            // qq에 저장된 검색 작업들을 일정 간격(100 작업 내)마다 처리
            while(!qq.empty() && qq.front() < i+100) {
                auto& t = pool[tid][qq.front()];
                long ret = list.query(t.value, t.count);
                
                // 원하는 Q까지 insert가 되지 않음. 나중에 다시 시도
                if(ret == -2) break;
                
                // 출력 시 다른 스레드와의 출력 충돌을 피하기 위해 뮤텍스 사용
                pthread_mutex_lock(&print);

                // query이전에 query value가 skiplist에 insert된 적이 없음.
                if(ret == -1) {
                    cout << "ERROR: Not Found: " << t.value << endl;
                }

                // query 현재 value와 바로 다음 value를 출력
                else {
                    cout << t.value << ' ' << ret << endl;
                }
                pthread_mutex_unlock(&print);

                qq.pop();
            }

            //cout << query.first << ' ' << query.second << endl;
        }

        // for 루프 후, 큐에 남은 검색 작업을 처리
        while(!qq.empty()) {
            auto& t = pool[tid][qq.front()];
            long ret = list.query(t.value, t.count);

            if(ret == -2) continue;

            pthread_mutex_lock(&print);
            if(ret == -1) {
                cout << "ERROR: Not Found: " << t.value << endl;
            }
            else {
                cout << t.value << ' ' << ret << endl;
            }
            pthread_mutex_unlock(&print);

            qq.pop();
        }

        // p 처리할 차례. 모든 스레드가 p 전까지 모든 작업을 마칠 때까지 동기화
        pthread_barrier_wait(&barrier);

        // tid 0이 대표로 전역 상태를 업데이트
        if(tid == 0) {
            // 마지막 p인 경우. 즉, 모든 커맨드가 끝난 경우 EXIT으로 바꿔서 worker 종료
            if(pli == pLoc.size()-1) {
                threadVar = EXIT;
                plmt = count;
            }

            // 그렇지 않으면 현재 skiplist 상태 출력
            else {
                plmt = pLoc[++pli];
                cout << list.printList() << endl;
            }
        }

        pthread_barrier_wait(&barrier);

        // EXIT 신호가 오면 스레드 종료
        if(threadVar == EXIT) {
            return NULL;
        }
    }

    return NULL;
}

int main(int argc, char* argv[])
{
    struct timespec start, stop; // 시간 체크

    // 인자 검사: 실행 파일, 입력 파일, 스레드 수
    if (argc != 3) {
        printf("Usage: %s <infile> <numThread>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *fn = argv[1];

    numThread = atoi(argv[2]);

    // 스레드 할당
    threads = new pthread_t[numThread];
    // 스레드 베리어 초기화
    pthread_barrier_init(&barrier, NULL, numThread);
    // 작업 풀 크기 조정
    pool.resize(numThread);

    for(int i = 0; i < numThread; ++i) {
        pool[i].reserve(262144); // 미리 메모리 할당당
    }

    clock_gettime( CLOCK_REALTIME, &start);

    // load input file
    FILE* file = fopen(argv[1], "rb");
    if(!file) {
        cerr << "File does not exist: " << argv[1] << endl;
        return 1;
    }

    char* buffer = new char[BUFFER_SIZE];
    size_t bytes_read;
    char action;
    long value;
    int insertNum = 0;  // insert 작업 순서 번호
    string partial;     // 버퍼 단위로 읽을 때 남은 불완전한 라인 저장
    size_t t = 0;       // 작업을 스레드에 round-robin 방식으로 분배하기 위한 인덱스

    while((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        size_t start = 0;
        for(size_t i = 0; i < bytes_read; ++i) {
            if(buffer[i] != '\n')
                continue;
            
            /* buffer에서 한 줄이 끝난 경우 => 하나의 명령어를 입력받은 경우*/

            
            size_t length = i - start;

            // 하나의 명령어 string으로 저장
            string line;

            // 이전 버퍼에서 불완전한 라인을 합침
            if(!partial.empty()) {
                line = partial + string(buffer+start, length);
                partial.clear();
            }
            else {
                line = string(buffer+start, length);
            }
            start = i+1;

            // 명령어 Parsing
            istringstream iss(line);
            if (!(iss >> action >> value)) {
                // p를 입력받은 경우
                if(action == 'p') {
                    pLoc.push_back(count);
                    ++count;
                }
                // 잘못된 형식의 명령어
                else {
                    printf("ERROR: Unrecognized action: '%c'\n", action);
                }
                continue;
            }
            
            // 잘못된 형식의 명령어
            if(action != 'i' && action != 'q') {
                printf("ERROR: Unrecognized action: '%c'\n", action);
                continue;
            }

            Q q_item(action, value, insertNum);
            pool[t].push_back(move(q_item));

            // insert인 경우 insert Num 증가시킴
            insertNum += (action == 'i');
            ++count;

            // 작업을 스레드에 분배 (round-robin)
            if(++t >= numThread) {
                t = 0;
            }
        }
        
        // 버퍼 끝에 도달했는데 라인이 완성되지 않은 경우 partial에 저장
        if (start < bytes_read) {
            partial += string(buffer + start, bytes_read - start);
        }
    }

    fclose(file);

    // 남은 partial 문자열 처리
    if (!partial.empty()) {
        istringstream iss(partial);
        if (iss >> action >> value) {
            // 잘못된 형식의 명령어
            if(action != 'i' && action != 'q') {
                printf("ERROR: Unrecognized action: '%c'\n", action);
            }
            else {
                Q q_item(action, value, insertNum);
                insertNum += (action == 'i');
                ++count;
                pool[t].push_back(move(q_item));
                if(++t >= numThread) {
                    t = 0;
                }
            }
        } else {
            if(action == 'p') {
                pLoc.push_back(count);
            }
            else {
                printf("ERROR: Unrecognized action: '%c'\n", action);
            }
        }
    }

    // 마지막 작업 위치 저장 (출력 작업 위치)
    pLoc.push_back(count);
    // 초기 plmt는 첫 번째 출력 작업 위치
    plmt = pLoc[0];

    
    list.expectNodeNum(insertNum);
    for (int i = 0; i < numThread; i++) {
        int* arg = new int(i);
        int rc = pthread_create(&threads[i], NULL, worker, (void*) arg);
        if (rc) {
            cout << "Error: unable to create thread," << rc << endl;
            exit(-1);
        }
    }


    for (int i = 0; i < numThread; i++) {
        int rc = pthread_join(threads[i], NULL);
        if (rc) {
            cout << "Error: unable to join thread," << rc << endl;
            delete[] buffer;
            delete[] threads;
            exit(-1);
        }
    }

    //cout << s[0] << endl;

    //cout << ss[0] << endl;
    //cout << s[0] << ' ' << ss[0] << endl;
    //cout << "Query Hit: " << double(s[0]) / ss[0] << endl;

    delete[] buffer;
    delete[] threads;
    //cout << list.printList() << endl;

    clock_gettime( CLOCK_REALTIME, &stop);

    // print results
    double elapsed_time = (stop.tv_sec - start.tv_sec) + ((double) (stop.tv_nsec - start.tv_nsec))/BILLION ;

    cerr << "Elapsed time: " << elapsed_time << " sec" << endl;
    cerr << "Throughput: " << (double) count / elapsed_time << " ops (operations/sec)" << endl;

    return (EXIT_SUCCESS);
}

