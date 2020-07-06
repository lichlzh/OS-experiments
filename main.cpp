#include<cstdio>
#include<cstring>
#include<cstdlib>
#include<cmath>
#include<malloc.h>
#include<algorithm>
using namespace std;
const int MAXN=32,opt_num=19,disk_size=100*1024*1024,block_size=1024,home_dir=0;
const char options[][MAXN]={"login","logout","exit","refresh","register","logoff","info","create","dir","pwd","cd","delete","open","read","write","rename","chmod","copy","tree"};
unsigned char disk[disk_size];
unsigned char *super_block_begin,*super_block_end,*user_block_begin,*user_block_end,*bitmap_block_begin,*bitmap_block_end,*inode_block_begin,*inode_block_end,*data_block_begin,*data_block_end;
struct User{
	char username[MAXN],password[MAXN];
	int id,folder;
	void print(){
		printf("username:%s\npassword:%s\nid:%d\nfolder:%d\n",username,password,id,folder);
	}
};
struct Inode{
	char filename[MAXN];
	int total_size,owner_id,authority,father;
	int cnt,list[200];
	bool isfolder;
	void print(){
		printf("filename:%s\ntotal_size:%d\nowner_id:%d\nauthority:%d\nfather:%d\ncnt:%d\n",filename,total_size,owner_id,authority,father,cnt);
		printf("list:");for(int i=0;i<cnt;i++)printf("%d ",list[i]);
		printf("\nisfolder:%d\n",isfolder);
	}
};
int user_id=-1,cur_pos=0;

void read_from_disk(unsigned char* addr,bool& x){
	x=*addr;
}
void write_to_disk(unsigned char* addr,bool x){
	*addr=x;
}

void read_from_disk(unsigned char* addr,int& x){
	x=0;
	for(int i=0;i<4;i++)
		x=(x<<8)+addr[i];
}
void write_to_disk(unsigned char* addr,int x){
	for(int i=3;i>=0;i--){
		addr[i]=x&255;
		x>>=8;
	}
}

void read_from_disk(unsigned char* addr,char& x){
	x=*addr;
}
void write_to_disk(unsigned char* addr,char x){
	*addr=x;
}

void read_from_disk(unsigned char* addr,struct User& x){
	for(int i=0;i<MAXN;i++){
		read_from_disk(addr,x.username[i]);
		addr++;
	}
	for(int i=0;i<MAXN;i++){
		read_from_disk(addr,x.password[i]);
		addr++;
	}
	read_from_disk(addr,x.id);
	read_from_disk(addr+4,x.folder);
}
void write_to_disk(unsigned char* addr,struct User& x){
	for(int i=0;i<MAXN;i++){
		write_to_disk(addr,x.username[i]);
		addr++;
	}
	for(int i=0;i<MAXN;i++){
		write_to_disk(addr,x.password[i]);
		addr++;
	}
	write_to_disk(addr,x.id);
	write_to_disk(addr+4,x.folder);
}

void read_from_disk(unsigned char* addr,struct Inode& x){
	for(int i=0;i<MAXN;i++){
		read_from_disk(addr,x.filename[i]);
		addr++;
	}
	read_from_disk(addr,x.total_size);
	read_from_disk(addr+4,x.owner_id);
	read_from_disk(addr+8,x.authority);
	read_from_disk(addr+12,x.father);
	read_from_disk(addr+16,x.cnt);
	for(int i=0;i<200;i++)
		read_from_disk(addr+20+i*4,x.list[i]);
	read_from_disk(addr+20+200*4,x.isfolder);
}
void write_to_disk(unsigned char* addr,struct Inode& x){
	for(int i=0;i<MAXN;i++){
		write_to_disk(addr,x.filename[i]);
		addr++;
	}
	write_to_disk(addr,x.total_size);
	write_to_disk(addr+4,x.owner_id);
	write_to_disk(addr+8,x.authority);
	write_to_disk(addr+12,x.father);
	write_to_disk(addr+16,x.cnt);
	for(int i=0;i<200;i++)
		write_to_disk(addr+20+i*4,x.list[i]);
	write_to_disk(addr+20+200*4,x.isfolder);
}

