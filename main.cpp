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
char BUF[BLOCK_SIZE*100];
char tmp_str[BLOCK_SIZE/8];
int tail,bg;
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
    file_node(){
        for(int i=0;i<15;i++)
            to[i]=0;
    }
};
struct link_table{
    int to[64];
    link_table(){
        for(int i=0;i<64;i++)
            to[i]=0;
    }
};
struct trash_stack{
    int nxt;
    char blk[BLOCK_SIZE/8-4];
    trash_stack(){nxt=0;}
};
struct superblock{
    int index_root;
    int trash_top;
    int tail_pos;
    char free_space[BLOCK_SIZE/8-4*3];
};
void* getblock(FILE *cur,int pos){
    fseek(cur, pos*BLOCK_SIZE/8, SEEK_SET);
    char *buf = (char*)malloc(BLOCK_SIZE/8);
    fread(buf, BLOCK_SIZE/8, 1, cur);
    return buf;
}
void writeblock(int pos,void *buf){
    fseek(cur, pos*BLOCK_SIZE/8, SEEK_SET);
    fwrite(buf, BLOCK_SIZE/8, 1, cur);
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
    writeblock(pos,&st);
}
void format_file(FILE *f){
    cur=f;
    fseek(cur,0,SEEK_SET);
    for(int i=0;i<SYSTEM_SIZE/4;i++){
        int now=0;
        fwrite(&now,4,1,cur);
    }
    superblock sb;
    sb.index_root=1;
    sb.trash_top=0;
    sb.tail_pos=2;
    writeblock(0,&sb);
    index_node root;
    strcpy(root.name,"/");
    writeblock(1,&root);
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
                strcpy(tmp_str,(char*)getblock(cur,lt.to[i]));
                for(int i=0;i<BLOCK_SIZE/8;i++)
                    BUF[tail++]=tmp_str[i];
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
void readfile(int pos){
    tail=0;
    file_node root=*(file_node*)getblock(cur,pos);
    for(int i=0;i<=11;i++){
        if(root.to[i]!=0){
            strcpy(tmp_str,(char*)getblock(cur,root.to[i]));
            for(int i=0;i<BLOCK_SIZE/8;i++)
                BUF[tail++]=tmp_str[i];
        }
    }
    if(root.to[12]!=0)
        tail=get_from_link_table(root.to[12],0,tail);
    if(root.to[13]!=0)
        tail=get_from_link_table(root.to[13],1,tail);
    if(root.to[14]!=0)
        tail=get_from_link_table(root.to[14],2,tail);
    return;
}
void erase_the_link_table(int pos,int step){
    if(step==0){
        link_table lt=*(link_table*)getblock(cur,pos);
        for(int i=0;i<64;i++)
            if(lt.to[i]!=0)
                free_block(lt.to[i]);
    }
    else{
        link_table lt=*(link_table*)getblock(cur,pos);
        for(int i=0;i<64;i++){
            if(lt.to[i]!=0)
                erase_the_link_table(lt.to[i],step-1);
        }
    }
    free_block(pos);
}
void erase_file(int pos){
    file_node root=*(file_node*)getblock(cur,pos);
    for(int i=0;i<=11;i++){
        if(root.to[i]!=0){
            free_block(root.to[i]);
            root.to[i]=0;
        }
    }
    
    if(root.to[12]!=0)
        erase_the_link_table(root.to[12],0);
    if(root.to[13]!=0)
        erase_the_link_table(root.to[13],1);
    if(root.to[14]!=0)
        erase_the_link_table(root.to[14],2);
    free_block(pos);
}
void write_to_link_table(int pos,int step){
    if(step==0){
        link_table lt=*(link_table*)getblock(cur,pos);
        for(int i=0;i<64&&bg<tail;i++){
            lt.to[i]=get_new_block();
            for(int j=0;j<BLOCK_SIZE/8&&bg<tail;j++)
                tmp_str[j]=BUF[bg++];
            writeblock(lt.to[i],tmp_str);
        }
        writeblock(pos,&lt);
    }else{
        link_table lt=*(link_table*)getblock(cur,pos);
        for(int i=0;i<64&&bg<tail;i++){
            lt.to[i]=get_new_block();
            write_to_link_table(lt.to[i],step-1);
        }
        writeblock(pos,&lt);
    }
}
void writefile(int pos){
    BUF[tail++]='\n';
    file_node current=*(file_node*)getblock(cur,pos);
    bg=0;
    for(int i=0;i<=11;i++){
        if(bg!=tail){
            current.to[i]=get_new_block();
            for(int i=0;i<BLOCK_SIZE/8&&bg<tail;i++)
                tmp_str[i]=BUF[bg++];
            writeblock(current.to[i],tmp_str);
            writeblock(pos,&current);
        }
    }
    if(bg<tail){
        current.to[12]=get_new_block();
        write_to_link_table(current.to[12],0);
    }
    if(bg<tail){
        current.to[13]=get_new_block();
        write_to_link_table(current.to[13],1);
    }
    if(bg<tail){
        current.to[14]=get_new_block();
        write_to_link_table(current.to[14],2);
    }
    writeblock(pos,&current);
}
void write(char name[]){
    int pos=now.son;
    int flag=0;
    while(pos!=0){
        index_node tmp=*(index_node*)getblock(cur,pos);
        if(strcmp(name,tmp.name)==0){
            flag=1;
            break;
        };
        pos=tmp.bro;
    }
    index_node tmp=*(index_node*)getblock(cur,pos);
    if(flag==0){
        printf("No such file\n");
        return;
    }
    tail=0;
    printf("enter '@' to finish writing\n");
    char a;
    while((a=getchar())!='@')
        BUF[tail++]=a;
    BUF[tail++]='\0';
    writefile(tmp.son);
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
    new_node.son=0;
    now.son=get_new_block();
    writeblock(cur_pos,&now);
    writeblock(now.son,(void *)&new_node);
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
        if(cur_pos==1)
            return;
        cur_pos=now.fa;
        now=*(index_node*)getblock(cur,cur_pos);
        return;
    }
    if(strcmp(name,".")==0){
        return;
    }
    int pos=now.son;
    while(pos!=0){
        index_node tmp=*(index_node*)getblock(cur,pos);
        if(strcmp(name,tmp.name)==0){
            cur_pos=pos;
            now=tmp;
            return;
        }
        pos=tmp.bro;
    }
    printf("No such file\n");
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
    new_node.son=get_new_block();
    file_node f;
    writeblock(cur_pos,&now);
    writeblock(now.son,&new_node);
    writeblock(new_node.son,&f);
}
void rm_index(int pos){
    if(pos==0)
        return;
    index_node root=*(index_node*)getblock(cur,pos);
    if(root.bro!=0)
        rm_index(root.bro);
    if(root.son!=0){
        if(root.is_file)
            erase_file(root.son);
        else 
            rm_index(root.son);
    }
    free_block(pos);
}
void rm(char name[]){
    int pos=now.son;
    int flag=0;
    int nxt=0;
    while(pos!=0){
        index_node tmp=*(index_node*)getblock(cur,pos);
        if(strcmp(name,tmp.name)==0){
            flag=1;
            nxt=tmp.bro;
            break;
        }
        pos=tmp.bro;
    }
    index_node tmp=*(index_node*)getblock(cur,pos);
    if(flag==0){
        printf("No such file\n");
        return;
    }
    if(now.son==pos){
        now.son=nxt;
        writeblock(cur_pos,&now);
        rm_index(pos);
    }else{
        int index=now.son;
        index_node tmp=*(index_node*)getblock(cur,index);
        while(tmp.bro!=pos){
            index=tmp.bro;
            tmp=*(index_node*)getblock(cur,tmp.bro);
        }
        tmp.bro=nxt;
        writeblock(index,&tmp);
        rm_index(pos);
    }
}
void cat(char name[]){
    int pos=now.son;
    while(pos!=0){
        index_node tmp=*(index_node*)getblock(cur,pos);
        if(strcmp(tmp.name,name)==0){
            if(tmp.is_file==0){
                printf("cat: %s: Is a directory\n",name);
                return;
            }
            int pos=tmp.son;
            // printf("pos=%d\n",pos);
            readfile(pos);
            for(int i=0;i<tail;i++)
                printf("%c",BUF[i]);
            printf("\n");
            return;
        }
        pos=tmp.bro;
    }
    printf("no such file or directory\n");
}
int main_loop(){
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
    }else if(strcmp(cmd,"pwd")==0){
        pwd();
    }
    else if(strcmp(cmd,"write")==0){
        char name[NAME_LEN];
        scanf("%s",name);
        write(name);
    }else  if(strcmp(cmd,"exit")==0){
        return 1;
    }else if(strcmp(cmd,"clear")==0){
        system("clear");
    }
    else{
        printf("error command\n");
    }
    return 0;
}
int main(int argc, char* argv[]) {
    init_file();
    cur=fopen("ext2fs.dump", "rb+");
    init(cur);
    while(1)
        if(main_loop())
            break;
    fclose(cur);
    return 0;
}