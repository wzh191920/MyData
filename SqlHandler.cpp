#include "StdAfx.h"
#include "SqlHandler.h"
#include "MyDataCore.h"
#include "LogFilew.h"
#include "MyDataError.h"

#define SQL_BUFFER 512
#define QUERY_BUFFER 50000



SqlHandler::SqlHandler(void):db_(0)
{
}


SqlHandler::~SqlHandler(void)
{
	if (db_)
		sqlite3_close(db_);
}

int SqlHandler::Init()
{
	char path[MAX_PATH];
	sprintf(path, "%s/MyData.db", _config._data_path);
	return sqlite3_open(path, &db_);
}

bool SqlHandler::IsPointExist(int point_id)
{
	char sql[SQL_BUFFER];
	sprintf(sql, "select count(1) from points where id = %d and \"using\"=1", point_id);
	char *sqlerr = NULL;
	int num = 0;
	int ret = sqlite3_exec(db_, sql, GetNumCallBack, &num, &sqlerr);
	if (ret || num == -1)
	{
		return false;
	}
	else
	{
		return true;
	}
}

int SqlHandler::AddType(Points::AddTypeQuery &query, Points::AddTypeReply &reply)
{
	char sql[SQL_BUFFER]; 
	sprintf(sql, "insert into types values(NULL, \"%s\", \"%s\", \"%s\")", query.name().c_str(), 
		query.type().c_str(), query.type_names().c_str());
	char *sqlerr = NULL;
	int ret = sqlite3_exec(db_, sql, NULL, NULL, &sqlerr);
	if (ret)
	{
		LOG_ERROR_WRITE("添加类型失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_TYPE_FAIL;
	}
	reply.set_err(ret);
	return ret;
}

int SqlHandler::DeleteType(Points::DeleteTypeQuery &query, Points::DeleteTypeReply &reply)
{
	char sql[SQL_BUFFER]; 
	sprintf(sql, "select count(1) from tables where type_id = %d", query.id());
	int num = 0;
	char *sqlerr = NULL;
	int ret = sqlite3_exec(db_, sql, GetNumCallBack, &num, &sqlerr);
	if (ret || num == -1)
	{
		LOG_ERROR_WRITE("查找表相关性失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_TYPE_FAIL;
		reply.set_err(ret);
		return ret;
	}
	if (num > 0)
	{
		ret = MYDATA_SQL_TABLE_NOT_ALLOWD;
		reply.set_err(ret);
		return ret;
	}

	sprintf(sql, "delete from types where id = %d", query.id());
	ret = sqlite3_exec(db_, sql, NULL, NULL, &sqlerr);
	if (ret)
	{
		LOG_ERROR_WRITE("删除类型失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_TYPE_FAIL;
	}
	reply.set_err(ret);
	return ret;
}

int SqlHandler::GetTypesNum(Points::GetTypesNumReply &reply)
{
	char sql[SQL_BUFFER]; 
	strcpy(sql, "select count(1) from types");
	char *sqlerr = NULL;
	int num = 0;
	int ret = sqlite3_exec(db_, sql, GetNumCallBack, &num, &sqlerr);
	if (ret || num == -1)
	{
		LOG_ERROR_WRITE("获取类型数量失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_TYPE_FAIL;
		reply.set_err(ret);
	}
	else
	{
		reply.set_err(MYDATA_OK);
		reply.set_num(num);
	}
	return ret;
}

int SqlHandler::GetTypes(Points::GetTypesQuery &query, Points::GetTypesReply &reply)
{
	Points::GetTypesNumReply num_reply;
	int ret = GetTypesNum(num_reply);
	if (ret || num_reply.err())
	{
		ret = MYDATA_SQL_TYPE_FAIL;
		reply.set_err(ret);
		return ret;
	}
	reply.mutable_type_info_array()->Reserve(num_reply.num());
	char sql[SQL_BUFFER]; 
	sprintf(sql, "select id, name, type, type_names from types limit %d", query.num());
	char *sqlerr = NULL;
	ret = sqlite3_exec(db_, sql, GetTypesCallBack, &reply, &sqlerr);
	if (ret)
	{
		LOG_ERROR_WRITE("获取类型信息失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_TYPE_FAIL;
	}
	reply.set_err(ret);
	return ret;
}

int SqlHandler::GetTypesCallBack( void * para, int n_column, char ** column_value, char ** column_name )
{
	Points::GetTypesReply *reply = (Points::GetTypesReply *)para;
	auto array = reply->mutable_type_info_array();
	auto type_info = array->Add();
	type_info->set_id(atoi(column_value[0]));
	type_info->set_name(column_value[1]);
	type_info->set_type(column_value[2]);
	type_info->set_type_names(column_value[3]);
	return 0;
}

int SqlHandler::GetTypeByID(Points::GetTypeByIDQuery &query, Points::GetTypeByIDReply &reply)
{
	char sql[SQL_BUFFER]; 
	sprintf(sql, "select id, name, type, type_names from types where id = %d", query.id());
	char *sqlerr = NULL;
	reply.set_err(MYDATA_SQL_TYPE_FAIL);
	int ret = sqlite3_exec(db_, sql, GetTypeByIDCallBack, &reply, &sqlerr);
	if (ret)
	{
		LOG_ERROR_WRITE("获取指定类型信息失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_TYPE_FAIL;
		reply.set_err(ret);
	}
	return ret;
}

int SqlHandler::GetTypeByIDCallBack( void * para, int n_column, char ** column_value, char ** column_name )
{
	Points::GetTypeByIDReply *reply = (Points::GetTypeByIDReply *)para;
	if (n_column != 4)
	{
		reply->set_err(MYDATA_SQL_TYPE_FAIL);
		return MYDATA_SQL_TYPE_FAIL;
	}
	else
	{
		auto info = reply->mutable_info();
		info->set_id(atoi(column_value[0]));
		info->set_name(column_value[1]);
		info->set_type(column_value[2]);
		info->set_type_names(column_value[3]);
		reply->set_err(MYDATA_OK);
	}
	return 0;
}

int SqlHandler::GetTableByIDCallBack( void * para, int n_column, char ** column_value, char ** column_name )
{
	Points::GetTableByIDReply *reply = (Points::GetTableByIDReply *)para;
	if (n_column != 3)
	{
		reply->set_err(MYDATA_SQL_TABLE_FAIL);
		return MYDATA_SQL_TABLE_FAIL;
	}
	else
	{
		reply->set_err(MYDATA_OK);
		reply->set_id(atoi(column_value[0]));
		reply->set_type_id(atoi(column_value[1]));
		reply->set_name(column_value[2]);
	}
	return 0;
}

int SqlHandler::AddTable(Points::AddTableQuery &query, Points::AddTableReply &reply)
{
	char sql[SQL_BUFFER]; 
	sprintf(sql, "insert into tables select NULL, types.id, \"%s\" from types where types.id = %d", query.name().c_str(), query.type_id());
	char *sqlerr = NULL;
	int ret = sqlite3_exec(db_, sql, NULL, NULL, &sqlerr);
	if (ret)
	{
		LOG_ERROR_WRITE("添加表失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_TABLE_FAIL;
	}
	reply.set_err(ret);
	return ret;
}
int SqlHandler::DeleteTable(Points::DeleteTableQuery &query, Points::DeleteTableReply &reply)
{
	char sql[SQL_BUFFER]; 
	sprintf(sql, "select count(1) from points where table_id = %d", query.id());
	int num = 0;
	char *sqlerr = NULL;
	int ret = sqlite3_exec(db_, sql, GetNumCallBack, &num, &sqlerr);
	if (ret || num == -1)
	{
		LOG_ERROR_WRITE("查找点相关性失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_TABLE_FAIL;
		reply.set_err(ret);
		return ret;
	}
	if (num > 0)
	{
		sprintf(sql, "insert into jobs values(NULL,%d,NULL,%d);update points set \"using\"=0 where table_id=%d", delete_table, query.id(), query.id());
		ret = sqlite3_exec(db_, sql, NULL, NULL, &sqlerr);
		if (ret)
		{
			LOG_ERROR_WRITE("删除表失败, %s", sqlerr);
			sqlite3_free(sqlerr);
			ret = MYDATA_SQL_TABLE_FAIL;
			reply.set_err(ret);
			return ret;
		}
	}
	sprintf(sql, "delete from tables where id = %d", query.id());
	ret = sqlite3_exec(db_, sql, NULL, NULL, &sqlerr);
	if (ret)
	{
		LOG_ERROR_WRITE("删除表失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_TABLE_FAIL;
	}
	reply.set_err(ret);
	return ret;
}

int SqlHandler::GetTablesNum(Points::GetTablesNumReply &reply)
{
	char sql[SQL_BUFFER]; 
	strcpy(sql, "select count(1) from tables");
	char *sqlerr = NULL;
	int num = 0;
	int ret = sqlite3_exec(db_, sql, GetNumCallBack, &num, &sqlerr);
	if (ret || num == -1)
	{
		LOG_ERROR_WRITE("获取表数量失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_TABLE_FAIL;
		reply.set_err(ret);
	}
	else
	{
		reply.set_err(MYDATA_OK);
		reply.set_num(num);
	}
	return ret;
}
int SqlHandler::GetNumCallBack( void * para, int n_column, char ** column_value, char ** column_name )
{
	int *num = (int *)para;
	if (n_column == 1)
	{
		*num = atoi(column_value[0]);
	}
	else
	{
		*num = -1;
	}
	return 0;
}
int SqlHandler::GetTables(Points::GetTablesQuery &query, Points::GetTablesReply &reply)
{
	Points::GetTablesNumReply num_reply;
	int ret = GetTablesNum(num_reply);
	if (ret || num_reply.err())
	{
		ret = MYDATA_SQL_TABLE_FAIL;
		reply.set_err(ret);
		return ret;
	}
	reply.mutable_table_info_array()->Reserve(num_reply.num());
	char sql[SQL_BUFFER];
	sprintf(sql, "select id, type_id, name from tables limit %d", query.num());
	char *sqlerr = NULL;
	ret = sqlite3_exec(db_, sql, GetTablesCallBack, &reply, &sqlerr);
	if (ret)
	{
		LOG_ERROR_WRITE("获取表信息失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_TABLE_FAIL;
	}
	reply.set_err(ret);
	return ret;
}
int SqlHandler::GetTableByID(Points::GetTableByIDQuery &query, Points::GetTableByIDReply &reply)
{
	char sql[SQL_BUFFER];
	sprintf(sql, "select id, type_id, name from tables where id = %d", query.id());
	reply.set_err(MYDATA_SQL_TABLE_FAIL);
	char *sqlerr = NULL;
	int ret = sqlite3_exec(db_, sql, GetTableByIDCallBack, &reply, &sqlerr);
	if (ret)
	{
		LOG_ERROR_WRITE("获取表信息失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_TABLE_FAIL;
		reply.set_err(ret);
	}
	return ret;
}
int SqlHandler::GetTablesCallBack( void * para, int n_column, char ** column_value, char ** column_name )
{
	Points::GetTablesReply *reply = (Points::GetTablesReply*)para;
	auto array = reply->mutable_table_info_array();
	auto table_info = array->Add();
	table_info->set_id(atoi(column_value[0]));
	table_info->set_type_id(atoi(column_value[1]));
	table_info->set_name(column_value[2]);
	return 0;
}

int SqlHandler::AddPoints(Points::AddPointsQuery &query, Points::AddPointsReply &reply)
{
	auto points = query.mutable_point_names();
	char sql[SQL_BUFFER]; 
	int ret;
	sprintf(sql, "insert into points select NULL, tables.id, ?, 1 from tables where tables.id = %d", query.table_id());
	sqlite3_exec(db_,"begin;",0,0,0);  
	sqlite3_stmt *stmt; 
	sqlite3_prepare_v2(db_, sql, strlen(sql), &stmt, 0); 
	int num = 0;
	for(int i=0; i<points->size(); ++i)  
	{   
		sqlite3_reset(stmt);  
		sqlite3_bind_text(stmt, 1, points->Get(i).c_str(), points->Get(i).length(), SQLITE_STATIC);
		ret = sqlite3_step(stmt);
		if (ret == SQLITE_DONE)
			num++;  
	}  
	sqlite3_finalize(stmt);  
	sqlite3_exec(db_,"commit;",0,0,0);  
	if (num != points->size())
		ret = MYDATA_DATA_PAR_SUC;
	else
		ret = MYDATA_OK;
	reply.set_err(ret);
	reply.set_suc_num(num);
	return MYDATA_OK;
}

int SqlHandler::DeletePoint(Points::DeletePointQuery &query, Points::DeletePointReply &reply)
{
	char sql[SQL_BUFFER]; 
	sprintf(sql, "insert into jobs values(NULL, %d, NULL, %d);delete from points where id=%d", delete_point, query.id(), query.id());
	char *sqlerr = NULL;
	int ret = sqlite3_exec(db_, sql, NULL, NULL, &sqlerr);
	if (ret)
	{
		LOG_ERROR_WRITE("删除标签点失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_POINT_FAIL;
	}
	reply.set_err(ret);
	return ret;
}

// int SqlHandler::FinishDeletePoint(Points::DeletePointQuery &query, Points::DeletePointReply &reply)
// {
// 	char sql[SQL_BUFFER]; 
// 	sprintf(sql, "delete from jobs where operation = %d and desc2 = %d", delete_point, query.id());
// 	char *sqlerr = NULL;
// 	int ret = sqlite3_exec(db_, sql, NULL, NULL, &sqlerr);
// 	if (ret)
// 	{
// 		LOG_ERROR_WRITE("结束删除标签点任务失败, %s", sqlerr);
// 		sqlite3_free(sqlerr);
// 		ret = MYDATA_SQL_POINT_FAIL;
// 	}
// 	reply.set_err(ret);
// 	return ret;
// 
// }

int SqlHandler::SearchPointsNum(Points::SearchPointsNumQuery &query, Points::SearchPointsNumReply &reply)
{
	char sql[SQL_BUFFER] = {"select count(1) from points, tables where points.table_id = tables.id and \"using\"=1"}; 
	char sql_where[SQL_BUFFER] = {""};
	if (query.table_name() != "%")
	{
		sprintf(sql_where, " and tables.name like \"%s\"", query.table_name().c_str());
		strcat(sql, sql_where);
	}
	if (query.point_name() != "%" )
	{
		sprintf(sql_where, " and points.name like \"%s\"", query.point_name().c_str());
		strcat(sql, sql_where);
	}
	int num = 0;
	char *sqlerr = NULL;
	int ret = sqlite3_exec(db_, sql, GetNumCallBack, &num, &sqlerr);
	if (ret || num == -1)
	{
		LOG_ERROR_WRITE("查询标签点数量失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_POINT_FAIL;
		reply.set_err(ret);
	}
	else
	{
		reply.set_err(MYDATA_OK);
		reply.set_num(num);
	}
	return ret;
}

int SqlHandler::SearchPoints(Points::SearchPointsQuery &query, Points::SearchPointsReply &reply)
{
	char sql[SQL_BUFFER] = {"select points.id, table_id, points.name from points, tables where points.table_id = tables.id and \"using\"=1"}; 
	char sql_where[SQL_BUFFER] = {""};
	if (query.table_name() != "%")
	{
		sprintf(sql_where, " and tables.name like \"%s\"", query.table_name().c_str());
		strcat(sql, sql_where);
	}
	if (query.point_name() != "%" )
	{
		sprintf(sql_where, " and points.name like \"%s\"", query.point_name().c_str());
		strcat(sql, sql_where);
	}
	sprintf(sql_where, " and points.id > %d limit %d", query.from_id(), QUERY_BUFFER);
	strcat(sql, sql_where);
	auto array = reply.mutable_point_info_array();
	array->Reserve(QUERY_BUFFER);
	char *sqlerr = NULL;
	int ret = sqlite3_exec(db_, sql, SearchPointsCallBack, &reply, &sqlerr);
	if (ret)
	{
		LOG_ERROR_WRITE("查询标签点信息失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_POINT_FAIL;
		reply.set_err(ret);
		return ret;
	}
	if (array->size() < QUERY_BUFFER)
	{
		reply.set_err(MYDATA_SQL_POINT_END);
	}
	else
	{
		reply.set_err(ret);
	}
	return ret;
}
int SqlHandler::SearchPointsCallBack( void * para, int n_column, char ** column_value, char ** column_name )
{
	Points::SearchPointsReply *reply = (Points::SearchPointsReply*)para;
	auto array = reply->mutable_point_info_array();
	auto point_info = array->Add();
	point_info->set_id(atoi(column_value[0]));
	point_info->set_table_id(atoi(column_value[1]));
	point_info->set_name(column_value[2]);
	return 0;
}

int SqlHandler::SearchPointsNumByID(Points::SearchPointsNumByIDQuery &query, Points::SearchPointsNumReply &reply)
{
	char sql[SQL_BUFFER];
	sprintf(sql, "select count(1) from points, tables where points.table_id = tables.id and points.table_id = %d and \"using\"=1", query.table_id()); 
	char sql_where[SQL_BUFFER] = {""};
	if (query.point_name() != "%" )
	{
		sprintf(sql_where, " and points.name like \"%s\"", query.point_name().c_str());
		strcat(sql, sql_where);
	}
	int num = 0;
	char *sqlerr = NULL;
	int ret = sqlite3_exec(db_, sql, GetNumCallBack, &num, &sqlerr);
	if (ret || num == -1)
	{
		LOG_ERROR_WRITE("查询标签点数量失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_POINT_FAIL;
		reply.set_err(ret);
	}
	else
	{
		reply.set_err(MYDATA_OK);
		reply.set_num(num);
	}
	return ret;
}

int SqlHandler::SearchPointsByID(Points::SearchPointsByIDQuery &query, Points::SearchPointsReply &reply)
{
	char sql[SQL_BUFFER];
	sprintf(sql, "select  points.id, table_id, points.name from points, tables where points.table_id = tables.id and points.table_id = %d and \"using\"=1", query.table_id()); 
	char sql_where[SQL_BUFFER] = {""};
	if (query.point_name() != "%" )
	{
		sprintf(sql_where, " and points.name like \"%s\"", query.point_name().c_str());
		strcat(sql, sql_where);
	}
	sprintf(sql_where, " and points.id > %d limit %d", query.from_id(), QUERY_BUFFER);
	strcat(sql, sql_where);
	auto array = reply.mutable_point_info_array();
	array->Reserve(QUERY_BUFFER);
	char *sqlerr = NULL;
	int ret = sqlite3_exec(db_, sql, SearchPointsCallBack, &reply, &sqlerr);
	if (ret)
	{
		LOG_ERROR_WRITE("查询标签点信息失败, %s", sqlerr);
		sqlite3_free(sqlerr);
		ret = MYDATA_SQL_POINT_FAIL;
		reply.set_err(ret);
		return ret;
	}
	if (array->size() < QUERY_BUFFER)
	{
		reply.set_err(MYDATA_SQL_POINT_END);
	}
	else
	{
		reply.set_err(ret);
	}
	return ret;
}

int SqlHandler::GetJobs(int operation, JobDesc &job_desc, int num)
{
	char sql[SQL_BUFFER];
	sprintf(sql, "select desc, desc2 from jobs where operation=%d limit %d", operation, num);
	return sqlite3_exec(db_, sql, GetJobsCallBack, &job_desc, NULL);
}

int SqlHandler::GetJobsCallBack( void * para, int n_column, char ** column_value, char ** column_name )
{
	JobDesc *job_desc = (JobDesc*)para;
	if (column_value[0])
		job_desc->desc.push_back(column_value[0]);
	if (column_value[1])
		job_desc->desc2.push_back(atoi(column_value[1]));
	return 0;
}

int SqlHandler::ClearJobs(int operation, JobDesc &job_desc)
{
	char sql[SQL_BUFFER];
	sqlite3_exec(db_,"begin;",0,0,0); 
	sqlite3_stmt *stmt; 
	do 
	{
		if (operation == delete_point || operation == delete_table)
		{
			sprintf(sql, "delete from jobs where operation=%d and desc2=?", operation);
			sqlite3_prepare_v2(db_, sql, strlen(sql), &stmt, 0); 
			for (vector<int>::iterator iter = job_desc.desc2.begin(); iter != job_desc.desc2.end(); ++iter)
			{
				sqlite3_reset(stmt);
				sqlite3_bind_int(stmt, 1, *iter);
				sqlite3_step(stmt);
			}
		}
		else
			break;
		sqlite3_finalize(stmt);  
	} while (0);

	sqlite3_exec(db_,"commit;",0,0,0);  
	return 0;
}

int SqlHandler::GetDeletePointsByTableID(int table_id, vector<int> &point_ids)
{
	char sql[SQL_BUFFER];
	sprintf(sql, "select id from points where table_id=%d limit %d", table_id, JOB_QUERY_BUFFER);
	return sqlite3_exec(db_, sql, GetDeletePointsByTableIDCallBack, &point_ids, NULL);
}

int SqlHandler::GetDeletePointsByTableIDCallBack( void * para, int n_column, char ** column_value, char ** column_name )
{
	vector<int> *point_ids = (vector<int>*)para;
	point_ids->push_back(atoi(column_value[0]));
	return 0;
}

int SqlHandler::ClearDeletePoints(vector<int> &point_ids)
{
	char sql[SQL_BUFFER] = {"delete from points where id=?"}; 
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db_, sql, strlen(sql), &stmt, 0); 
	sqlite3_exec(db_,"begin;",0,0,0); 
	for (vector<int>::iterator iter = point_ids.begin(); iter != point_ids.end(); ++iter)
	{
		sqlite3_reset(stmt);
		sqlite3_bind_int(stmt, 1, *iter);
		sqlite3_step(stmt);
	}
	sqlite3_finalize(stmt);  
	sqlite3_exec(db_,"commit;",0,0,0);
	return 0;
}