int path_analyzer(int id,char* path){
	Inode x,y;
	read_from_disk(inode_block_begin+block_size*id+1,x);
	char s[MAXN];
	int cnt=0;
	int len=strlen(path);
	for(int i=0;i<len;i++){
		if(path[i]!='\\')
			s[cnt++]=path[i];
		if(path[i]=='\\'||i==len-1){
			if(cnt==2&&s[0]==s[1]&&s[0]=='.'){
				if(x.father!=-1){
					id=x.father;
					read_from_disk(inode_block_begin+block_size*id+1,x);
					cnt=0;
				}
				continue;
			}
			if(!x.isfolder)
				return -1;
			s[cnt]='\0';
			bool find=false;
			for(int j=0;j<x.cnt;j++){
				int tmp_pos=x.list[j];
				read_from_disk(inode_block_begin+tmp_pos*block_size+1,y);
				if(strcmp(s,y.filename)==0){
					find=true;
					id=tmp_pos;
					x=y;
					cnt=0;
					break;
				}
			}
			if(!find)
				return -1;
		}
	}
	return id;
}

int fill_data(unsigned char* content){
	for(unsigned char* i=bitmap_block_begin+(inode_block_end-disk)/block_size/8;i<bitmap_block_begin+disk_size/block_size/8;i++){
		for(int j=7;j>=0;j--){
			if(!((*i)&(1<<j))){
				(*i)|=(1<<j);
				int block_id=(i-bitmap_block_begin)*8+7-j,len=strlen((char*)content);
				for(int k=block_id*block_size;k<block_id*block_size+len;k++)
					disk[k]=content[k-block_id*block_size];
				return block_id;
			}
		}
	}
	return -1;
}

void erase_data(int p){
	int i=p/8,j=p%8;
	*(bitmap_block_begin+i)^=(1<<(7-j));
}
unsigned char* get_data(int p,int len){
	static unsigned char res_data[block_size+1];
	for(int i=0;i<len;i++)
		res_data[i]=disk[block_size*p+i];
	res_data[len]='\0';
	return res_data;
}
int copy_data(int p,int len){
	for(unsigned char* i=bitmap_block_begin+(inode_block_end-disk)/block_size/8;i<bitmap_block_begin+disk_size/block_size/8;i++){
		for(int j=7;j>=0;j--){
			if(!((*i)&(1<<j))){
				(*i)|=(1<<j);
				int block_id=(i-bitmap_block_begin)*8+7-j;
				for(int k=block_id*block_size;k<block_id*block_size+len;k++)
					disk[k]=disk[block_size*p+k-block_id*block_size];
				return block_id;
			}
		}
	}
	return -1;
}

