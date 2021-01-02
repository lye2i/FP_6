#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "person.h"

int num=1;

void readPage(FILE *fp, char *pagebuf, int pagenum)
{
	fseek(fp, PAGE_SIZE*pagenum, SEEK_SET);
	fread((void*)pagebuf, PAGE_SIZE, 1, fp);
}

void writePage(FILE *fp, const char *pagebuf, int pagenum)
{
	fseek(fp, PAGE_SIZE*pagenum, SEEK_SET);
	fwrite((void*)pagebuf, PAGE_SIZE, 1, fp);
}

//
// 주어진 레코드 파일에서 레코드를 읽어 heap을 만들어 나간다. Heap은 배열을 이용하여 저장되며,
// heap의 생성은 Chap8에서 제시한 알고리즘을 따른다. 레코드를 읽을 때 페이지 단위를 사용한다는 것에 주의해야 한다.
//
void buildHeap(FILE *inputfp, char **heaparray)
{
	char *pagebuf = (char*)malloc(PAGE_SIZE);
	char *tmp = (char*)malloc(RECORD_SIZE); //sort할 때 record 임시저장할 배열
	int fsize;

	memset(pagebuf, 0xFF, PAGE_SIZE);
	memset(tmp, 0xFF, RECORD_SIZE);

	fseek(inputfp, 0, SEEK_END); //파일의 크기 구하기
	fsize = ftell(inputfp);
	fseek(inputfp, 0, SEEK_SET); //다시 파일의 처음으로 이동

	for(int i=1; i<(fsize/PAGE_SIZE); i++){
		readPage(inputfp, pagebuf, i); //각각의 페이지 가져오기

		for(int j=0; (PAGE_SIZE-(j*RECORD_SIZE))>RECORD_SIZE; j++){
			char *c = (char*)malloc(sizeof(char));
			memcpy(c, pagebuf+(j*RECORD_SIZE), 1);

			if(strcmp(c, "0")!=0&&atoi(c)==0){ //비어있는 레코드라면 건너뛰기
				free(c);
				continue;}

			free(c);
			
			memcpy(heaparray[num++],pagebuf+(j*RECORD_SIZE), RECORD_SIZE); //각각의 페이지의 레코드 heap배열에 추가
			int heap = num-1;
			while((heap/2)!=0){ //parent를 비교하여 정렬
				int parent = heap/2;
				char sn_buf[RECORD_SIZE] = {0,};
				char parent_buf[RECORD_SIZE] = {0,};
				strcpy(sn_buf, heaparray[heap]);
				strcpy(parent_buf, heaparray[parent]);
				char *sn_ptr = strtok(sn_buf, "#");
				char *parent_ptr = strtok(parent_buf, "#");
				char sn[14]={0,};
				char parent_sn[14]={0,};
				sprintf(sn, "%s", sn_ptr);
				sprintf(parent_sn, "%s", parent_ptr);	
				if(atof(parent_sn)>atof(sn)){ //parent가 더 큰 경우 정렬
					memcpy(tmp, heaparray[parent], RECORD_SIZE);
					memcpy(heaparray[parent], heaparray[heap], RECORD_SIZE);
					memcpy(heaparray[heap], tmp, RECORD_SIZE);
					memset(tmp, 0XFF, RECORD_SIZE);
				}

				heap = heap/2;
			}
		}
	}

	free(pagebuf);
	free(tmp);
}

