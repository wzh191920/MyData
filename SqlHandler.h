#pragma once
#include "sqlite3.h"
#include "points.pb.h"
#include <vector>
using std::vector;
class JobDesc;
class SqlHandler
{
public:
	SqlHandler();
	~SqlHandler();
	int Init();
	bool IsPointExist(int point_id);
	int AddType(Points::AddTypeQuery &query, Points::AddTypeReply &reply);
	int DeleteType(Points::DeleteTypeQuery &query, Points::DeleteTypeReply &reply);
	int GetTypesNum(Points::GetTypesNumReply &reply);
	int GetTypes(Points::GetTypesQuery &query, Points::GetTypesReply &reply);
	int GetTypeByID(Points::GetTypeByIDQuery &query, Points::GetTypeByIDReply &reply);
	int AddTable(Points::AddTableQuery &query, Points::AddTableReply &reply);
	int DeleteTable(Points::DeleteTableQuery &query, Points::DeleteTableReply &reply);
	int GetTablesNum(Points::GetTablesNumReply &reply);
	int GetTables(Points::GetTablesQuery &query, Points::GetTablesReply &reply);
	int GetTableByID(Points::GetTableByIDQuery &query, Points::GetTableByIDReply &reply);
	int AddPoints(Points::AddPointsQuery &query, Points::AddPointsReply &reply);
	int DeletePoint(Points::DeletePointQuery &query, Points::DeletePointReply &reply);
	//int FinishDeletePoint(Points::DeletePointQuery &query, Points::DeletePointReply &reply);
	int SearchPointsNum(Points::SearchPointsNumQuery &query, Points::SearchPointsNumReply &reply);
	int SearchPointsNumByID(Points::SearchPointsNumByIDQuery &query, Points::SearchPointsNumReply &reply);
	int SearchPoints(Points::SearchPointsQuery &query, Points::SearchPointsReply &reply);
	int SearchPointsByID(Points::SearchPointsByIDQuery &query, Points::SearchPointsReply &reply);
	//任务处理
	int GetJobs(int operation, JobDesc &job_desc, int num);
	int ClearJobs(int operation, JobDesc &job_desc);
	int GetDeletePointsByTableID(int table_id, vector<int> &point_ids);
	int ClearDeletePoints(vector<int> &point_ids);
	enum 
	{
		delete_point = 1,
		delete_table = 2,
	};

private:
	static int GetNumCallBack( void * para, int n_column, char ** column_value, char ** column_name );
	static int GetTypesCallBack( void * para, int n_column, char ** column_value, char ** column_name );
	static int GetTablesCallBack( void * para, int n_column, char ** column_value, char ** column_name );
	static int SearchPointsCallBack( void * para, int n_column, char ** column_value, char ** column_name );
	static int GetTypeByIDCallBack( void * para, int n_column, char ** column_value, char ** column_name );
	static int GetTableByIDCallBack( void * para, int n_column, char ** column_value, char ** column_name );
	static int GetJobsCallBack( void * para, int n_column, char ** column_value, char ** column_name );
	static int GetDeletePointsByTableIDCallBack( void * para, int n_column, char ** column_value, char ** column_name );

	sqlite3 *db_;
};