int create_file(Inode &x,unsigned char* content=NULL){
	unsigned char* s=(unsigned char*)malloc((block_size+1)*sizeof(unsigned char));
	for(unsigned char* i=inode_block_begin;i<inode_block_end;i+=block_size){
		bool has_inode;
		read_from_disk(i,has_inode);
		if(!has_inode){
			if(!x.isfolder){
				int len=strlen((char*)content);
				for(int j=0;j<x.cnt;j++){
					for(int k=j*block_size;k<min(j*block_size+block_size,len);k++)
						s[k-j*block_size]=content[k];
					s[min(j*block_size+block_size,len)-j*block_size]='\0';
					x.list[j]=fill_data(s);
				}
			}
			write_to_disk(i,true);
			write_to_disk(i+1,x);
			free(s);
			printf("create success\n");
			return (i-inode_block_begin)/block_size;
		}
	}
	free(s);
	return -1;
}
bool judge_authority(int id,Inode &x,int authority){
	if(x.owner_id!=id)
		return (authority&x.authority)==authority;
	return (authority&(x.authority>>6))==authority;
}
bool delete_file(int id,int uid=user_id){
	Inode x,y;
	read_from_disk(inode_block_begin+block_size*id+1,x);
	if(x.isfolder){
		for(int i=0;i<x.cnt;i++)
			if(!delete_file(x.list[i],uid)){
				write_to_disk(inode_block_begin+block_size*id,false);
				return false;
			}
		if(!judge_authority(uid,x,2)){
			write_to_disk(inode_block_begin+block_size*id,false);
			return false;
		}
	}
	else{
		if(!judge_authority(uid,x,2))
			return false;
		for(int i=0;i<x.cnt;i++)
			erase_data(x.list[i]);
	}
	if(x.father!=-1){
		read_from_disk(inode_block_begin+block_size*x.father+1,y);
		bool vis_del=false;
		for(int i=0;i<y.cnt;i++)
			if(vis_del)
				y.list[i-1]=y.list[i];
			else if(y.list[i]==id)
				vis_del=true;
		y.cnt--;
		write_to_disk(inode_block_begin+block_size*x.father+1,y);
	}
	write_to_disk(inode_block_begin+block_size*id,false);
	return true;
}
bool open_file(Inode &x,int uid=user_id){
	unsigned char* res;
	if(!judge_authority(uid,x,1))
		return false;
	int res_size=x.total_size;
	for(int i=0;i<x.cnt;i++){
		if(res_size>=block_size){
			res=get_data(x.list[i],block_size);
			res_size-=block_size;
		}
		else{
			res=get_data(x.list[i],res_size);
			res_size=0;
		}
		printf("%s",res);
	}
	printf("\n");
	return true;
}
bool read_file(Inode &x,int uid=user_id){
	unsigned char* res;
	if(!judge_authority(uid,x,4))
		return false;
	int res_size=x.total_size;
	for(int i=0;i<x.cnt;i++){
		if(res_size>=block_size){
			res=get_data(x.list[i],block_size);
			res_size-=block_size;
		}
		else{
			res=get_data(x.list[i],res_size);
			res_size=0;
		}
		printf("%s",res);
	}
	printf("\n");
	return true;
}
bool write_file(int id,int uid,unsigned char* content){
	Inode x;
	read_from_disk(inode_block_begin+block_size*id+1,x);
	if(!judge_authority(uid,x,2))
		return false;
	for(int i=0;i<x.cnt;i++)
		erase_data(x.list[i]);
	unsigned char* s=(unsigned char*)malloc((block_size+1)*sizeof(unsigned char));
	int len=strlen((char*)content);
	x.total_size=len;
	x.cnt=len/block_size+(len%block_size!=0);
	for(int j=0;j<x.cnt;j++){
		for(int k=j*block_size;k<min(j*block_size+block_size,len);k++)
			s[k-j*block_size]=content[k];
		s[min(j*block_size+block_size,len)-j*block_size]='\0';
		x.list[j]=fill_data(s);
	}
	write_to_disk(inode_block_begin+block_size*id+1,x);
	free(s);
	return true;
}
bool judge_include(int from_id,int dst_id){
	if(from_id==dst_id)
		return true;
	Inode x;
	int t=dst_id;
	while(t!=-1){
		read_from_disk(inode_block_begin+block_size*t+1,x);
		if(x.father==from_id)
			return true;
		t=x.father;
	}
	return false;
}
bool copy_all(int from_id,int dst_id){
	Inode x,y,z;
	read_from_disk(inode_block_begin+block_size*from_id+1,x);
	read_from_disk(inode_block_begin+block_size*dst_id+1,y);
	z=x,z.cnt=0,z.father=dst_id,z.owner_id=user_id;
	int new_id=-1;
	for(unsigned char* i=inode_block_begin;i<inode_block_end;i+=block_size){
		bool has_inode;
		read_from_disk(i,has_inode);
		if(!has_inode){
			write_to_disk(i,true);
			write_to_disk(i+1,x);
			new_id=(i-inode_block_begin)/block_size;
			break;
		}
	}
	if(new_id==-1)
		return false;
	y.list[y.cnt++]=new_id;
	write_to_disk(inode_block_begin+block_size*dst_id+1,y);
	if(x.isfolder){
		write_to_disk(inode_block_begin+block_size*new_id+1,z);
		Inode w;
		for(int i=0;i<x.cnt;i++){
			read_from_disk(inode_block_begin+block_size*x.list[i]+1,w);
			if(!judge_authority(user_id,w,5)){
				printf("permission denied\n");
				return false;
			}
			copy_all(x.list[i],new_id);
		}
	}
	else{
		for(int i=0;i<x.cnt;i++){
			int len=block_size;
			if(i==x.cnt-1)
				len=x.total_size%block_size;
			z.list[i]=copy_data(x.list[i],len);
			z.cnt++;
		}
		write_to_disk(inode_block_begin+block_size*new_id+1,z);
	}
	return true;
}

