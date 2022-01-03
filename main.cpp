#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BLOCK_SIZE 2048
#define NAME_LEN 240
#define SYSTEM_SIZE 67108864 
FILE *cur=NULL;
void *tt;
char BUF[BLOCK_SIZE*1000];
char tmp_str[BLOCK_SIZE/8];
int tail,bg;
struct index_node {     
    char name[NAME_LEN];      
    int bro, son, fa;   
    int is_file;          
    index_node(){bro=0,son=0,fa=0,is_file=0;memset(name,0,NAME_LEN);}
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
w
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
        tt=getblock(cur,trash_top);
        trash_stack st=*(trash_stack*)tt;
        free(tt);
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
int search_current_index(char name[]){
    int ans=0;
    int pos=now.son;
    while(pos!=0){
        index_node in=*(index_node*)getblock(cur,pos);
        if(strcmp(in.name,name)==0)
            return pos;
        pos=in.bro;
    }
    return -1;
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
void get_from_link_table(int pos,int step){
    tt=getblock(cur,pos);
    link_table lt=*(link_table*)tt;
    free(tt);
    if(step==0){
        for(int i=0;i<64&&lt.to[i]!=0;i++){
            tt=getblock(cur,lt.to[i]);
            char* str=(char*)tt;
            free(tt);
            for(int j=0;j<BLOCK_SIZE/8;j++)
                if(bg!=0)
                    bg--;
                else
                    BUF[tail++]=str[j];
        }
    }
    else{
        for(int i=0;i<64&&lt.to[i]!=0;i++)
            get_from_link_table(lt.to[i],step-1);
    }
    return;
}
void readfile(int pos){
    tail=0;
    tt=getblock(cur,pos);
    file_node root=*(file_node*)tt;
    free(tt);
    for(int i=0;i<=11&&root.to[i]!=0;i++){
        tt=getblock(cur,root.to[i]);
        char * str =(char *)tt;
        free(tt);
        for(int j=0;j<BLOCK_SIZE/8;j++){
            if(bg!=0)
                bg--;
            else
                BUF[tail++]=str[j];
        }
    }
    if(root.to[12]!=0)
        get_from_link_table(root.to[12],0);
    if(root.to[13]!=0)
        get_from_link_table(root.to[13],1);
    if(root.to[14]!=0)
        get_from_link_table(root.to[14],2);
    return;
}
void erase_the_link_table(int pos,int step){
    if(step==0){
        tt=getblock(cur,pos);
        link_table lt=*(link_table*)tt;
        free(tt);
        for(int i=0;i<64;i++)
            if(lt.to[i]!=0)
                free_block(lt.to[i]);
    }
    else{
        tt=getblock(cur,pos);
        link_table lt=*(link_table*)tt;
        free(tt);
        for(int i=0;i<64;i++){
            if(lt.to[i]!=0)
                erase_the_link_table(lt.to[i],step-1);
        }
    }
    free_block(pos);
}
void erase_file(int pos){
    tt=getblock(cur,pos);
    file_node root=*(file_node*)tt;
    free(tt);
    for(int i=0;i<=11;i++)
        if(root.to[i]!=0)
            free_block(root.to[i]);
    if(root.to[12]!=0)
        erase_the_link_table(root.to[12],0);
    if(root.to[13]!=0)
        erase_the_link_table(root.to[13],1);
    if(root.to[14]!=0)
        erase_the_link_table(root.to[14],2);
    for(int i=0;i<15;i++)
        root.to[i]=0;
    writeblock(pos,&root);
}
void write_to_link_table(int pos,int step){
    if(step==0){
        link_table lt;
        for(int i=0;i<64&&bg<tail;i++){
            lt.to[i]=get_new_block();
            for(int j=0;j<BLOCK_SIZE/8&&bg<tail;j++)
                tmp_str[j]=BUF[bg++];
            writeblock(lt.to[i],tmp_str);
        }
        writeblock(pos,&lt);
    }else{
        link_table lt;
        for(int i=0;i<64&&bg<tail;i++){
            lt.to[i]=get_new_block();
            write_to_link_table(lt.to[i],step-1);
        }
        writeblock(pos,&lt);
    }
}
void writefile(int pos){
    file_node current;
    bg=0;
    for(int i=0;i<=11&&bg<tail;i++){
        current.to[i]=get_new_block();
        for(int j=0;j<BLOCK_SIZE/8&&bg<tail;j++)
            tmp_str[j]=BUF[bg++];
        writeblock(current.to[i],tmp_str);
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
void write(char name[],int offset){
    int pos=now.son;
    int flag=0;
    while(pos!=0){
        tt=getblock(cur,pos);
        index_node tmp=*(index_node*)tt;
        free(tt);
        if(strcmp(name,tmp.name)==0){
            if(tmp.is_file==0){
                printf("This is not a file,can't write\n");
                return;
            }
            flag=1;
            break;
        };
        pos=tmp.bro;
    }
    if(flag==0){
        printf("No such file\n");
        return;
    }
    tt=getblock(cur,pos);
    index_node tmp=*(index_node*)tt;
    free(tt);
    tail=0;
    bg=0;
    readfile(tmp.son);
    BUF[tail++]='\0';
    char file_name_str[NAME_LEN+20],cmd[NAME_LEN+20];
    sprintf(file_name_str,"%d%d",rand(),rand());
    strcat(file_name_str,name);
    FILE *fp=fopen(file_name_str,"w");
    fprintf(fp,"%s",BUF+offset);
    fclose(fp);
    strcpy(cmd,"vim ");
    strcat(cmd,file_name_str);
    system(cmd);
    fp=fopen(file_name_str,"r");
    tail=offset;
    char a;
    while((fscanf(fp,"%c",&a))!=EOF)
        BUF[tail++]=a;
    BUF[tail++]='\0';
    fclose(fp);
    memset(cmd,0,NAME_LEN+20);
    strcpy(cmd,"rm -f ");
    strcat(cmd,file_name_str);
    system(cmd);
    erase_file(tmp.son);  //something went wrong???
    writefile(tmp.son);
}
void init(FILE *cur){
    tt=getblock(cur,1);
    now=*(index_node*)tt;
    free(tt);
    cur_pos=1;
    tt=getblock(cur,0);
    superblock sb=*(superblock*)tt;
    free(tt);
    tail_pos=sb.tail_pos;
    trash_top=sb.trash_top;
}
void mkdir(char name[]){
    int pos=search_current_index(name);
    if(pos!=-1){
        printf("Already exist!\n");
        return;
    }
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
    printf(".\n");
    printf("..\n");
    int pos=now.son;
    while(pos!=0){
        tt=getblock(cur,pos);
        index_node now=*(index_node*)tt;
        free(tt);
        printf("%s\n",now.name);
        pos=now.bro;
    }
}
void cd(char name[]){
    if(strcmp(name,"..")==0){
        if(cur_pos==1)
            return;
        cur_pos=now.fa;
        tt=getblock(cur,cur_pos);
        now=*(index_node*)tt;
        free(tt);
        return;
    }
    if(strcmp(name,".")==0){
        return;
    }
    int pos=now.son;
    while(pos!=0){
        tt=getblock(cur,pos);
        index_node tmp=*(index_node*)tt;
        free(tt);
        if(strcmp(name,tmp.name)==0){
            if(tmp.is_file==1){
                printf("This is a file,can't cd\n");
                return;
            }
            cur_pos=pos;
            now=tmp;
            return;
        }
        pos=tmp.bro;
    }
}
void reverse_string(char str[]){
    int i=0,j=strlen(str)-1;
    while(i<j){
        char tmp=str[i];
        str[i]=str[j];
        str[j]=tmp;
        i++;
        j--;
    }
}
char * pwd(){
    int pos=cur_pos;
    char* path=(char *)malloc(sizeof(char)*10000);
    strcpy(path,"/");
    while(pos!=1){
        tt=getblock(cur,pos);
        index_node now=*(index_node*)tt;
        free(tt);
        char tmp[NAME_LEN];
        strcpy(tmp,now.name);
        reverse_string(tmp);
        strcat(path,tmp);
        strcat(path,"/");
        pos=now.fa;
    }
    reverse_string(path);
    return path;
}
void touch(char name[]){
    int pos=search_current_index(name);
    if(pos!=-1){
        printf("Already exist!\n");
        return;
    }
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
    tt=getblock(cur,pos);
    index_node root=*(index_node*)tt;
    free(tt);
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
        tt=getblock(cur,pos);
        index_node tmp=*(index_node*)tt;
        free(tt);
        if(strcmp(name,tmp.name)==0){
            flag=1;
            nxt=tmp.bro;
            break;
        }
        pos=tmp.bro;
    }
    tt=getblock(cur,pos);
    index_node tmp=*(index_node*)tt;
    free(tt);
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
        tt=getblock(cur,index);
        index_node tmp=*(index_node*)tt;
        free(tt);
        while(tmp.bro!=pos){
            index=tmp.bro;
            tmp=*(index_node*)getblock(cur,tmp.bro);
        }
        tmp.bro=nxt;
        writeblock(index,&tmp);
        rm_index(pos);
    }
}
void cat(char name[],int offset){
    int pos=now.son;
    while(pos!=0){
        tt=getblock(cur,pos);
        index_node tmp=*(index_node*)tt;
        free(tt);
        if(strcmp(tmp.name,name)==0){
            if(tmp.is_file==0){
                printf("cat: %s: Is a directory\n",name);
                return;
            }
            int pos=tmp.son;
            bg=offset;
            readfile(pos);
            for(int i=0;i<tail;i++){
                if(BUF[i]=='\0') 
                    break;               
                printf("%c",BUF[i]);
            }
            return;
        }
        pos=tmp.bro;
    }
    printf("\n");
    return;
}
int main_loop(){
    char *current_dir=pwd();
    printf("\n\e[1mrandow_file_sys\e[0m@admin \e[1m%s\e[0m\n", current_dir);
    printf("> \e[032m$\e[0m ");
    free(current_dir);
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
        int offset;
        scanf("%d %s",&offset,name);
        cat(name,offset);
    }else if(strcmp(cmd,"pwd")==0){
        char * tmp=pwd();
        printf("%s\n",tmp);
        free(tmp);
    }
    else if(strcmp(cmd,"write")==0){
        int offset;
        char name[NAME_LEN];
        scanf("%d %s",&offset,name);
        write(name,offset);
    }else  if(strcmp(cmd,"exit")==0)
        return 1;
    else if(strcmp(cmd,"clear")==0)
        system("clear");
    else
        printf("error command\n");
    return 0;
}
int main(int argc, char* argv[]) {
    init_file();
    cur=fopen("ext2fs.dump", "rb+");
    init(cur);
    system("clear");
    while(1)
        if(main_loop())
            break;
    fclose(cur);
    return 0;
}