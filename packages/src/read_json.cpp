
/* 
A json file parser based on boost libraries. It creates a tree structure and parses the json file.
prop_get templated function does the work of extracting the numbers(integer, float or double) from the strings
using c++11 regular expression functionality.
*/

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <type_traits>
#include <regex>
#include <vector>
#include <iostream>
#include <string>
#include "json_reader.h"
//#include "QMessageBox"

struct Getter
{
    template<typename T, class Ptree>
    static void prop_get_json(Ptree &pt, const std::string & data, std::vector<T> & results)
    {
        try {
            // first strip key from data, ex. data=data.boundary.nodeid, chapter==data.boundary, key==nodeid
            size_t i = data.find_last_of(".");
            std::string chapter = data.substr(0, i);
            std::string key = data.substr(i + 1);
            auto child = pt.get_child(chapter);
            for (auto& p : child)
            {
                std::string a = p.first.data();
                std::string b = p.second.data();
                if (boost::iequals(p.first.data(), key))
                {
                    if (!p.second.data().empty())
                    {
                        T val = p.second.get_value<T>();
                        results.push_back(val);
                    }
                }
            }
            if (results.size() != 0) { return; }

            for (auto& pvv : child)
            {
                for (auto& pv : pvv.second)
                {
                    for (auto& p : pv.second)
                    {
                        std::string a = p.first.data();
                        std::string b = p.second.data();
                        std::string c = pv.first.data();
                        std::string d = pv.second.data();
                        if (p.second.data() != "" && boost::iequals(pv.first.data(), key))
                        {
                            T val = p.second.get_value<T>();
                            results.push_back(val);
                        }
                    }
                }
            }
            if (results.size() != 0) { return; }

            for (auto& pv : child)
            {
                for (auto& p : pv.second)
                {
                    //string a = p.first.data();
                    //string b = p.second.data();
                    //string c = pv.first.data();
                    if (p.first.data() != ""  && boost::iequals(p.first.data(), key))
                    {
                        T val = p.second.get_value<T>();
                        results.push_back(val);
                    }
                    else if (pv.first.data() != "" && boost::iequals(pv.first.data(), key))
                    {
                        T val = p.second.get_value<T>();
                        results.push_back(val);
                    }
                }
            }
            if (results.size() != 0) { return; }
        }

        catch (const ptree_error &e)
        {
            (void)e;  // to prevent compiler warning
            //cout << e.what() << endl;
            //QString msg = QString::fromStdString(e.what()).trimmed();
            //QMessageBox::warning(0, "JSON_READER::prop_get_json", QString("%1").arg(msg));
        }
    }
    template<typename T, class Ptree>
    static void prop_get_json(Ptree &pt, const std::string & data, std::vector< std::vector< std::vector<T>>> & results)
    {
        try
        {
            std::vector<double> vec;
            std::vector<std::vector<double>> coord;

            size_t i = data.find_last_of(".");
            std::string chapter = data.substr(0, i);
            std::string key = data.substr(i + 1);
            for (auto & v1 : pt.get_child(chapter))
            {
                for (auto& p1 : v1.second)
                {
                    if (boost::iequals(p1.first, key))
                    {
                        for (auto& p2 : p1.second)
                        {
                            if (boost::iequals(p2.first, "x") || boost::iequals(p2.first, "y"))
                            {
                                for (auto& p : p2.second)
                                {
                                    std::string str = p.second.data();
                                    vec.push_back(stod(str));
                                }
                                coord.push_back(vec);
                                vec.clear();
                            }
                        }
                        results.push_back(coord);
                        coord.clear();
                    }
                }
            }
        }
        catch (const boost::property_tree::ptree_error& e)
        {
            (void)e;  // to prevent compiler warning
            //std::cout << e.what() << endl;
        }
    }
};

JSON_READER::JSON_READER(std::string file_json)
{
    m_filename = file_json;
    try
    {
        boost::property_tree::read_json(file_json, m_ptrtree);
    }
    catch (const ptree_error &e)
    {
        (void)e;  // to prevent compiler warning
        //cout << e.what() << endl;
        //QString msg = QString::fromStdString(e.what()).trimmed();
        //QMessageBox::warning(0, "JSON_READER::JSON_READER", QString("%1").arg(msg));
    }
}

long JSON_READER::get(std::string data, std::vector<std::string> & strJsonResults)
{
    strJsonResults.clear();
    Getter::prop_get_json(m_ptrtree, data, strJsonResults);
    return 0;
}
long JSON_READER::get(std::string data, std::vector<double> & strJsonResults)
{
    strJsonResults.clear();
    Getter::prop_get_json(m_ptrtree, data, strJsonResults);
    return 0;
}
long JSON_READER::get(std::string data, std::vector < std::vector< std::vector<double>>> & strJsonResults)
{
    strJsonResults.clear();
    Getter::prop_get_json(m_ptrtree, data, strJsonResults);
    return 0;
}
std::string JSON_READER::get_filename()
{
    return this->m_filename;
}