void tree(int id,int uid,int depth=0){
	Inode x;
	read_from_disk(inode_block_begin+id*block_size+1,x);
	for(int i=0;i<depth;i++)
		printf("-");
	printf("%s\n",x.filename);
	if(x.isfolder&&judge_authority(uid,x,5))
		for(int i=0;i<x.cnt;i++)
			tree(x.list[i],uid,depth+1);
}

void analyze_super_block(){
	int x;
	read_from_disk(disk,x);
	super_block_begin=disk+x*1024;
	read_from_disk(disk+4,x);
	super_block_end=disk+x*1024;
	
	read_from_disk(disk+8,x);
	user_block_begin=disk+x*1024;
	read_from_disk(disk+12,x);
	user_block_end=disk+x*1024;
	
	read_from_disk(disk+16,x);
	bitmap_block_begin=disk+x*1024;
	read_from_disk(disk+20,x);
	bitmap_block_end=disk+x*1024;
	
	read_from_disk(disk+24,x);
	inode_block_begin=disk+x*1024;
	read_from_disk(disk+28,x);
	inode_block_end=disk+x*1024;
	
	read_from_disk(disk+32,x);
	data_block_begin=disk+x*1024;
	read_from_disk(disk+36,x);
	data_block_end=disk+x*1024;
}
void initialize(){
	printf("loading from simdisk\n");
	FILE *File=fopen("simdisk","rb");
	fread(&disk,disk_size,1,File);
	fclose(File);
	printf("loading success\n");
	printf("analyzing super block\n");
	analyze_super_block();
	printf("analyze success\n");
}

void refresh(){
	FILE *File=fopen("simdisk","wb");
	fwrite(&disk,disk_size,1,File);
	fclose(File);
	printf("refresh success\n");
}

void write_test_data(){
	write_to_disk(disk,0);
	write_to_disk(disk+4,1);
	write_to_disk(disk+8,1);
	write_to_disk(disk+12,11);
	write_to_disk(disk+16,11);
	write_to_disk(disk+20,24);
	write_to_disk(disk+24,24);
	write_to_disk(disk+28,1024);
	write_to_disk(disk+32,1024);
	write_to_disk(disk+36,102400);
	int user_count=1,user_inc=1;
	write_to_disk(disk+1024,user_count);
	write_to_disk(disk+1024+4,user_inc);
	Inode x;
	strcpy(x.filename,"home"),x.total_size=0,x.owner_id=-1,x.authority=0777,x.father=-1;
	x.cnt=0,x.isfolder=true;
	write_to_disk(disk+24576,true);
	write_to_disk(disk+24577,x);
	refresh();
}

User* find_user(char *username){
	int user_count,user_inc;
	User *user=(User*)malloc(sizeof(User));
	read_from_disk(user_block_begin,user_count);
	read_from_disk(user_block_begin+4,user_inc);
	for(int i=0;i<user_count;i++){
		read_from_disk(user_block_begin+8+i*sizeof(User),*user);
		if(strcmp(user->username,username)==0)
			return user;
	}
	return NULL;
}

void create_user(char* username,char* password){
	int user_count,user_inc;
	read_from_disk(user_block_begin,user_count);
	read_from_disk(user_block_begin+4,user_inc);
	if(user_count==150){
		printf("user number exceeded\n");
		return;
	}
	User *user=(User*)malloc(sizeof(User));
	Inode x;
	strcpy(x.filename,username),x.total_size=0,x.owner_id=user_inc,x.authority=0775,x.father=0,x.cnt=0,x.isfolder=true;
	strcpy(user->username,username),strcpy(user->password,password),user->id=user_inc,user->folder=create_file(x);
	write_to_disk(user_block_begin,user_count+1);
	write_to_disk(user_block_begin+4,user_inc+1);
	write_to_disk(user_block_begin+8+user_count*sizeof(User),user);
	
	read_from_disk(inode_block_begin+home_dir*block_size+1,x);
	x.list[x.cnt++]=user->folder;
	write_to_disk(inode_block_begin+home_dir*block_size+1,x);
	printf("create success\n");
}

