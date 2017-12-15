#pragma once
#include <vector>
#include <string>
#include "LogFilew.h"
#include "MyDataConfig.h"

extern MyDataConfig _config;
struct JobDesc
{
	std::vector<std::string> desc;
	std::vector<int> desc2;
};
#define JOB_QUERY_BUFFER 500	

