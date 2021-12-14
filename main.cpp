// #pragma warning (disable:4996)
// #pragma warning (disable:6031)
#include<bits/stdc++.h>
using namespace std;
#include <stdio.h>
#include<stdlib.h>
#include <string.h>
#define BLOCK_SIZE 2048
#define NAME_LEN 240
#define SYSTEM_SIZE 67108864/1024 //64K
FILE *cur=NULL;
char BUF[BLOCK_SIZE*2048];
struct index_node {      //total:256bit
    char name[NAME_LEN];      //16*8bit
    int bro, son, fa;   //32*3bit
    int is_file;           //32bit
    index_node(){bro=0,son=0,fa=0,is_file=0;}
}now;
int cur_pos=0;
int tail_pos,trash_top;
struct file_node{
    char name[NAME_LEN-11*4];
    int to[15];
};
// unordered_map<int,index_node>  index_cache;
struct link_table{
    int to[64];
};
struct trash_stack{
    int nxt;
    char blk[BLOCK_SIZE-4];
    trash_stack(){nxt=0;}
};
struct superblock{
    int index_root;
    int trash_top;
    int tail_pos;
    char free_space[BLOCK_SIZE-4*3];
};
void* getblock(FILE *cur,int pos){
    fseek(cur, pos*BLOCK_SIZE, SEEK_SET);
    char *buf = (char*)malloc(BLOCK_SIZE);
    fread(buf, BLOCK_SIZE, 1, cur);
    return buf;
}
void writeblock(FILE *cur,int pos,void *buf){
    fseek(cur, pos*BLOCK_SIZE, SEEK_SET);
    fwrite(buf, BLOCK_SIZE, 1, cur);
}
int get_new_block(){
    if(trash_top==0) return tail_pos++;
    else{
        trash_stack st=*(trash_stack*)getblock(cur,trash_top);
        int ans=trash_top;
        trash_top=st.nxt;
        return ans;
    }
    if(tail_pos==SYSTEM_SIZE/4) return -1;
}
void free_block(int pos){
    trash_stack st;
    st.nxt=trash_top;
    trash_top=pos;
    writeblock(cur,pos,&st);
}
void format_file(FILE *cur){
    fseek(cur,0,SEEK_SET);
    for(int i=0;i<SYSTEM_SIZE/4;i++){
        int now=0;
        fwrite(&now,4,1,cur);
    }
    superblock sb;
    sb.index_root=1;
    sb.trash_top=0;
    sb.tail_pos=2;
    writeblock(cur,0,&sb);
    index_node root;
    strcpy(root.name,"/");
    writeblock(cur,1,&root);
}
void init_file(){
    FILE *f = fopen("ext2fs.dump", "r");
    if(f == NULL){
        FILE *f =fopen("ext2fs.dump", "w");
        format_file(f);
        fclose(f);
    }else fclose(f);
}
int get_from_link_table(int pos,int step,int tail){
    link_table lt=*(link_table*)getblock(cur,pos);
    if(step==0){
        for(int i=0;i<64;i++){
            if(lt.to[i]!=0){
                char* str=(char*)getblock(cur,lt.to[i]);
                for(int i=0;i<BLOCK_SIZE/8;i++)
                    BUF[tail++]=str[i];
            }
        }
    }
    else{
        for(int i=0;i<64;i++){
            if(lt.to[i]!=0)
                tail=get_from_link_table(lt.to[i],step-1,tail);
        }
    }
    return tail;
}
int readfile(int pos){
    int tail=0;
    file_node root=*(file_node*)getblock(cur,pos);
    for(int i=0;i<=11;i++){
        if(root.to[i]!=0){
            char* str=(char*)getblock(cur,root.to[i]);
            for(int i=0;i<BLOCK_SIZE/8;i++)
                BUF[tail++]=str[i];
        }
    }
    if(root.to[12]!=0)
        tail=get_from_link_table(root.to[11],0,tail);
    if(root.to[13]!=0)
        tail=get_from_link_table(root.to[12],1,tail);
    if(root.to[14]!=0)
        tail=get_from_link_table(root.to[13],2,tail);
    return tail;
}
void erase_the_link_table(int pos,int step){
    if(step==0){
        link_table lt=*(link_table*)getblock(cur,pos);
        for(int i=0;i<64;i++){
            if(lt.to[i]!=0){
                free_block(lt.to[i]);
                lt.to[i]=0;
            }
        }
        writeblock(cur,pos,&lt);
    }
    else{
        link_table lt=*(link_table*)getblock(cur,pos);
        for(int i=0;i<64;i++){
            if(lt.to[i]!=0)
                erase_the_link_table(lt.to[i],step-1);
        }
    }
}
void erase_file(int pos){
    file_node root=*(file_node*)getblock(cur,pos);
    for(int i=0;i<=11;i++){
        if(root.to[i]!=0)
            erase_the_link_table(root.to[i],0);
    }
    if(root.to[12]!=0)
        erase_the_link_table(root.to[11],1);
    if(root.to[13]!=0)
        erase_the_link_table(root.to[12],2);
    if(root.to[14]!=0)
        erase_the_link_table(root.to[13],3);
}
int write_to_link_table(int pos,int tail,int now,int step){
    if(step==0){
    }
}
int writefile(int pos,int tail){
    erase_file(pos);
}