void delete_user(char* username,char* password){
	int user_count,user_inc;
	bool vis_del=false;
	read_from_disk(user_block_begin,user_count);
	read_from_disk(user_block_begin+4,user_inc);
	User user;
	for(int i=0;i<user_count;i++){
		read_from_disk(user_block_begin+8+i*sizeof(User),user);
		if(strcmp(user.username,username)==0){
			if(strcmp(user.password,password)==0){
				vis_del=1;
				if(user_id==user.id)
					user_id=-1;
				continue;
			}
			printf("password wrong\n");
			return;
		}
		if(!vis_del)
			write_to_disk(user_block_begin+8+i*sizeof(User),user);
		else
			write_to_disk(user_block_begin+8+(i-1)*sizeof(User),user);
	}
	write_to_disk(user_block_begin,user_count-1);
	printf("logoff success\n");
}

void process_option(int x){
	if(x==0){//login
		static char username[MAXN],password[MAXN];
		printf("Please input username:");
		scanf("%s",username);
		struct User* user=find_user(username);
		if(user==NULL){
			printf("user does not exist\n");
			free(user);
			return;
		}
		printf("Please input password:");
		scanf("%s",password);
		if(strcmp(password,user->password)==0){
			printf("login success\n");
			user_id=user->id;
		}
		else
			printf("password wrong\n");
		free(user);
	}
	else if(x==1){//logout
		user_id=-1;
		printf("logout success\n");
	}
	else if(x==2)exit(0);//exit
	else if(x==3)refresh();//refresh
	else if(x==4){//register
		static char username[MAXN],password[MAXN];
		printf("Please input username:");
		scanf("%s",username);
		struct User* user=find_user(username);
		if(user!=NULL){
			printf("user exists\n");
			free(user);
			return;
		}
		printf("Please input password:");
		scanf("%s",password);
		create_user(username,password);
		free(user);
	}
	else if(x==5){//logoff
		static char username[MAXN],password[MAXN];
		printf("Please input username:");
		scanf("%s",username);
		struct User* user=find_user(username);
		if(user==NULL){
			printf("user does not exist\n");
			free(user);
			return;
		}
		printf("Please input password:");
		scanf("%s",password);
		delete_user(username,password);
		free(user);
	}
	else if(x==6){//info
		printf("Welcome to File System!\n");
		printf("disk structure:\n");
		printf("super_block:  [%d,%d)\n",super_block_begin-disk,super_block_end-disk);
		printf("user_block:   [%d,%d)\n",user_block_begin-disk,user_block_end-disk);
		printf("bitmap_block: [%d,%d)\n",bitmap_block_begin-disk,bitmap_block_end-disk);
		printf("inode_block:  [%d,%d)\n",inode_block_begin-disk,inode_block_end-disk);
		printf("data_block:   [%d,%d)\n",data_block_begin-disk,data_block_end-disk);
	}
	else if(x==7){//create
		if(user_id==-1){
			printf("please login first\n");
			return;
		}
		static unsigned char content[200*block_size+1];
		Inode x,y;
		read_from_disk(inode_block_begin+cur_pos*block_size+1,x);
		if(!judge_authority(user_id,x,7)){
			printf("permission denied\n");
			return;
		}
		scanf("%s",content);
		if(strcmp((char*)content,"--dir")==0){
			y.isfolder=true;
			scanf("%s",y.filename);
			y.total_size=0,y.owner_id=user_id,y.authority=0775,y.father=cur_pos,y.cnt=0;
			x.list[x.cnt++]=create_file(y);
		}
		else if(strcmp((char*)content,"--file")==0){
			y.isfolder=false;
			scanf("%s%s",y.filename,content);
			int len=strlen((char*)content);
			y.total_size=len,y.owner_id=user_id,y.authority=0775,y.father=cur_pos,y.cnt=len/block_size+(len%block_size!=0);
			x.list[x.cnt++]=create_file(y,content);
		}
		else{
			printf("argument error\n");
			return;
		}
		write_to_disk(inode_block_begin+cur_pos*block_size+1,x);
	}
	else if(x==8){//dir
		Inode x,y;
		read_from_disk(inode_block_begin+cur_pos*block_size+1,x);
		if(!judge_authority(user_id,x,4)){
			printf("permission denied\n");
			return;
		}
		for(int i=0;i<x.cnt;i++){
			int tmp_pos=x.list[i];
			read_from_disk(inode_block_begin+tmp_pos*block_size+1,y);
			if(y.isfolder)
				printf("foldername:%s, inode address:%d, total_size:??B, authority:%o\n",y.filename,inode_block_begin-disk+tmp_pos*block_size+1,y.authority);
			else{
				printf("filename:%s, inode address:%d, total_size:%dB, authority:%o, ",y.filename,inode_block_begin-disk+tmp_pos*block_size+1,y.total_size,y.authority);
				printf("data block number:%d, data block address:",y.cnt);
				for(int j=0;j<y.cnt;j++)
					printf("%d ",y.list[j]*block_size);
				printf("\n");
			}
		}
	}
	else if(x==9){//pwd
		static char* path[MAXN];
		Inode x;
		int len=0;
		int tmp_pos=cur_pos;
		while(tmp_pos!=-1){
			read_from_disk(inode_block_begin+tmp_pos*block_size+1,x);
			path[len]=(char*)malloc(MAXN*sizeof(char));
			strcpy(path[len],x.filename);
			len++;
			tmp_pos=x.father;
		}
		for(int i=len-1;i>=0;i--){
			printf("%s\\",path[i]);
			free(path[i]);
		}
		printf("\n");
	}
	else if(x==10){//cd
		Inode x;
		read_from_disk(inode_block_begin+cur_pos*block_size+1,x);
		if(!judge_authority(user_id,x,1)){
			printf("permission denied\n");
			return;
		}
		static char arg[MAXN*MAXN];
		scanf("%s",arg);
		int dst_id=path_analyzer(cur_pos,arg);
		if(dst_id==-1)
			printf("no such file or directory\n");
		else{
			read_from_disk(inode_block_begin+dst_id*block_size+1,x);
			if(!x.isfolder)
				printf("cannot cd a file");
			else
				cur_pos=dst_id;
		}
	}//todo: cd absolute diretory
	else if(x==11){//delete
		if(user_id==-1){
			printf("please login first\n");
			return;
		}
		static char arg[MAXN*MAXN];
		scanf("%s",arg);
		int dst_id=path_analyzer(cur_pos,arg);
		if(dst_id==-1)
			printf("no such file or directory\n");
		else{
			if(delete_file(dst_id,user_id))
				printf("delete success\n");
			else
				printf("permission denied\n");
		}
	}
	else if(x==12){//open
		if(user_id==-1){
			printf("please login first\n");
			return;
		}
		static char arg[MAXN*MAXN];
		scanf("%s",arg);
		int dst_id=path_analyzer(cur_pos,arg);
		if(dst_id==-1)
			printf("no such file or directory\n");
		else{
			Inode x;
			read_from_disk(inode_block_begin+dst_id*block_size+1,x);
			if(x.isfolder)
				printf("cannot open a directory\n");
			else if(!open_file(x,user_id))
				printf("permission denied\n");
		}
	}
	else if(x==13){//read
		if(user_id==-1){
			printf("please login first\n");
			return;
		}
		static char arg[MAXN*MAXN];
		scanf("%s",arg);
		int dst_id=path_analyzer(cur_pos,arg);
		if(dst_id==-1)
			printf("no such file or directory\n");
		else{
			Inode x;
			read_from_disk(inode_block_begin+dst_id*block_size+1,x);
			if(x.isfolder)
				printf("cannot read a directory\n");
			else if(!read_file(x,user_id))
				printf("permission denied\n");
		}
	}
	else if(x==14){//write
		if(user_id==-1){
			printf("please login first\n");
			return;
		}
		static char arg[MAXN*MAXN];
		static unsigned char content[200*block_size+1];
		scanf("%s",arg);
		int dst_id=path_analyzer(cur_pos,arg);
		if(dst_id==-1)
			printf("no such file or directory\n");
		else{
			Inode x;
			read_from_disk(inode_block_begin+dst_id*block_size+1,x);
			if(x.isfolder)
				printf("cannot write to a directory\n");
			else{
				scanf("%s",content);
				if(!write_file(dst_id,user_id,content))
					printf("permission denied\n");
				else
					printf("write success\n");
			}
		}
	}
	else if(x==15){//rename
		if(user_id==-1){
			printf("please login first\n");
			return;
		}
		Inode x,y,z;
		read_from_disk(inode_block_begin+cur_pos*block_size+1,x);
		if(!judge_authority(user_id,x,7)){
			printf("permission denied\n");
			return;
		}
		static char arg[MAXN*MAXN];
		scanf("%s",arg);
		int dst_id=path_analyzer(cur_pos,arg);
		if(dst_id==-1)
			printf("no such file or directory\n");
		else{
			scanf("%s",arg);
			read_from_disk(inode_block_begin+dst_id*block_size+1,x);
			if(x.father==-1){
				printf("cannot rename home\n");
				return;
			}
			read_from_disk(inode_block_begin+x.father*block_size+1,y);
			for(int i=0;i<y.cnt;i++){
				read_from_disk(inode_block_begin+y.list[i]*block_size+1,z);
				if(strcmp(z.filename,arg)==0){
					printf("duplicate file name\n");
					return;
				}
			}
			strcmp(x.filename,arg);
			write_to_disk(inode_block_begin+dst_id*block_size+1,x);
			printf("rename success\n");
		}
	}
	else if(x==16){//chmod
		if(user_id==-1){
			printf("please login first\n");
			return;
		}
		static char arg[MAXN*MAXN];
		scanf("%s",arg);
		int dst_id=path_analyzer(cur_pos,arg);
		if(dst_id==-1)
			printf("no such file or directory\n");
		else{
			Inode x;
			read_from_disk(inode_block_begin+dst_id*block_size+1,x);
			if(x.owner_id!=user_id){
				printf("only owner is permitted to chmod\n");
				return;
			}
			int authority;
			scanf("%o",&authority);
			if(authority>777){
				printf("authority code invalid\n");
				return;
			}
			x.authority=authority;
			write_to_disk(inode_block_begin+dst_id*block_size+1,x);
		}
	}
	else if(x==17){//copy
		if(user_id==-1){
			printf("please login first\n");
			return;
		}
		static char arg[MAXN*MAXN];
		scanf("%s",arg);
		int from_id=path_analyzer(cur_pos,arg);
		if(from_id==-1)
			printf("no such file or directory\n");
		else{
			Inode x,y;
			read_from_disk(inode_block_begin+from_id*block_size+1,x);
			if(x.father==-1){
				printf("cannot copy home directory\n");
				return;
			}
			read_from_disk(inode_block_begin+x.father*block_size+1,y);
			if(!judge_authority(user_id,y,5)){
				printf("permission denied\n");
				return;
			}
			scanf("%s",arg);
			int dst_id=path_analyzer(cur_pos,arg);
			if(dst_id==-1)
				printf("no such file or directory\n");
			else{
				read_from_disk(inode_block_begin+dst_id*block_size+1,y);
				if(!y.isfolder){
					printf("cannot copy to a file\n");
					return;
				}
				if(!judge_authority(user_id,y,7)){
					printf("permission denied\n");
					return;
				}
				if(judge_include(from_id,dst_id))
					printf("path joint\n");
				else{
					if(copy_all(from_id,dst_id))
						printf("copy success\n");
					else
						printf("permission denied\n");
				}
			}
		}
	}
	else if(x==18){//tree
		tree(cur_pos,user_id);
	}
}

void work(){
	printf("Please input options(login,logout,exit,refresh,register,logoff,info,create,dir,pwd,cd,delete,open,read,write,rename,chmod,copy,tree)\n");
	while(true){
		static char opt[MAXN];
		scanf("%s",opt);
		for(int i=0;i<opt_num;i++)
			if(strcmp(options[i],opt)==0){
				process_option(i);
				break;
			}
	}
}
int main(){
	//write_test_data();
	initialize();
	work();
	return 0;
}
