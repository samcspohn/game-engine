#pragma once
#include <vector>
#include <set>
#include <array>
#include <map>
#include <deque>
#include <mutex>
#include <math.h>
#include "concurrency.h"
#include "serialize.h"
using namespace std;

template <typename t>
class array_heap
{
	mutex m;
	SER_HELPER()
	{
		ar &extent &avail &data &valid;
	}

public:
	int active = 0;
	struct ref
	{
		SER_HELPER()
		{
			ar &a &index;
		}

		int index;
		array_heap<t> *a;
		t *operator->()
		{
			return &(a->data.at(index));
		}
		operator int()
		{
			return index;
		}
		t &operator*()
		{
			return a->data[index];
		}
		t &data()
		{
			return a->data.at(index);
		}
	};

	t &operator[](unsigned int i)
	{
		//		if (i >= extent)
		//			throw -10;//"out of bounds";
		//if(!valid[i])
		//	throw exception("invalid");
		return data.at(i);
	}
	template <typename... types>
	ref _new(types... args)
	{
		ref ret;
		ret.a = this;
		m.lock();
		if (avail.size() > 0)
		{
			ret.index = *avail.begin();
			avail.erase(avail.begin());
			;
			valid[ret.index] = true;
			// ret.d = &data[ret.index];
			if (ret.index >= extent)
			{
				extent = ret.index + 1;
			}
			++active;
			m.unlock();
			new (&data[ret.index]) t{args...};
		}
		else
		{
			ret.index = valid.size();
			if(valid.size() == data.size()){
				data.emplace_back(args...);
			}
			// ret.d = &data.back();
			valid.emplace_back(true);
			++extent;
			++active;
			m.unlock();

		}
		return ret;
	}
	void _delete(ref& r)
	{
		if (r.a != this)
			throw;
		m.lock();
		// delete r.d;
		avail.emplace_front(r.index);
		valid[r.index] = false;
		--active;
		m.unlock();
		data[r.index].~t();
		// memset(&data[r.index],0,sizeof(t));
	}

	float density()
	{
		if (extent == 0)
			return INFINITY;
		return 1 - (float)avail.size() / (float)extent;
	}

	unsigned int size()
	{
		return extent;
	}
	vector<t> data;
	std::vector<bool> valid;

private:
	deque<uint> avail;
	uint extent = 0;
};

template <typename t>
class deque_heap
{
	mutex m;

public:
	SER_HELPER()
	{
		ar &extent &active &avail &valid &data;
	}
	int active = 0;
	struct ref
	{
		int index;
		deque_heap<t> *a;
		// t* d;
		t *operator->()
		{
			// return d;
			return &(a->data[index]);
			// return &(a->data.at(index));
		}
		operator int()
		{
			return index;
		}
		t &operator*()
		{
			return a->data[index];
		}
		t &data()
		{
			return a->data.at(index);
		}
		void _delete()
		{
			a->_delete(*this);
		}
	};
	ref getRef(int i)
	{
		return ref{i, this};
	}

	t &operator[](unsigned int i)
	{
		return data[i];
	}
	template <typename... types>
	ref _new(types... args)
	{
		ref ret;
		ret.a = this;
		m.lock();
		if (avail.size() > 0)
		{
			ret.index = *avail.begin();
			avail.erase(avail.begin());
			;
			valid[ret.index] = true;
			// ret.d = &data[ret.index];
			if (ret.index >= extent)
			{
				extent = ret.index + 1;
			}
			++active;
			m.unlock();
			new (&data[ret.index]) t{args...};
		}
		else
		{
			ret.index = valid.size();
			if(valid.size() == data.size()){
				data.emplace_back(args...);
			}
			// ret.d = &data.back();
			valid.emplace_back(true);
			++extent;
			++active;
			m.unlock();

		}
		return ret;
	}
	void _delete(ref& r)
	{
		if (r.a != this)
			throw;
		m.lock();
		// delete r.d;
		avail.emplace_front(r.index);
		valid[r.index] = false;
		--active;
		m.unlock();
		// delete (&data[r.index]);
		data[r.index].~t();
		// memset(&data[r.index],0,sizeof(t));
	}

	float density()
	{
		if (extent == 0)
			return INFINITY;
		return 1 - (float)avail.size() / (float)extent;
	}
	// unsigned int active(){
	// return this->active;
	// }
	unsigned int size()
	{
		return extent;
	}

	void clear(){
		extent = 0;
		avail.clear();
		avail.shrink_to_fit();
		valid.clear();
		valid.shrink_to_fit();
		for(auto &i : data){
			memset(&i,0,sizeof(t));
		}
		data.clear();
		data.shrink_to_fit();
	}

	deque<t> data;
	std::deque<bool> valid;
private:
	deque<uint> avail;
	uint extent = 0;
};

// template<typename t>
// class heap2{
// 	mutex m;
// 	#define bit_shift 10
// 	#define heap2BlockSize 1<<bit_shift
// 	uint type_size = sizeof(m_element);
// 	uint block_mask = ~0>>bit_shift;
// 	struct m_element {
// 		bool valid;
// 		t data;
// 	};
// 	struct ref{
// 		uint index;
// 		heap2* a;
// 		m_element* d;
// 		t* operator->() {
// 			return d->data;
// 		}
// 	};
// 	struct m_block{
// 		char* data;
// 		m_block* next;
// 		m_block* prev;
// 		m_block(){
// 			data = new char[type_size*heap2BlockSize];
// 		}
// 		m_element& operator[](uint i) {
// 			return ((m_element*)data)[i];
// 		}
// 		m_element&at(uint i){
// 			return ((m_element*)data)[i];
// 		}
// 		~m_block(){
// 			delete[] data;
// 		}
// 	};
// 	uint m_size;
// 	std::map<uint,m_block*> index1;
// 	std::map<uint,m_block*> index2;
// 	std::map<uint,m_block*>* indexing;

// 	deque<uint> avail;

// 	heap2(){
// 		m_size = 0;
// 	}
// 	t& operator[](unsigned int i){
// 		return indexing->at(i>>bit_shift)->at(i&block_mask);
// 	}
// 	ref _new(){
// 		ref ret;
// 		ret.a = this;

// 		m.lock();
// 		if (avail.size() > 0) {
// 			ret.index = *avail.begin();
// 			avail.erase(avail.begin());;
// 		}
// 		else {
// 			ret.index = m_size;
// 			if(ret.index>>bit_shift >= indexing->size()){
// 				std::map<uint,m_block*>* temp;
// 				if(indexing == &index1)
// 					temp = &index2;
// 				else
// 					temp = &index1;
// 				*temp = *indexing;
// 				m_block* new_block = new m_block();
// 				temp->insert(std::pair<uint,m_block*>(ret.index>>bit_shift,new_block));
// 				indexing = temp;
// 			}
// 			++m_size;
// 		}
// 		ret.d = indexing->at(ret.index>>bit_shift)[ret.index & block_mask];
// 		ret.d->valid = false;
// 		m.unlock();
// 		ret.d->data.t();
// 		return ret;
// 	}
// 	void _del(ref r){
// 		if (r.a != this)
// 			throw;
// 		r.d->data.~t();
// 		m.lock();
// 		r.d->valid = false;
// 		avail.emplace_front(r.index);
// 		m.unlock();
// 	}

// 	~heap2(){
// 		for(auto&i : *indexing){
// 			delete i.second;
// 		}
// 	}
// };