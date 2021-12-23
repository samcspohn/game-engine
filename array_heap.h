#pragma once
#include <vector>
#include <set>
#include <array>
#include <map>
#include <deque>
#include <mutex>
#include <math.h>
#include <atomic>
#include "concurrency.h"
#include "serialize.h"
#include <iostream>
using namespace std;

template <typename t>
class deque_heap
{
	mutex m;

public:
	t &get(int i)
	{
		return data[i];
	}
	bool getv(int i)
	{
		return valid[i];
	}

	template <typename... types>
	int _new(types... args)
	{
		// std::lock_guard<std::mutex> lck(this->m);
		int id = -1;
		m.lock();
		if (avail.size() > 0)
		{
			id = avail.back();
			avail.pop_back();
			new (&data[id]) t{args...};
			valid[id] = true;
			m.unlock();
		}
		else
		{
			id = data.size();
			data.emplace_back(args...);
			valid.emplace_back(true);
			m.unlock();
		}
		return id;
	}

	void _delete(int i)
	{
		data[i].~t();
		{
			std::lock_guard<std::mutex> lck(this->m);
			avail.push_back(i);
			valid[i] = false;
		}
		// memset(&data[r.id],0,sizeof(t));
	}

	int size()
	{
		return data.size();
	}
	int active()
	{
		return data.size() - avail.size();
	}

	void clear()
	{
		avail.clear();
		avail.shrink_to_fit();
		valid.clear();
		valid.shrink_to_fit();
		for (auto &i : data)
		{
			memset(&i, 0, sizeof(t));
		}
		data.clear();
		data.shrink_to_fit();
	}

	deque<t> data;
	deque<bool> valid;
private:
	deque<uint> avail;
};



template <typename t>
class array_heap
{
	mutex m;

public:
	t &get(int i)
	{
		return data[i];
	}
	bool getv(int i)
	{
		return valid[i];
	}

	template <typename... types>
	int _new(types... args)
	{
		// std::lock_guard<std::mutex> lck(this->m);
		int id = -1;
		m.lock();
		if (avail.size() > 0)
		{
			id = avail.back();
			avail.pop_back();
			new (&data[id]) t{args...};
			valid[id] = true;
			m.unlock();
		}
		else
		{
			id = data.size();
			data.emplace_back(args...);
			valid.emplace_back(true);
			m.unlock();
		}
		return id;
	}

	void _delete(int i)
	{
		data[i].~t();
		{
			std::lock_guard<std::mutex> lck(this->m);
			avail.push_back(i);
			valid[i] = false;
		}
		// memset(&data[r.id],0,sizeof(t));
	}

	int size()
	{
		return data.size();
	}
	int active()
	{
		return data.size() - avail.size();
	}

	void clear()
	{
		avail.clear();
		avail.shrink_to_fit();
		valid.clear();
		valid.shrink_to_fit();
		for (auto &i : data)
		{
			memset(&i, 0, sizeof(t));
		}
		data.clear();
		data.shrink_to_fit();
	}

	array_heap<t>() : m(){};
	array_heap<t>(const array_heap<t>& rhs) : m() {
		this->avail = rhs.avail;
		this->valid = rhs.valid;
		this->data = rhs.data;
	};

	// array_heap<t> operator=(const array_heap<t>& rhs) {
	// 	array_heap<t> lhs;
	// 	lhs.data = rhs.data;
	// 	lhs.valid = rhs.valid;
	// 	lhs.avail = rhs.avail;
	// 	return lhs;
	// }

	friend struct YAML::convert<array_heap<t>>;
	vector<t> data;
	deque<bool> valid;
private:
	deque<uint> avail;
};


namespace YAML
{
	template <typename T>
	struct convert<array_heap<T>>
	{
		static Node encode(const array_heap<T> &rhs)
		{
			Node node;
			node["avail"] = rhs.avail;
			node["valid"] = rhs.valid;
			node["data"] = rhs.data;
			return node;
		}

		static bool decode(const Node &node, array_heap<T> &rhs)
		{
			rhs.avail = node["avail"].as<deque<uint>>();
			rhs.valid = node["valid"].as<deque<bool>>();
			rhs.data = node["data"].as<vector<T>>();

			return true;
		}
	};

	template <typename T>
	struct convert<std::deque<T>>
	{
		static YAML::Node encode(const std::deque<T> &rhs)
		{
			YAML::Node node;
			for(auto& i: rhs){
				node.push_back(i);
			}
			return node;
		}

