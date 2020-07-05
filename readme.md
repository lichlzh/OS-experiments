### 二级文件系统

#### 运行环境

$gcc3.3.1$ 其他很多版本应该也可以。

#### 硬盘结构

共 $100M$，每块 $1K$，即 $100*1024$ 块。

本项目使用内存模拟外存的方式，程序启动时，加载 $\text{simdisk}$ 硬盘文件到数组中，之后对数组进行操作，收到特定指令(refresh)时，将数组内容写入 $\text{simdisk}$，这样做是因为频繁同步 $100M$ 大小的内容会造成卡顿。

第一次使用请去掉 $\text{//write_test_data();}$ 的注释，创建硬盘，写入初始数据。

##### (1)

$[0,1)$ 超级块：用于存储硬盘分区上一共有多少个 $\text{block group}$，以及各个 $\text{block group}$ 的占用区间，超级块就是第一个 $\text{block group}$。所以实际上超级块只是线性存储了一些 $int$。

程序启动时会加载超级块中的内容。

##### (2)

$[1,11)$ 用户信息块：这个 $\text{block group}$ 起始存储两个$int$，分别是用户总数 $\text{user_count}$ 和用户自增标号 $\text{user_inc}$（用于设置新增的用户编号）。后面线性存储用户名、密码、$id$ 和用户文件夹的$inode~~id$（也是下标），用户数量上限是 $150$。

```c++
struct User{
	char username[MAXN],password[MAXN];
	int id,folder;
};
```

用户名密码使用固定长度数组存储，用户名密码长度不能超过 $31$。

这其实算是设计缺陷之一，用户信息其实也可以作为文件存储，放入数据块中。

##### (3)

$[11,24)$ 位图($bitmap$)块，用于存储位图，需要占用 $12.5$ 个块，所以分配了 $13$ 个块。位图用来标记数据块是否被占用。

##### (4)

$[24,1024)$ $inode$ 块，每个 $inode$ 占用一个块，共有 $1000$ 个 $inode$。每个 $inode$ 中第一个字节标记了此块是否存储了 $inode$。$inode$ 中分别是文件名、文件大小、文件所有者、$9$ 位保护码（只有权限）、父文件夹 $inode$ 地址、子文件(夹)数量、子文件 $inode$ 地址列表以及文件夹标记。

```c++
struct Inode{
	char filename[MAXN];
	int total_size,owner_id,authority,father;
	int cnt,list[200];
	bool isfolder;
};
```

##### (5)

$[1024,102400)$ 数据块。

#### 指令说明

```
(1)login arg1 arg2
arg1:用户名，arg2:密码。登录指令。

(2)logout
退出当前用户。

(3)exit
退出系统（不自动保存到硬盘）

(4)refresh
将数组中内容二进制格式更新到硬盘simdisk。

(5)register arg1 arg2
arg1:用户名，arg2:密码。注册指令，注意用户名不能重复，用户id自增。

(6)logoff arg1 arg2
arg1:用户名，arg2:密码。用户注销，将用户删除。

(7)info
输出超级块中的系统信息。

(8)create --dir arg
在当前目录创建文件夹，文件夹名为arg；
create --file arg1 arg2
在当前目录创建文件，文件名为arg1，文件初始内容为arg2。

(9)dir
列出当前目录下的文件和文件夹。
文件夹显示 文件夹名，inode地址，9位保护码（权限）

(10)pwd
显示当前所在位置

(11)cd arg
进入arg目录，支持多级，分隔符为反斜杠(\)，只支持相对目录。

(12)delete arg
删除arg文件，支持多级，分隔符为反斜杠(\)，只支持相对目录。支持删除文件夹，如果删除文件夹中的某些文件没有权限，立刻停止，已经删掉的就删掉吧，随缘。

(13)open arg
打开arg文件，支持多级，分隔符为反斜杠(\)，只支持相对目录。目前只支持字符串文件，打开后显示字符串内容。

(14)read arg
打开arg文件，支持多级，分隔符为反斜杠(\)，只支持相对目录。目前只支持字符串文件，打开后显示字符串内容。与open权限不同。

(15)write arg1 arg2
将arg2字符串写入arg1文件，支持多级，分隔符为反斜杠(\)，只支持相对目录。

(16)rename arg1 arg2
将arg1文件重命名为arg2，支持多级，分隔符为反斜杠(\)，只支持相对目录。

(17)chmod arg XXX
将arg文件权限修改为XXX。XXX为三位八进制数。只有文件所有者才能修改权限。
修改权限会导致很多问题，暂时没有对这些可能出现的异常进行设计和处理。
支持多级，分隔符为反斜杠(\)，只支持相对目录。

(18)copy arg1 arg2
将arg1文件(夹)复制到arg2文件夹，文件的权限和所有者都不变。支持多级，分隔符为反斜杠(\)，只支持相对目录。
```

#### 设计逻辑附录

##### 权限说明

文件和目录的权限，有 $RWX(421)$ 三种。
$r(Read，读取)$：对文件而言，具有读取文件内容的权限；对目录来说，具有浏览目录的权限。
$w(Write,写入)$：对文件而言，具有新增,修改,删除文件内容的权限；对目录来说，具有新建，删除，修改，移动目录内文件的权限。
$x(eXecute，执行)$：对文件而言，具有执行文件的权限；对目录了来说该用户具有进入目录的权限。
1、目录的只读访问不允许使用 $cd$ 进入目录，必须要有执行的权限才能进入。
2、只有执行权限只能进入目录，不能 $dir$ 看到目录下的内容，要想看到目录下的文件名和目录名，需要可读权限。
3、一个文件能不能被删除，主要看该文件所在的目录对用户是否具有写权限，如果目录对用户没有写权限，则该目录下的所有文件都不能被删除。
新建文件的权限默认 $755$。

##### 底层实现

没有使用 $bool$ 数组，因为一个 $bool$ 变量其实是一个字节。而使用位域的话会出现很多错误，所以最终使用了$unsigned~~char$ 数组模拟硬盘。

实现了各种数据类型写和读 $unsigned~~char$数组 的函数，在后续的开发中提供了很多便利。

##### 函数一览表

```
void read_from_disk(unsigned char* addr,bool& x)
从磁盘addr地址读入一个bool存入x中。
void write_to_disk(unsigned char* addr,bool x)
将bool x写入磁盘的addr地址。
void read_from_disk(unsigned char* addr,int& x)
从磁盘addr地址读入一个int存入x中。
void write_to_disk(unsigned char* addr,int x)
将int x写入磁盘的addr地址。
void read_from_disk(unsigned char* addr,char& x)
从磁盘addr地址读入一个char存入x中。
void write_to_disk(unsigned char* addr,char x)
将char x写入磁盘的addr地址。
void read_from_disk(unsigned char* addr,struct User& x)
从磁盘addr地址读入一个User类存入x中。
void write_to_disk(unsigned char* addr,struct User& x)
将User x写入磁盘的addr地址。此处x使用引用类型节省内存开销。
void read_from_disk(unsigned char* addr,struct Inoder& x)
从磁盘addr地址读入一个Inode类存入x中。
void write_to_disk(unsigned char* addr,struct Inode& x)
将Inode x写入磁盘的Inode地址。
int path_analyzer(int id,char* path)
返回从编号为id的文件夹经过路径path后到达的文件id，如果路径错误则返回-1。
int fill_data(unsigned char* content)
寻找bitmap中的空位，将content中的数据写入数据块，返回写入的数据块块号。
void erase_data(int p)
释放块号为p的数据块。
unsigned char* get_data(int p,int len)
从块号为p的数据块读取len个字节并返回。
int copy_data(int p,int len)
创建块号为p的数据块中前len个字节数据的副本，寻找空数据块写入，返回写入的块号。
int create_file(Inode &x,unsigned char* content=NULL)
创建Inode x对应的文件：在Inode块中寻找空位存储x，如果x是文件，还要将content中的数据写入数据块并将块号存入x的list中。
bool judge_authority(int id,Inode &x,int authority)
判断用户id是否对Inode x对应的文件有authority权限。
bool delete_file(int id,int uid=user_id)
用户uid为操作者，删除id对应的文件，如果是文件夹则递归删除其中所有文件，如果权限错误返回false。
bool open_file(Inode &x,int uid=user_id)
用户uid为操作者，打开Inode x对应的文件，如果权限错误返回false。
bool read_file(Inode &x,int uid=user_id)
用户uid为操作者，读取Inode x对应的文件，如果权限错误返回false。
bool write_file(int id,int uid,unsigned char* content)
用户uid为操作者，将content中的数据写入id对应的文件。
bool judge_include(int from_id,int dst_id)
复制操作前使用，判断源文件夹from_id是否包括目的文件夹dst_id。
bool copy_all(int from_id,int dst_id)
将from_id对应的文件(夹)复制到dst_id目录下，如果权限错误返回false。
void analyze_super_block()
程序启动初始化时使用，加载超级块。
void initialize()
程序初始化，加载虚拟硬盘simdisk到内存，加载超级块。
void refresh()
将内存写入虚拟硬盘simdisk。
void write_test_data()
初始化空虚拟硬盘，将硬盘分区设计写入超级块，创建根文件夹home。
User* find_user(char *username)
返回用户名为username的User指针。
void create_user(char* username,char* password)
新建用户，用户名username，密码password。
void delete_user(char* username,char* password)
删除用户，用户名username，密码password。
void process_option(int x)
进行第x种操作。
void work()
监听用户输入，识别相应的操作。
int main()
主函数。
```

