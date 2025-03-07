#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define SPARSENESS 5

int main(int argc, char** argv)
{
    int i,r;
    int opt;
    int opt_cnt=0;
    int nqueries=10;
    int wr_proportion=90;
    int hit_proportion=50;
    extern char* optarg;
    int *isUsed;
    int step;


//    while( (opt = getopt(argc, argv, "n:p:h:")) != -1) {
    while( (opt = getopt(argc, argv, "n:h:")) != -1) {
        switch (opt) {
        case 'n': 
		nqueries = atoi(optarg) ; 
		break;
//        case 'p': 
//		wr_proportion = atoi(optarg); 
//		break;
        case 'h': 
		hit_proportion = atoi(optarg); 
		break;
        default:
//            fprintf(stderr, "Usage: %s -n {number of queries}  -p {insert rate (0-100)} -h {search hit rate (0-100)} \nFor example, %s -n100 -p50 -h100\n", argv[0], argv[0]);
            fprintf(stderr, "Usage: %s -n {number of queries}  -h {search hit rate (0-100)} \nFor example, %s -n100 -h100\n", argv[0], argv[0]);
            exit(0);
	}
	opt_cnt++;
    }

    if(opt_cnt<2 || nqueries<10){
            fprintf(stderr, "Usage: %s -n {number of queries (>10)}  -h {search hit rate (0-100)} \nFor example, %s -n100 -h100\n", argv[0], argv[0]);
            exit(0);
    }

    srand(time(NULL));

    isUsed = (int*) malloc(sizeof(int)*SPARSENESS*nqueries);
    for(i=0;i<SPARSENESS*nqueries;i++){
	isUsed[i] = 0;
    }

    step = nqueries/20; 

    //printf("n=%d, p=%d\n", nqueries, wr_proportion);
    for(i=0;i<nqueries;i++){
	    //if(i==nqueries-1) printf("#\n");
	    //if(i%step==0) printf("#");
	    if( i< nqueries*0.8 || (rand()%100) < wr_proportion) {
	    // insert query
		while(1){
			r = rand()%(nqueries*SPARSENESS);
			if(isUsed[r]) continue;
			else {
				isUsed[r] = 1;
				break;
			}
		}

		printf("i %d\n", r);
	    }
	    else{
	    //search 
		while(1){
			if(rand()%100 < hit_proportion) { // 70% success rate for search
				r = rand()%(nqueries*SPARSENESS);
				if(isUsed[r]) {  // hit
					printf("q %d\n", r);
					break;
				}
			}
			else {
				r = rand()%(nqueries*SPARSENESS);
				if(!isUsed[r]) {  // miss
					printf("q %d\n", r);
					break;
				}
			}
		}
	    }
    }

    free(isUsed);

}
