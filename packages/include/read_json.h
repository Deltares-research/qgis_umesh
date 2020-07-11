#ifndef __READ_JSON_H
#define __READ_JSON_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;
using namespace boost;
using namespace property_tree;

class READ_JSON
{
public:
    READ_JSON(string);
    long get(string, vector<string> &);
    long get(string, vector<double> &);
    long get(string, vector<vector<vector<double>>> &);
    string get_filename();
    void prop_get_json(boost::property_tree::iptree &, const string, vector<string> &);
    void prop_get_json(boost::property_tree::iptree &, const string, vector<double> &);
    void prop_get_json(boost::property_tree::iptree &, const string, vector<vector<vector<double>>> &);

private:
    string m_filename;
    boost::property_tree::iptree m_ptrtree;
};
#endif  // __READ_JSON_H
