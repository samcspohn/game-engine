#pragma once
#include <vector>
#include <set>
#include <array>
#include <map>
#include <deque>
#include <mutex>
#include <queue>
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

#include <tbb/concurrent_priority_queue.h>
#include <tbb/concurrent_vector.h>
template <typename t>
class storage
{
	mutex m;
#define chunk_size 256
	// struct _chunk {
	// 	t* m_data;
	// 	_chunk() {
	// 		m_data = (t*)(new char[chunk_size * sizeof(t)]);
	// 	}
	// 	~_chunk() {
	// 		delete[] (char*)m_data;
	// 	}
	// 	t& operator[](int i){
	// 		return m_data[i];
	// 	}
	// 	t& at(int i) {
	// 		return m_data[i];
	// 	}
	// };
 	tbb::concurrent_vector<t> data;
    tbb::concurrent_priority_queue<int, std::greater<int>> avail;
    tbb::concurrent_vector<atomic<bool>> valid;

	atomic<int> extent;
	// atomic<int> _extent;
	// tbb::atomic<int> _extent;
public:

    t &get(int id)
    {
        return data[id];
    }
    bool getv(int id) 
    {
        return valid[id];
    };
	template <typename... types>
	int _new(types&&... args)
    {
		// lock_guard<mutex> lck(m);
        int id;
		{
			if(avail.try_pop(id)){
				valid[id] = true;
				{
					lock_guard<mutex> lck(m);
					if(id >= extent){
						extent = id + 1;
					}
				}
			}else {
				lock_guard<mutex> lck(m);
				// id = valid.size();
				id = (valid.emplace_back(true) - valid.begin());
				data.emplace_back();
				extent = id + 1;
				// if (valid.size() >= data.size() * chunk_size)
				// {
				// 	data.emplace_back(std::make_shared<_chunk>());
				// }
			}
		}
        new (&data[id]) t{args...};
        return id;
    }
	void compress() {
			while(extent > 0 && !valid[extent - 1])
				--extent;
	}
    void _delete(int i)
    {
		// lock_guard<mutex> lck(m);
        data[i].~t();
		valid[i] = false;
		avail.push(i);

    }
    int size()
    {
		lock_guard<mutex> lck(m);
        return extent;
    }
    int active()
    {
		lock_guard<mutex> lck(m);
        return valid.size() - avail.size();
    }
	void clear(){
		for (int i = 0; i < valid.size(); i++)
        {
            if (valid[i])
            {
                _delete(i);
            }
			memset(&data[i],0,sizeof(t));
        }
		data.clear();
		valid.clear();
		avail.clear();
	}
    ~storage()
    {
        clear();
    }
	storage() : m{}, extent{0} { }

#undef chunk
#undef get
};

template <typename t>
class storage2
{
	mutex m;
#define chunk_size 256
	// set<int> avail;
	deque<int> avail;
	atomic<int> avail_id{0};
	struct _chunk {
		int m_id;
		vector<int> m_avail;
		t* m_data;
		_chunk(int id) : m_id{id} {
			m_data = (t*)(new char[chunk_size * sizeof(t)]); 
		}
		~_chunk() {
			delete[] (char*)m_data;
		}
		t& operator[](int i){
			return m_data[i];
		}
		t& at(int i) noexcept {
			return m_data[i];
		}
		void _new_id(int& id, set<int>& avail){
			id = m_avail.back();
			m_avail.pop_back();
			if(m_avail.size() == 0){
				avail.erase(m_id);
			}
		}
		void _delete_id(int& id, set<int>& avail){
			m_avail.push_back(id);
			avail.emplace(this->m_id);
		}
	};
 	vector<_chunk*> data;
	vector<_chunk*> d1;
	vector<_chunk*> d2;
	atomic<vector<_chunk*>*> r;
    // tbb::concurrent_priority_queue<int, std::greater<int>> avail;
    deque<atomic<bool>> valid;

	atomic<int> extent;
	// atomic<int> _extent;
	// tbb::atomic<int> _extent;
public:

    t &get(int id)
    {
        return (*r)[id / chunk_size]->at(id % chunk_size);
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

			if(avail_id > 0){
				int _id = avail_id.fetch_add(-1);
				if(_id <= 0){
            		lock_guard<mutex> lck(m);
					avail_id = 0;
					id = valid.size();
					valid.emplace_back(true);
					avail.emplace_back(0);
					++extent;
					if (valid.size() >= data.size() * chunk_size)
					{
						data.emplace_back(new _chunk(data.size()
						));
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
				}else{
					id = avail[_id - 1];
					valid[id] = true;
				}
				
            }
            else
            {
            	lock_guard<mutex> lck(m);
                id = valid.size();
                valid.emplace_back(true);
				avail.emplace_back(0);
				++extent;
                if (valid.size() >= data.size() * chunk_size)
                {
                    data.emplace_back(new _chunk(data.size()));
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
        new (&(*r)[id / chunk_size]->at(id % chunk_size)) t{args...};
        return id;
    }
	void compress() {
			// while(extent > 0 && !valid[extent - 1])
			// 	--extent;
	}
    void _delete(int i)
    {
        data[i / chunk_size]->at(i % chunk_size).~t();
		valid[i] = false;
		avail[avail_id.fetch_add(1)] = i;
    }
    int size()
    {
        return extent;
    }
    int active()
    {
        return valid.size() - avail_id;
    }
	void clear(){
		for (int i = 0; i < valid.size(); i++)
        {
            if (valid[i])
            {
                _delete(i);
            }
        }
		d1.clear();
		d2.clear();
		for(auto& i : data){
			delete i;
		}
		data.clear();
		valid.clear();
		avail.clear();
		avail_id = 0;
	}
    ~storage2()
    {
        clear();
    }
	storage2() : m{}, extent{0} { }

#undef chunk
#undef get
};


#define STORAGE storage