//qsort排序文件名有问题
#include <stdio.h>
#include <stdlib.h>    //malloc，qsort
#include <string.h>    //字符串处理函数
#include <sys/stat.h>  //lstat，S_ISDIR等宏
#include <sys/types.h> //lstat,opendir，getpwuid，getgrgid
#include <time.h>
#include <unistd.h> //lstat
#include <dirent.h> //opendir,readdir
#include <grp.h>
#include <pwd.h> //getpwuid，getgrgid
#include <errno.h>

#define NO 0 //无参数
#define A  1   //-a：显示所有文件
#define L  2   //-l：显示文件的详细信息
#define R  4   //-R：连同子目录内容一起列出来
#define I  8   //-i：显示每个文件的inode号
#define T  16  //-t：按文件最后的修改时间排序
#define r  32  //-r：将文件以相反次序显示
#define S  64  //-s：以块大小为单位列出所有文件的大小
#define MAX_ROWLEN 100 //一行显示的最多字符串

int g_leave_len = MAX_ROWLEN; //一行剩余长度，用于输出对齐
int g_maxlen;                 //存放某目录下最长文件名的长度

int flag = NO;      //记录输入的所有选项
char pathname[260]; //记录输入的路径名
char PATH[260];     //记录路径名（-R）
int f;              //在有R时用来保证操作只做一次

void anal_param(int argc, char *argv[]); //分析参数，得到flag，path

void my_err(const char *err_string, int line); //错误处理函数

void disply_file_only(char *path); //无-l单文件打印函数,仅打印文件名

void disply_R_only(char *path);//-R，无-l单文件打印函数

void disply_file_l(char *path); //-l单文件打印函数，打印文件详细信息

void disply_R_l(char *path);//-l,-R单文件打印函数

void disply_dir(char *path); //目录下多文件打印函数

void disply(char **name, int count);//多文件打印函数

int cmp(const void *a, const void *b); //比较函数

void file_sort(char **filenames, int count); //目录下多文件排序函数（-r,-t）

void color_printf(char *filename, struct stat buf); //染色打印文件名函数

int cmp(const void *x, const void *y); //用于qsort比较函数


int main(int argc, char *argv[])
{
    anal_param(argc, argv); //解析参数和判断是否含有有效路径

    //根据路径类型进入不同函数
    struct stat Stat;       //保存路径信息的结构体Stat
    lstat(pathname, &Stat); //获取路径信息
    
    if (S_ISDIR(Stat.st_mode)) //如果输入的路径是目录，进入目录打印函数
    {
        if( pathname[strlen(pathname)-1] !='/' )
        {
            pathname[strlen(pathname)] = '/';
            pathname[strlen(pathname)+1] = '\0';
        }
        disply_dir(pathname);
    }
    else //否则输入的路径是文件
    {
        if (flag & L) //有-l选项，进入-l文件打印函数
            disply_file_l(pathname);
        else
        {
            disply_file_only(pathname); //无-l选项，进入无-l文件打印函数
            printf("\n");
        }
    }

    return 0;
}


