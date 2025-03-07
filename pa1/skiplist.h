#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>

#define BILLION  1000000000L

using namespace std;

pthread_mutexattr_t attr;

//template<class V, int MAXLEVEL>

template<class V, int MAXLEVEL = 16>
class skiplist
{
protected:
    int randomLevel(int count) {
        // 시간 단축을 위해 count의 trailing zero를 이용하여 skiplist 레벨 설정
        int level = __builtin_ctz(count);
        return level == 0 ? 1 : min(level, MAXLEVEL-1);
    }

    struct node {
        long value;                 // 노드가 저장하는 값
        int forwards[MAXLEVEL+1];   // 각 레벨의 후속 노드를 가리키는 인덱스
        pthread_mutex_t mlock;      // 동시성 제어를 위한 뮤텍스
        bool fullyLinked = false;   // 노드가 모든 레벨에 완전히 연결되었는지 여부

        // 생성자: mlock 초기화
        node() {
            pthread_mutex_init(&mlock, &attr);
        }
        
        void lock() {
            pthread_mutex_lock(&mlock);
        }

        void unlock() {
            pthread_mutex_unlock(&mlock);
        }
    };

    V m_minKey;             // header node. 선언 시 초기화(0)
    V m_maxKey;             // tail node. 선언 시 초기화(INT_MAX)
    int max_curr_level;     // 현재 skip list의 최대 level. 선언 시 초기화(1)

    vector<node> nodePool;  // 노드 풀
    int pHeader;            // header node index. 선언 시 초기화(-1), expectNodeNum시 초기화
    int pTail;              // tail node index

    int insertCompl = 0;    // 완료된 insert 작업 수
    // 삽입 완료 상태 업데이트용 뮤텍스
    pthread_mutex_t instlock = PTHREAD_MUTEX_INITIALIZER;

    // 높은 레벨에서 삽입 완료 상태를 갱신하기 위한 함수
    void rr(int count) {
        if(pthread_mutex_trylock(&instlock) == 0) {
            while(insertCompl < pHeader && nodePool[insertCompl].fullyLinked) ++insertCompl;
            pthread_mutex_unlock(&instlock);
        }
    }

public:
    skiplist(V minKey, V maxKey):pHeader(-1),pTail(-1),
                                max_curr_level(1),
                                m_minKey(minKey),m_maxKey(maxKey)
    {

    }

    // 예상 insert 개수를 전달받아 노드 풀을 초기화하고, 헤더 및 꼬리 노드를 설정
    void expectNodeNum(size_t insertNum) {
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        nodePool.resize(insertNum+2);
        /* index 설정
         * 0 to insertNum-1: 삽입되는 value의 위치
         * insertNum: header Node
         * insertNum+1: tail Node
         */
        pHeader = insertNum;  // header node index 설정
        pTail = insertNum+1;  // tail node index 설정

        // header/tail node의 값 설정
        nodePool[pHeader].value = m_minKey;
        nodePool[pTail].value = m_maxKey;

        for ( int i=1; i<=MAXLEVEL; i++ ) {
            nodePool[pHeader].forwards[i] = pTail;
        }
    }

    virtual ~skiplist()
    {

    }

    /*
     * searchvalue를 찾고,
     * 각 레벨별로 선행 노드(preds)와 후행 노드(succs)를 기록.
     * 값을 찾으면 해당 노드의 인덱스를, 없으면 -1을 반환
     */
    int find(V searchvalue, int* preds, int* succs) {
        int lFound = -1;

        int pred = pHeader;
        
        for(int level = MAXLEVEL; level >= 1; --level) {
            int curr = nodePool[pred].forwards[level];
            while(searchvalue > nodePool[curr].value) {
                pred = curr; curr = nodePool[curr].forwards[level];
            }
            if(lFound == -1 && searchvalue == nodePool[curr].value) {
                lFound = curr;
            }
            preds[level] = pred; // 해당 레벨의 선행 노드 기록
            succs[level] = curr; // 해당 레벨의 후행 노드 기록
        }

        return lFound;
    }

