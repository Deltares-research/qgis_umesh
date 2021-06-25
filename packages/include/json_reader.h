#ifndef __JSON_READER_H
#define __JSON_READER_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace boost;
using namespace property_tree;

class JSON_READER
{
public:
    JSON_READER(std::string);
    long get(std::string, std::vector<std::string> &);
    long get(std::string, std::vector<double> &);
    long get(std::string, std::vector<std::vector<std::vector<double>>> &);
    std::string get_filename();
    void prop_get_json(boost::property_tree::iptree &, const std::string, std::vector<std::string> &);
    void prop_get_json(boost::property_tree::iptree &, const std::string, std::vector<double> &);
    void prop_get_json(boost::property_tree::iptree &, const std::string, std::vector<std::vector<std::vector<double>>> &);

private:
    std::string m_filename;
    boost::property_tree::iptree m_ptrtree;
};
#endif  // __JSON_READER_H