		static bool decode(const YAML::Node &node, std::deque<T> &rhs)
		{
			for(size_t i = 0; i < node.size(); ++i){
				rhs.push_back(node[i].as<T>());
			}
			return true;
		}
	};
}


template <typename t>
class storage
{
	mutex m;
#define chunk 256
 	vector<t*> data;
	vector<t*> d1;
	vector<t*> d2;
	atomic<vector<t*>*> r;
    deque<int> avail;
    deque<atomic<bool>> valid;

	atomic<int> extent;
	atomic<int> avail_count;
public:

    t &get(int id)
    {
        // return data[id];
        return (*r)[id / chunk][id % chunk];
    }
    bool getv(int id)
    {
        return valid[id];
    };
	template <typename... types>
	int _new(types&&... args)
    {
        int id;
        {
            lock_guard<mutex> lck(m);
            if (avail.size() > 0)
            {
                id = avail.front();
                avail.pop_front();
				--avail_count;
                valid[id] = true;
            }
            else
            {
                id = valid.size();
                valid.emplace_back(true);
				++extent;
                if (valid.size() >= data.size() * chunk)
                {
                    data.emplace_back((t*)(new char[chunk * sizeof(t)]));
					if(r == &d1){
						d2.emplace_back(data.back());
						r = &d2;
						d1.emplace_back(data.back());
					}else{
						d1.emplace_back(data.back());
						r = &d1;
						d2.emplace_back(data.back());
					}
                }
            }
        }
        new (&(*r)[id / chunk][id % chunk]) t{args...};
        return id;
    }
    void _delete(int i)
    {
        data[i / chunk][i % chunk].~t();
        {
            lock_guard<mutex> lck(m);
            valid[i] = false;
            avail.push_front(i);
			++avail_count;
        }
    }
    int size()
    {
		// lock_guard<mutex> lck(m);
        return extent;
    }
    int active()
    {
		lock_guard<mutex> lck(m);
        return extent - avail_count;
    }
	void clear(){
		for (int i = 0; i < valid.size(); i++)
        {
            if (valid[i])
            {
                _delete(i);
            }
        }
        // for (int i = 0; i < data.size(); i++)
        // {
        //     for (int j = 0; j < chunk; j++)
        //     {
        //         memset(&data[i][j], 0, sizeof(t));
        //     }
        // }
		for(auto d : data){
			delete[] (char*)d;
		}
        // data.clear();
		valid.clear();
		avail.clear();
	}
    ~storage()
    {
        clear();
    }
	storage() : m{}, extent{0}, avail_count{0}{	}

#undef chunk
#undef get
};

// template <typename t>
// class storage
// {
// #define chunk chunk
// #define _chunk 0b1111111111
// #define _get(id) data[id >> 10][(id & _chunk) * sizeof(t)]
// 	int extent = 0;
// 	mutex m;
// 	// const int _size = sizeof(t);
// 	vector<array<t, chunk>> data;
// 	vector<array<bool,chunk>> valid;
// 	vector<int> avail;
// 	bool& _getv(int id){
// 		return valid[id >> 10][id & _chunk];
// 	}

// public:
// 	t &get(int id)
// 	{
// 		// return *reinterpret_cast<t *>(&_get(id));
// 		return data[id >> 10][id & _chunk];
// 	}
// 	bool getv(int id)
// 	{
// 		return _getv(id);
// 	}
// 	int active()
// 	{
// 		return extent - avail.size();
// 	}
// 	int size()
// 	{
// 		return extent;
// 	}
// 	void clear(){
// 		data.clear();
// 		valid.clear();
// 		avail.clear();
// 	}

// 	template <typename... types>
// 	int _new(types... args)
// 	{
// 		lock_guard lck(m);
// 		int id;
// 		if (avail.size() > 0)
// 		{
// 			id = avail.back();
// 			avail.pop_back();
// 		}
// 		else
// 		{
// 			if (extent >= data.size() * chunk){
// 				data.emplace_back();
// 				valid.emplace_back();
// 			}
// 			id = extent;
// 			++extent;
// 		}
// 		new (&get(id)) t{args...};
// 		_getv(id) = true;
// 		return id;
// 	}
// 	void _delete(int id)
// 	{
// 		lock_guard lck(m);
// 		get(id).~t();
// 		avail.push_back(id);
// 		_getv(id) = false;
// 	}

// #undef chunk
// #undef get
// };

#define STORAGE storage