//
// Programmer: Jan Mooiman
// Date: 14 April 2021
// Email: jan.mooiman@deltares.nl
//
#include <cstdlib>
#include <vector>
#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>

#include "perf_timer.h"

using namespace std;

int TestTimer::object_count = 0;
TestTimer* TestTimer::obj;  // Initialize static member of class System

TestTimer::TestTimer(const std::string& name, const std::string& fileName, const std::string& functionName, const int lineNumber) :
	m_name(name), m_filename(fileName), m_functionname(functionName), m_line_number(lineNumber)
{
	if (object_count == 0)
	{
		object_count = 1;
		m_level = 0;
		timing.push_back(new Timing(m_name, m_filename, m_functionname, m_line_number));
		timing[0]->level = m_level;
		timing[0]->running = false;
		timing[0]->elapse_time = 0.0;
	}
}
TestTimer::~TestTimer()
{
	if (object_count == 1)
	{
		for (int i = 0; i < timing.size() - 1; i++)
		{
			delete timing[i];
		}
	}
}
int TestTimer::get_count()
{
	return object_count;
}
void TestTimer::clear()
{
	if (object_count == 1)
	{
		object_count = 0;
		m_level = 0;
		for (int i = 1; i < timing.size() - 1; i++)  // skip the first one; it is the root and has level zero
		{
			delete timing[i];
		}
	}
	timing.resize(1);
}
void TestTimer::start(const std::string& name, const std::string& fileName, const std::string& functionName, const int lineNumber)
{
	bool found = false;
	for (int j = 0; j < timing.size(); j++)
	{
		if (timing[j]->name == name)
		{
			timing[j]->start = std::chrono::high_resolution_clock::now();
			timing[j]->running = true;
			timing[j]->nr_calls += 1;
			m_level += 1;
			found = true;
			break;
		}
	}
	if (!found) 
	{
		timing.push_back(new Timing(name, fileName, functionName, lineNumber));
		m_level += 1;
		timing[timing.size() - 1]->level = m_level;
		timing[timing.size() - 1]->running = true;
		timing[timing.size() - 1]->elapse_time = 0.0;
	}
}
void TestTimer::stop(const std::string& name)
{
	for (int i = 0; i < timing.size(); i++)
	{
		if (timing[i]->name == name)
		{
			m_level -= 1;
			timing[i]->running = false;
			timing[i]->stop = std::chrono::high_resolution_clock::now();
			std::chrono::high_resolution_clock::duration d = timing[i]->stop - timing[i]->start;
			timing[i]->elapse_time += d.count() * (double)std::chrono::high_resolution_clock::duration::period::num / std::chrono::high_resolution_clock::duration::period::den;
		}
	}
}
void TestTimer::log(string out_file)
{
	if (timing.size() > 1)
	{
		std::ofstream os;
		os.open(out_file);
		for (int i = 0; i < timing.size(); i++)
		{
			if (timing[i]->running)
			{
				timing[i]->running = false;
				timing[i]->stop = std::chrono::high_resolution_clock::now();
				std::chrono::high_resolution_clock::duration d = timing[i]->stop - timing[i]->start;
				timing[i]->elapse_time += d.count() * (double)std::chrono::high_resolution_clock::duration::period::num / std::chrono::high_resolution_clock::duration::period::den;
			}
		}

		// determine string length, needed for nice layout
		size_t name_strlen = 4;
		size_t file_strlen = 4;
		size_t function_strlen = 8;
		for (int i = 0; i < timing.size(); i++)
		{
			name_strlen = max(name_strlen, timing[i]->name.length());
			std::string file_str = timing[i]->filename.substr(timing[i]->filename.find_last_of("/\\") + 1);
			file_strlen = max(file_strlen, file_str.length());
			function_strlen = max(function_strlen, timing[i]->functionname.length());
		}
		size_t total_strlen = 5 + 3 +  // level
			5 + 3 +  // calls
			10 + 3 +  // time[s]
			name_strlen + 3 +  // name
			file_strlen + 3 +  // file
			function_strlen + 3 +  //function name
			6;  // line number

		os << std::string(total_strlen, '=') << std::endl;
		os << "level" << "   " << "calls" << "   "
			<< right << setw(10) << " time[s]"
			<< "   "
			<< left << std::setw(name_strlen) << "name"
			<< "   "
			<< left << std::setw(file_strlen) << "file"
			<< "   "
			<< left << std::setw(function_strlen) << "function"
			<< "   "
			<< right << std::setw(6) << "line"
			<< std::endl;
		os << std::string(total_strlen, '-') << std::endl;
		for (int i = 1; i < timing.size(); i++)  // skip the first one; it is the root and has level zero
		{
			std::string file_str = timing[i]->filename.substr(timing[i]->filename.find_last_of("/\\") + 1);
			os << std::setw(5) << timing[i]->level
				<< "   "
				<< std::setw(5) << timing[i]->nr_calls
				<< "   "
				<< std::setw(10) << fixed << setprecision(3) << timing[i]->elapse_time
				<< "   "
				<< left << std::setw(name_strlen) << timing[i]->name
				<< "   "
				<< left << std::setw(file_strlen) << file_str
				<< "   "
				<< left << std::setw(function_strlen) << timing[i]->functionname
				<< "   "
				<< right << std::setw(6) << timing[i]->line_number
				<< std::endl;
		}
		os << std::string(total_strlen, '=') << std::endl;
		os.close();
	}
}
