#ifndef DATA_PROVIDERS_H
#define DATA_PROVIDERS_H

#include <map>
#include <string>
#include <vector>


template<typename T>
struct DataValuesProvider2
{
    DataValuesProvider2()
    {
    }

    int set_dimensions(int count_times, int count_xy)
    {
        int status = 1;  // default error status
        if (m_count_times == 0 && m_count_xy == 0)
        {
            m_count_times = count_times;
            m_count_xy = count_xy;
            status = 0;
        }
        if (m_count_times == count_times && m_count_xy == count_xy)
        {
            status = 0;
        }
        return status;
    }

    int set_data(const std::string& var_name, std::vector<double>& data)
    {
        m_data[var_name] = std::move(data);
        return 0;
    }
    int get_xy_count()
    {
        return m_count_xy;
    }
    void erase(const std::string& var_name)
    {
        m_data.erase(var_name);
    }

    T* get_timestep(const std::string& var_name, int timeIndex)
    {
        if (m_data.find(var_name) == m_data.end())
        {
            if (m_count_times == 0 && m_count_xy == 0)
            {

            }
            throw std::invalid_argument("Data for " + var_name + " not found");
        }

        auto const index = get_index(timeIndex, 0);
        return &m_data[var_name][index];
    }

private:
    std::map<std::string, std::vector<T>> m_data;
    std::vector<T> m_array;
    int m_count_times = 0;
    int m_count_xy = 0;

    int get_index(int timeIndex, int xyIndex)
    {
        const auto index = m_count_xy * timeIndex + xyIndex;
        return index;
    }

};



template<typename T>
struct DataValuesProvider3
{
    DataValuesProvider3()
    {
    }

    int set_dimensions(int count_times, int count_layer, int count_xy)
    {
        int status = 1;  // default error status
        if (m_count_times == 0 && m_count_layer == 0 && m_count_xy == 0)
        {
            m_count_times = count_times;
            m_count_layer = count_layer;
            m_count_xy = count_xy;
            status = 0;
        }
        if (m_count_times == count_times && m_count_layer == count_layer && m_count_xy == count_xy)
        {
            status = 0;
        }
        return status;
    }

    int set_data(const std::string& var_name, std::vector<double>& data)
    {
        m_data[var_name] = std::move(data);
        return 0;
    }
    void erase(const std::string& var_name)
    {
        m_data.erase(var_name);
    }
    int get_xy_count()
    {
        return m_count_xy;
    }

    T* get_timestep(const std::string& var_name, int timeIndex, int layerIndex)
    {
        if (m_data.find(var_name) == m_data.end())
        {
            if (m_count_times == 0 && m_count_xy == 0)
            {

            }
            throw std::invalid_argument("Data for " + var_name + " not found");
        }

        auto const index = get_index(timeIndex, layerIndex, 0);
        return &m_data[var_name][index];
    }

private:
    std::map<std::string, std::vector<T>> m_data;
    int m_count_times = 0;
    int m_count_layer = 0;
    int m_count_xy = 0;

    inline int get_index(int timeIndex, int layerIndex, int xyIndex)
    {
        int index = m_count_xy * m_count_layer * timeIndex + m_count_xy * layerIndex + xyIndex;
        return index;
    }
};


template<typename T>
struct DataValuesProvider4
{
    DataValuesProvider4()
    {
    }

    int set_dimensions(int count_times, int count_layer, int numSed, int count_xy)
    {
        int status = 1;  // default error status
        if (m_count_times == 0 && m_count_layer == 0 && m_count_xy == 0)
        {
            m_count_times = count_times;
            m_count_layer = count_layer;
            int m_numSed = 0;
            m_count_xy = count_xy;
            status = 0;
        }
        if (m_count_times == count_times && m_count_layer == count_layer && m_count_xy == count_xy)
        {
            status = 0;
        }
        return status;
    }

    int set_data(const std::string& var_name, std::vector<double>& data)
    {
        m_data[var_name] = std::move(data);
        return 0;
    }
    int get_xy_count()
    {
        return m_count_xy;
    }
    void erase(const std::string& var_name)
    {
        m_data.erase(var_name);
    }

    T* get_timestep(const std::string& var_name, int timeIndex, int layerIndex, int sedIndex)
    {
        if (m_data.find(var_name) == m_data.end())
        {
            if (m_count_times == 0 && m_count_xy == 0)
            {

            }
            throw std::invalid_argument("Data for " + var_name + " not found");
        }

        auto const index = get_index(timeIndex, layerIndex, sedIndex, 0);
        return &m_data[var_name][index];
    }

private:
    std::map<std::string, std::vector<T>> m_data;
    int m_count_times = 0;
    int m_count_layer = 0;
    int m_numSed = 0;
    int m_count_xy = 0;

    inline int GetIndex(int timeIndex, int layerIndex, int sedIndex, int xyIndex)
    {
        int index = m_count_xy * m_count_layer * m_numSed * timeIndex + m_count_xy * m_numSed * layerIndex + m_count_xy * sedIndex + xyIndex;
        return index;
    }
};


#endif DATA_PROVIDERS_H
