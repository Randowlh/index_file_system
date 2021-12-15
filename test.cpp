#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define BLOCK_SIZE 2048
FILE *cur=NULL;
char BUF[BLOCK_SIZE];
void writeblock(int pos,void *buf){
    fseek(cur, pos*BLOCK_SIZE/8, SEEK_SET);
    fwrite(buf, BLOCK_SIZE/8, 1, cur);
}
int main(){
    cur=fopen("test.bin","rb+");
    for(int i=0;i<BLOCK_SIZE;i++)
        BUF[i]=1;
    writeblock(0,BUF);
    fclose(cur);
}