    /*
     * insert: value를 skiplist에 insert
     * 만약 value가 이미 존재한다면 insert하지 않음. (false 반환)
     * count는 skiplist의 새 노드의 index로 사용됨.
     */
    bool insert(V value, int count) {
        int topLevel = randomLevel(count); // level 결정
        int preds[MAXLEVEL+1];
        int succs[MAXLEVEL+1];

        while(1) {
            // 값 find. preds와 succs 
            int lFound = find(value, preds, succs);

            // 동일 값이 존재하며 이미 삽입된 경우 삽입하지 않음
            if(lFound != -1 && lFound < count) {
                return false;
            }

            int highestLocked = -1;
            int pred = -1;
            int succ = -1;
            bool valid = true;
            
            // 새 노드를 삽입하기 위해 각 레벨의 선행 노드를 잠금
            for(int level = 1; valid && (level <= topLevel); ++level) {
                pred = preds[level];
                succ = succs[level];
                nodePool[pred].lock();
                highestLocked = level;

                // 다른 스레드에 의한 구조 변경 여부 체크
                valid = nodePool[pred].forwards[level] == succ;
            }
            

            if(!valid) {
                // 잠금 해제 후 재시도
                for(int level = 1; level <= highestLocked; ++level) {
                    nodePool[preds[level]].unlock();
                }
                continue;
            }

            int newNode = count; // 새 노드의 인덱스 지정
            nodePool[newNode].value = value;


            // 새 노드의 후행 노드 설정
            for(int level = 1; level <= topLevel; ++level) {
                nodePool[newNode].forwards[level] = succs[level];
            }

            //새 노드의 선행 노드 설정
            for(int level = 1; level <= topLevel; ++level) {
                nodePool[preds[level]].forwards[level] = newNode;
            }
            nodePool[newNode].fullyLinked = true; // 새 노드 삽입 완료 표시

            // 모든 레벨에 대해 잠금 해제
            for(int level = 1; level <= highestLocked; ++level) {
                nodePool[preds[level]].unlock();
            }
            
            // 각 스레드별로 1/16 확률로 현재 몇 번째 insert까지 진행했는지 갱신
            if(topLevel > 2) rr(count);
            return true;
        }
    }

    /*
     * query: value 탐색
     * 만약 아직 삽입 작업이 완료되지 않았으면 -2를, 검색 실패 시 -1을 반환
     * 성공 시에는 찾은 값에 연결된 후속 노드의 값을 반환
     */
    long query(V value, int count) {
        
        long ret = -1;
        pthread_mutex_lock(&instlock);

        // rr()과 같은 작업으로, 현재 몇 번째 명령어까지 insert됐는지 확인
        while(insertCompl < pHeader && nodePool[insertCompl].fullyLinked) ++insertCompl;

        long t = insertCompl;
        // 현재 count가 삽입 완료 상태보다 크다는 것은, query 시점의 insert가 아직 완료되지 않았다는 의미.
        if(count > insertCompl) {
            pthread_mutex_unlock(&instlock);
            return -2L;
        }

        pthread_mutex_unlock(&instlock);

        // value 탐색. insert 도중에 pointer가 이상한데 가르키거나, query가 skiplist구조를 바꾸지 않으므로 lock이 필요 없음.
        int curr = pHeader;
        for(int level = MAXLEVEL; level >=1; level--) {
            while ( nodePool[nodePool[curr].forwards[level]].value < value ){
                curr = nodePool[curr].forwards[level];
            }
        }

        curr=nodePool[curr].forwards[1];

        // 인덱스 범위 검사 및 값 일치 여부 확인
        if(curr >= count || value != nodePool[curr].value) return -1;

        curr=nodePool[curr].forwards[1];

        // 조건에 맞는 후속 노드를 찾기 위해 포인터 이동
        // query value가 만약 마지막 값이라면 그냥 pTail을 반환하도록 함
        while(curr != pTail && curr >= count) {
            curr=nodePool[curr].forwards[1];
        }

        return nodePool[curr].value;
    }

    // printList 함수: skiplist의 1레벨 링크를 따라 최대 200개 노드의 값을 문자열로 반환
    std::string printList()
    {
            int i=0;
        std::stringstream sstr;
        int currNode = nodePool[pHeader].forwards[1];

        while ( currNode != pTail ) {
            sstr << nodePool[currNode].value << " ";
            currNode = nodePool[currNode].forwards[1];
                i++;
            if(i>200) break;
        }
        return sstr.str();
    }
};
