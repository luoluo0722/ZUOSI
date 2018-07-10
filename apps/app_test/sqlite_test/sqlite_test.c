#include <stdio.h>
#include <sqlite3.h>

/*
*首先利用sqlite3_open打开一个数据库，mdatabase.db，如果不存在就创建它
*然后创建一个测试表
*然后再表里面插入一些记录
*最后查询这个表，并打印出来
*/
int main()
{
	sqlite3 *db;
	int result;
	char *errmsg = NULL;
	result = sqlite3_open("mdatabase.db",&db);  //打开数据库mdatabase.db，如果不存在就创建它
	if(result != SQLITE_OK)
	{
		printf("Can't open datebase\n%s\n",sqlite3_errmsg(db));
		return -1;
	}
	result = sqlite3_exec(db,"create table Mytable(ID,name,sex,age)",0,0, &errmsg);
	//创建一个测试表，表名My1table，有两个字段ID和name
	if(result != SQLITE_OK)
	{
		printf("warning:Create table fail! May table Mytable already.\n");
//		return -1;
	}
	result = sqlite3_exec(db,"insert into Mytable values(1,'student','M',20)",0,0,&errmsg);
	//插入一条记录
	if(result != SQLITE_OK) {
		printf("Can't insert into datebase：%d\n",errmsg);
	}

	result = sqlite3_exec(db,"insert into Mytable values(2,'teacher','W',49)",0,0,&errmsg);
	if(result != SQLITE_OK) {
		printf("Can't insert into datebase：%d\n",errmsg);
	}

	result = sqlite3_exec(db,"insert into Mytable values(3,'professer','M',30)",0,0,&errmsg);
	if(result != SQLITE_OK) {
		printf("Can't insert into datebase：%d\n",errmsg);
	}
	result = sqlite3_exec(db,"select * from Mytable",0,0,&errmsg);
	//查询数据库
	int nrow = 0, ncol = 0, i,j;
	char **table;
	
	printf("\n");
	sqlite3_get_table(db,"select * from Mytable",&table,&nrow,&ncol,&errmsg);
	printf("row:%d col:%d\n",nrow,ncol);
	printf("The result of querying is:\n");
	for (i = 0; i < nrow+1; i++) {
		for (j = 0; j < ncol; j++) {
			printf("% -9s	",table[i*ncol+j]);
		}
		printf("\n");
	}

	sqlite3_free_table(table);
	
	sqlite3_close(db); //关闭数据库
	return 0;
}