void init(FILE *cur){
    now=*(index_node*)getblock(cur,1);
    cur_pos=1;
    superblock sb=*(superblock*)getblock(cur,0);
    tail_pos=sb.tail_pos;
    trash_top=sb.trash_top;
}
void mkdir(char name[]){
    index_node new_node;
    strcpy(new_node.name,name);
    new_node.is_file=0;
    new_node.bro=now.son;
    new_node.fa=cur_pos;
}
void ls(){
    printf("..\n");
    printf(".\n");
    int pos=now.son;
    while(pos!=0){
        index_node now=*(index_node*)getblock(cur,pos);
        printf("%s\n",now.name);
        pos=now.bro;
    }
}
void cd(char name[]){
    if(strcmp(name,"..")==0){
        if(cur_pos==1) return;
        cur_pos=now.fa;
        now=*(index_node*)getblock(cur,cur_pos);
        return;
    }
    if(strcmp(name,".")==0) return;
    int pos=now.son;
    while(pos!=0){
        index_node now=*(index_node*)getblock(cur,pos);
        if(strcmp(now.name,name)==0){
            cur_pos=pos;
            return;
        }
        pos=now.bro;
    }
    printf("no such file or directory\n");
}
void pwd(){
    int pos=cur_pos;
    char path[NAME_LEN];
    strcpy(path,"/");
    while(pos!=1){
        index_node now=*(index_node*)getblock(cur,pos);
        strcat(path,now.name);
        strcat(path,"/");
        pos=now.fa;
    }
    printf("%s\n",path);
}
void touch(char name[]){
    index_node new_node;
    strcpy(new_node.name,name);
    new_node.is_file=1;
    new_node.bro=now.son;
    new_node.fa=cur_pos;
    now.son=get_new_block();
    writeblock(cur,cur_pos,&now);
    writeblock(cur,now.son,&new_node);
}
void rm(char name[]){
}
void cat(char name[]){
    int pos=now.son;
    while(pos!=0){
        index_node now=*(index_node*)getblock(cur,pos);
        if(strcmp(now.name,name)==0){
            if(now.is_file==0){
                printf("cat: %s: Is a directory\n",name);
                return;
            }
            int pos=now.bro;
            int tail=readfile(pos);
            for(int i=0;i<tail;i++)
                printf("%c",BUF[i]);
            printf("\n");
            return;
        }
        pos=now.bro;
    }
    printf("no such file or directory\n");
}
void write(char name[]){
    int tail=0;
    printf("Control-D to finish writing\n");
    char a;
    while((a=getchar())!=EOF)
        BUF[tail++]=a;
    BUF[tail++]='\0';

}
void main_loop(){
    printf("randow_pc:$");
    char cmd[100];
    scanf("%s",cmd);
    if(strcmp(cmd,"mkdir")==0){
        char name[NAME_LEN];
        scanf("%s",name);
        mkdir(name);
    }else if(strcmp(cmd,"cd")==0){
        char name[NAME_LEN];
        scanf("%s",name);
        cd(name);
    }else if(strcmp(cmd,"ls")==0){
        ls();
    }else if(strcmp(cmd,"rm")==0){
        char name[NAME_LEN];
        scanf("%s",name);
        rm(name);
    }else if(strcmp(cmd,"touch")==0){
        char name[NAME_LEN];
        scanf("%s",name);
        touch(name);
    }else if(strcmp(cmd,"cat")==0){
        char name[NAME_LEN];
        scanf("%s",name);
        cat(name);
    }else if(strcmp(cmd,"write")==0){
        char name[NAME_LEN];
        scanf("%s",name);
        write(name);
    }else  if(strcmp(cmd,"exit")==0){
        exit(0);
    }else{
        printf("error command\n");
    }
}
int main(int argc, char* argv[]) {
    init_file();
    FILE *cur=fopen("ext2fs.dump", "rb+");
    init(cur);
    while(1)
        main_loop();
    fclose(cur);
    return 0;
}