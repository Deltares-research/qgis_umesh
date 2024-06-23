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

int TestTimer::object_count = 0;
TestTimer* TestTimer::obj = nullptr;  // Initialize static member of class System

TestTimer::TestTimer(const std::string& name, const std::string& fileName, const std::string& functionName, const int lineNumber) :
    m_name(name), m_filename(fileName), m_functionname(functionName), m_line_number(lineNumber)
{
    if (object_count == 0)
    {
        object_count = 1;
        m_level = 0;
        timing.emplace_back(m_name, m_filename, m_functionname, m_line_number);
        timing.back().level = m_level;
        timing.back().running = false;
        timing.back().elapse_time = 0.0;
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
    }
    timing.resize(1);
}
void TestTimer::start(const std::string& name, const std::string& fileName, const std::string& functionName, const int lineNumber)
{
    bool found = false;
    for (auto& t : timing)
    {
        if (t.name == name)
        {
            t.start = std::chrono::high_resolution_clock::now();
            t.running = true;
            t.nr_calls += 1;
            m_level += 1;
            found = true;
            break;
        }
    }
    if (!found)
    {
        timing.emplace_back(Timing(name, fileName, functionName, lineNumber));
        m_level += 1;
        timing.back().level = m_level;
        timing.back().running = true;
        timing.back().elapse_time = 0.0;
    }
}
void TestTimer::stop(const std::string& name)
{
    for (auto& t : timing)
    {
        if (t.name == name)
        {
            m_level -= 1;
            t.running = false;
            t.stop = std::chrono::high_resolution_clock::now();
            std::chrono::high_resolution_clock::duration d = t.stop - t.start;
            t.elapse_time += d.count() * (double)std::chrono::high_resolution_clock::duration::period::num / std::chrono::high_resolution_clock::duration::period::den;
        }
    }
}
void TestTimer::log(std::string out_file)
{
    if (timing.size() > 1)
    {
        std::ofstream os;
        os.open(out_file);
        for (auto& t : timing)
        {
            if (t.running)
            {
                t.running = false;
                t.stop = std::chrono::high_resolution_clock::now();
                std::chrono::high_resolution_clock::duration d = t.stop - t.start;
                t.elapse_time += d.count() * (double)std::chrono::high_resolution_clock::duration::period::num / std::chrono::high_resolution_clock::duration::period::den;
            }
        }

        // determine string length, needed for nice layout
        size_t name_strlen = 4;
        size_t file_strlen = 4;
        size_t function_strlen = 8;
        for (auto& t : timing)
        {
            std::string s(2*t.level, '-');
            std::string t_name = s + t.name ;
            name_strlen = std::max(name_strlen, t_name.length());
            std::string file_str = t.filename.substr(t.filename.find_last_of("/\\") + 1);
            file_strlen = std::max(file_strlen, file_str.length());
            function_strlen = std::max(function_strlen, t.functionname.length());
        }
        size_t const total_strlen = 5 + 3 +  // level
            5 + 3 +  // calls
            10 + 3 +  // time[s]
            name_strlen + 3 +  // name
            file_strlen + 3 +  // file
            function_strlen + 3 +  //function name
            6;  // line number

        os << std::string(total_strlen, '=') << std::endl;
        os << "level" << "   " << "calls" << "   "
            << std::right << std::setw(10) << " time[s]"
            << "   "
            << std::left << std::setw(name_strlen) << "name"
            << "   "
            << std::left << std::setw(file_strlen) << "file"
            << "   "
            << std::left << std::setw(function_strlen) << "function"
            << "   "
            << std::right << std::setw(6) << "line"
            << std::endl;
        os << std::string(total_strlen, '-') << std::endl;
        for (auto& t : timing)
        {
            if (t.name == "root") 
            { 
                continue;  // skip the first one
            }
            std::string file_str = t.filename.substr(t.filename.find_last_of("/\\") + 1);
            std::string s(2 * (t.level-1), ' ');
            std::string t_name = s + t.name;
            os << std::setw(5) << t.level
                << "   "
                << std::setw(5) << t.nr_calls
                << "   "
                << std::setw(10) << std::fixed << std::setprecision(3) << t.elapse_time
                << "   "
                << std::left << std::setw(name_strlen) << t_name
                << "   "
                << std::left << std::setw(file_strlen) << file_str
                << "   "
                << std::left << std::setw(function_strlen) << t.functionname
                << "   "
                << std::right << std::setw(6) << t.line_number
                << std::endl;
        }
        os << std::string(total_strlen, '=') << std::endl;
        os.close();
    }
}