//分析参数，得到flag，path
void anal_param(int argc, char *argv[])
{
    char param[8] = {'0'}; //记录有哪些参数
    int n = 0;             //记录参数个数
    int j = 0;
    int k = 0;
    //保存参数进param
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            for (k = 1; k < strlen(argv[i]); k++, j++)
                param[j] = argv[i][k]; //获取‘-’后的参数保存到数组param中

            n++;
        }
    }
    //将参数保存在flag中
    for (int i = 0; i < n; i++)
    {
        if (param[i] == '0')
            continue;
        if (param[i] == 'a')
        {
            flag |= A;
            continue;
        }
        else if (param[i] == 'l')
        {
            flag |= L;
            continue;
        }
        else if (param[i] == 'R')
        {
            flag |= R;
            continue;
        }
        else if (param[i] == 't')
        {
            flag |= T;
            continue;
        }
        else if (param[i] == 'r')
        {
            flag |= r;
            continue;
        }
        else if (param[i] == 'i')
        {
            flag |= I;
            continue;
        }
        else if (param[i] == 's')
        {
            flag |= S;
            continue;
        }
        else
        {
            printf("my_ls: 无法实现参数： -%c\n", param[i]);
            exit(1);
        }
    }
    //如果没有输入文件（目录）路径，用path保存当前所在目录
    if ((n + 1) == argc)
        strcpy(pathname, ".");
    //否则至少输入了一个路径，检验路径是否有效后，用path保存该路径
    else
    {
        struct stat Stat; //用stat结构体保存输入的路径的信息，用以检验路径是否存在
        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '-') //如果是参数就跳过，否则（是路径）就保存该路径
            {
                i++;
                continue;
            }
            else
                strncpy(pathname, argv[i],100);
            if (lstat(pathname, &Stat) == -1) //如果输入的路径不存在，传到错误函数，报错并退出
                my_err("lstat", __LINE__);
        }
    }
}
//错误处理函数
void my_err(const char *err_string, int line)
{
    fprintf(stderr, "line: %d ", line);
    perror(err_string);
    exit(1);
}
//-l文件打印函数
void disply_file_l(char *path)
{
    struct stat Stat;        //内含文件信息的结构体
    char time[32];           //保存转换后的时间
    struct passwd *has_user; //内含用户名的结构体
    struct group *has_group; //内含组名的结构体

    if(flag & I)//有，打印inode号
        printf("%7ld ",Stat.st_ino);

    if (lstat(path, &Stat) == -1)
        my_err("lstat", __LINE__);
    if(flag & S)
        printf("%2ld ",Stat.st_blocks/2);
    //获取并打印文件类型
    if (S_ISLNK(Stat.st_mode))
        printf("1");
    else if (S_ISREG(Stat.st_mode))
        printf("-");
    else if (S_ISDIR(Stat.st_mode))
        printf("d");
    else if (S_ISCHR(Stat.st_mode))
        printf("c");
    else if (S_ISBLK(Stat.st_mode))
        printf("b");
    else if (S_ISFIFO(Stat.st_mode))
        printf("f");
    else if (S_ISSOCK(Stat.st_mode))
        printf("s");

    //获取并打印用户权限
    if (Stat.st_mode & S_IRUSR)
        printf("r");
    else
        printf("-");
    if (Stat.st_mode & S_IWUSR)
        printf("w");
    else
        printf("-");
    if (Stat.st_mode & S_IXUSR)
        printf("x");
    else
        printf("-");

    //获取并打印同组用户的权限
    if (Stat.st_mode & S_IRGRP)
        printf("r");
    else
        printf("-");
    if (Stat.st_mode & S_IWGRP)
        printf("w");
    else
        printf("-");
    if (Stat.st_mode & S_IXGRP)
        printf("x");
    else
        printf("-");

    //获取并打印其他用户的权限
    if (Stat.st_mode & S_IROTH)
        printf("r");
    else
        printf("-");
    if (Stat.st_mode & S_IWOTH)
        printf("w");
    else
        printf("-");
    if (Stat.st_mode & S_IXOTH)
        printf("x");
    else
        printf("-");

    printf(" ");

    printf("%2ld ", Stat.st_nlink); //打印文件链接数

    has_user = getpwuid(Stat.st_uid);  //根据uid获取用户名
    printf("%-s ", has_user->pw_name); //打印用户名

    has_group = getgrgid(Stat.st_gid); //跟据gid获取组名
    printf("%-s", has_group->gr_name); //打印组名

    printf("%6ld", Stat.st_size); //打印文件大小

    strcpy(time, ctime(&Stat.st_mtime)); //获取转换后的时间
    time[strlen(time) - 1] = '\0';       //去掉换行符
    printf(" %s ", time);                //打印文件时间信息
    color_printf(path, Stat);
    printf("\n");
}
//-l,-R单文件打印函数
void disply_R_l(char *path)
{
    struct stat buf;
    if(lstat(path,&buf) == -1)
        my_err("lstat",__LINE__);


    if(S_ISDIR(buf.st_mode))
    {
        printf("\n");

        disply_dir(path);
    
        char *p = PATH;
        while(*++p);
        while(*--p != '/');
        *p = '\0';
        chdir("..");    //返回上层目录
    }
    free(path);
}
//无-l打印函数
void disply_file_only(char *path)
{
    int i, len;
    struct stat Stat;

    if(lstat(path, &Stat) == -1)
        my_err("lstat", __LINE__);

    if(flag & I)//有，打印inode号
        printf("%7ld ",Stat.st_ino);

    //如果本行不足以打印一个文件名则换行
    if (g_leave_len < g_maxlen)
    {
        printf("\n");
        g_leave_len = MAX_ROWLEN;
    }

    len = strlen(path);
    len = g_maxlen - len;

    if(flag & S)
        printf("%2ld ",Stat.st_blocks/2);
    color_printf(path, Stat);

    for(i = 0; i < len; i++)//按最长的文件名对齐，多打空格
        printf(" ");
    printf("  "); //多打两个空格
    g_leave_len -= (g_maxlen + 2);
}
//-R，无-l单文件打印函数
void disply_R_only(char *path)
{
    struct stat buf;

    if(lstat(path,&buf) == -1)
        my_err("lstat",__LINE__);

    if(S_ISDIR(buf.st_mode))
    {
        printf("\n\n");
        
        disply_dir(path);
        
        char *p = PATH;
        while(*++p);
        while(*--p != '/');
        *p = '\0';
        chdir("..");
        printf("\n");
    }
    free(path);
}
//目录打印函数
void disply_dir(char *path)
{
    DIR *dir;           //存储目录信息的结构体
    struct dirent *ptr; //存储目录下文件信息的结构体
    int count = 0;      //该目录下文件总数
    int i, j, len;

    if(flag & R)
    {
        //对打印的路径名进行修饰处理
        len = strlen(PATH);
        if(len > 0)
        {
            if(PATH[len - 1] == '/')
                PATH[len - 1] = '\0';
        }
        if((path[0] == '.' || path[0] == '/') && f == 0)
        {
            strcat(PATH,path);
            f++;
        }
        else
        {
            strcat(PATH,"/");
            strcat(PATH,path);
        }
        printf("%s:\n",PATH); 
    }

    //获取该目录下文件总数和最长文件名
    dir = opendir(path);
    if (dir == NULL)
        my_err("opendir", __LINE__);

    g_maxlen = 0;
    while ((ptr = readdir(dir)) != NULL)
    {
        if (g_maxlen < strlen(ptr->d_name))
            g_maxlen = strlen(ptr->d_name);
        count++;
    }
    closedir(dir);

    //动态分配空间，减少栈的消耗
    char **filenames = (char **)malloc(sizeof(char *) * count);
    memset(filenames, 0, sizeof(char *) * count);
    for (i = 0; i < count; i++)
    {
        filenames[i] = (char *)malloc(sizeof(char) * g_maxlen + 1);
        memset(filenames[i], 0, sizeof(char) * g_maxlen + 1);
    }

    //获取并保存该目录下所有文件名进数组
    dir = opendir(path);
    len = strlen(path);
    for (i = 0; i < count; i++)
    {
        ptr = readdir(dir);
        if (ptr == NULL)
            my_err("readdir", __LINE__);
        strcpy(filenames[i], ptr->d_name);
    }

    closedir(dir);

    //对文件名排序-t,-r
    file_sort(filenames, count);

    //切换工作目录到输入的目录下
    if (chdir(path) == -1)
        my_err("chdir", __LINE__);

    //printf("%s",filenames[0]);

    //传进打印函数进行打印
    disply(filenames,count);

    //释放空间
    if(flag & R)
        free(filenames);
    else
    {
        for(i = 0; i < count; i++)
            free(filenames[i]);
        free(filenames);
    }

    if(!(flag & L) && !(flag & R))
        printf("\n");
}
//多文件打印函数(判断-R-a-l)
void disply(char **filenames, int count)
{
    if(!(flag & R) && !(flag & A) && !(flag & L))   //q全无
    {
        for(int i = 0; i < count; i++)
            if(filenames[i][0] != '.')
                disply_file_only(filenames[i]);
    }
    else if((flag & R) && !(flag & A) && !(flag & L))//-R
    {
        for(int i = 0; i < count; i++)
            if(filenames[i][0] != '.')
                disply_R_only(filenames[i]);
        
        for(int i = 0; i < count; i++)
            if(filenames[i][0] != '.')
                disply_R_only(filenames[i]);
    }
    else if(!(flag & R) && (flag & A) && !(flag & L))//-a
    {
        for(int i = 0; i < count; i++)
            disply_file_only(filenames[i]);
    }
    else if(!(flag & R) && !(flag & A) && (flag & L))//-l
    {
        for(int i = 0; i < count; i++)
            disply_file_l(filenames[i]);
    }
    else if((flag & R) && (flag & A) && !(flag & L))//-R,-a
    {
        for(int i = 0; i < count; i++)
            disply_R_only(filenames[i]);
        
        for(int i = 0; i < count; i++)
            if((strcmp(filenames[i],".") != 0) && (strcmp(filenames[i],"..") != 0))
                disply_R_only(filenames[i]);
    }
    else if((flag & R) && !(flag & A) && (flag & L))//-R,-l
    {
        for(int i = 0; i < count; i++)
            if(filenames[i][0] != '.')
                disply_R_l(filenames[i]);
        
        for(int i = 0; i < count; i++)
            if(filenames[i][0] != '.')
                disply_R_l(filenames[i]);
    }
    else if(!(flag & R) && (flag & A) && (flag & L))//-a,-l
    {
        for(int i = 0; i < count; i++)
            disply_file_l(filenames[i]);
    }
    else                                           //-R,-a,-l
    {
        for(int i = 0; i < count; i++)
                disply_R_l(filenames[i]);

        for(int i = 0; i < count; i++)
            if((strcmp(filenames[i],".") != 0) && (strcmp(filenames[i],"..") != 0))
                disply_R_l(filenames[i]);
    }
}
//目录下多文件排序函数（-r,-t）
void file_sort(char **filenames, int count)
{
    char* temp;
    qsort(filenames, count, sizeof(char *), cmp); //调用qsort排序
}
//染色打印文件名函数
void color_printf(char *filename, struct stat buf)
{
    if (S_ISDIR(buf.st_mode)) //目录
        printf("\e[1;34m%-s\e[0m", filename);
    else if (S_ISDIR(buf.st_mode) && (buf.st_mode & 0777) == 0777) //满权限目录
        printf("\e[1;34;42m%-s\e[0m", filename);
    else if (S_ISLNK(buf.st_mode)) //符号链接
        printf("\e[1;36m%-s\e[0m", filename);
    else if (buf.st_mode & S_IXUSR || buf.st_mode & S_IXGRP || buf.st_mode & S_IXOTH) //可执行文件
        printf("\e[1;32m%-s\e[0m", filename);
    else
        printf("%-s", filename);
}
//比较函数(-r,-t)
int cmp(const void *x, const void *y)
{
    int a = 0;
    if(!(flag & r) && !(flag & T))             //只有-r,
        a = -strcmp(*(char**)y, *(char**)x);
        //因为数组里存的是字符串的地址，所以要强制类型转换成(char **)
        //然后再解引用一下才是字符串的地址
    else if(!(flag & r) && (flag & T))         //只有-t
    {
        struct stat buf_x,buf_y;
        lstat(*(char**)x,&buf_x);
        lstat(*(char**)y,&buf_y);
        a = buf_y.st_mtime - buf_x.st_mtime;
    }
    else if((flag & r) && (flag & T))          //-r,-t
    {
        struct stat buf_x,buf_y;
        lstat(*(char**)x,&buf_x);
        lstat(*(char**)y,&buf_y);
        a = -(buf_y.st_mtime - buf_x.st_mtime);
    }
    else                                       //全无
    {
        a = strcmp(*(char**)y, *(char**)x);
    }
    return a;
} 