//
// 완성한 heap을 이용하여 주민번호를 기준으로 오름차순으로 레코드를 정렬하여 새로운 레코드 파일에 저장한다.
// Heap을 이용한 정렬은 Chap9에서 제시한 알고리즘을 이용한다.
// 레코드를 순서대로 저장할 때도 페이지 단위를 사용한다.
//
void makeSortedFile(FILE *outputfp, char **heaparray)
{
	char *pagebuf = (char*)malloc(PAGE_SIZE);
	char *tmp = (char*)malloc(RECORD_SIZE);
	int page_num = 1;
	int record_num = 0;
	memset(pagebuf, 0xFF, PAGE_SIZE);
	
	while(num!=1){
		memcpy(pagebuf+(RECORD_SIZE*record_num++), heaparray[1], RECORD_SIZE);
		
		if(num==2) //root노드만 있는 경우
			writePage(outputfp, pagebuf, page_num++);

		else if((PAGE_SIZE-(RECORD_SIZE*record_num))<RECORD_SIZE){ //페이지가 다 찬 경우
			writePage(outputfp, pagebuf, page_num++); //페이지 쓰기
			memset(pagebuf, 0xFF, PAGE_SIZE); //페이지 버퍼 초기화
			record_num = 0; //레코드 번호 초기화
		}

		int sort_num = 1;
		memcpy(heaparray[1], heaparray[num-1], RECORD_SIZE); //마지막 key값 루트 노드로 이동
		while((2*sort_num<num)&&(2*sort_num+1<num)){ //자식 노드가 두개 있을 때 부모 key값과 비교하면서 정렬하기
			char child1_buf[RECORD_SIZE]={0,};
			char child2_buf[RECORD_SIZE]={0,};
			char parent_buf[RECORD_SIZE]={0,};
			strcpy(child1_buf, heaparray[2*sort_num]);
			strcpy(child2_buf, heaparray[2*sort_num+1]);
			strcpy(parent_buf, heaparray[sort_num]);
			
			char *child1_ptr = strtok(child1_buf, "#");
			char *child2_ptr = strtok(child2_buf, "#");
			char *parent_ptr = strtok(parent_buf, "#");
			char child1[14] = {0,};
			char child2[14] = {0,};
			char parent[14] = {0,};
			sprintf(child1, "%s", child1_ptr);
			sprintf(child2, "%s", child2_ptr);
			sprintf(parent, "%s", parent_ptr);

			if(atof(child1)<atof(child2)){
				if(atof(parent)>atof(child1)){ //부모 key와 비교했을 때 더 작다면 
					memcpy(tmp, heaparray[sort_num], RECORD_SIZE);
					memcpy(heaparray[sort_num], heaparray[2*sort_num], RECORD_SIZE);
					memcpy(heaparray[2*sort_num], tmp, RECORD_SIZE);
					memset(tmp, 0XFF, RECORD_SIZE);
					sort_num = 2*sort_num;
				}

				else
					sort_num = 2*sort_num;
			}

			else{
				if(atof(parent)>atof(child2)){ //부모 key와 비교했을 때 더 작다면
					memcpy(tmp, heaparray[sort_num], RECORD_SIZE);
					memcpy(heaparray[sort_num], heaparray[2*sort_num+1], RECORD_SIZE);
					memcpy(heaparray[2*sort_num+1], tmp, RECORD_SIZE);
					memset(tmp, 0XFF, RECORD_SIZE);
					sort_num = 2*sort_num+1;
				}

				else
					sort_num = 2*sort_num +1;
			}
		}

		if((2*sort_num)<num){ //자식노드가 하나만 있을 때 부모 key와 비교하면서 정렬하기
			char child_buf[RECORD_SIZE] = {0,};
			char parent_buf[RECORD_SIZE] = {0,};
			strcpy(child_buf, heaparray[2*sort_num]);
			strcpy(parent_buf, heaparray[sort_num]);
			char *child_ptr = strtok(child_buf, "#");
			char *parent_ptr = strtok(parent_buf, "#");
			char child[14] = {0,};
			char parent[14] = {0,};
			sprintf(child, "%s", child_ptr);
			sprintf(parent,"%s", parent_ptr);

			if(atof(child)<atof(parent)){
				memcpy(tmp, heaparray[sort_num], RECORD_SIZE);
				memcpy(heaparray[sort_num], heaparray[2*sort_num], RECORD_SIZE);
				memcpy(heaparray[2*sort_num], tmp, RECORD_SIZE);
				memset(tmp, 0xFF, RECORD_SIZE);
			}
		}
		num--; //pagebuf에 쓰기
	}
	free(tmp);
	free(pagebuf);
}

int main(int argc, char *argv[])
{
	FILE *inputfp;	// 입력 레코드 파일의 파일 포인터
	FILE *outputfp;	// 정렬된 레코드 파일의 파일 포인터
	char **heaparray;
	char *header = (char*)malloc(PAGE_SIZE);
	int fsize;

	memset(header, 0xFF, PAGE_SIZE);

	if(argc!=4){ //인자갯수 확인
		fprintf(stderr, "usage: %s s <input record file name> <output record file name>\n", argv[0]);
		exit(1);
	}

	/*input 파일 오픈*/
	inputfp = fopen(argv[2], "r");
	fseek(inputfp, 0, SEEK_END);
	fsize = ftell(inputfp);
	fseek(inputfp, 0, SEEK_SET); //다시 파일의 처음으로 이동

	heaparray = (char **)malloc((fsize/RECORD_SIZE)*sizeof(char*)); //동적할당하기
	for(int i=0; i<fsize/RECORD_SIZE; i++){
		heaparray[i] = (char*)malloc(RECORD_SIZE);
		memset(heaparray[i], 0XFF, RECORD_SIZE);
	}

	outputfp = fopen(argv[3], "w"); //output파일 오픈

	readPage(inputfp, header, 0); //헤더페이지 구하기

	buildHeap(inputfp, heaparray); //heap sort하기

	writePage(outputfp, header, 0); //헤더페이지 쓰기

	makeSortedFile(outputfp, heaparray); //정렬된 record 복사

	for(int i=0; i<fsize/RECORD_SIZE; i++){ //메모리 할당 해제
		free(heaparray[i]);
	}

	return 1;
}
