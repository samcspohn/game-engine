#pragma once
#include <deque>
#include "plf_list.h"
#include <atomic>


#define LIST_THING_STORAGE plf::list
template<typename t>
class listThing {
public:
    struct elem;
    struct elemRef{
        typename LIST_THING_STORAGE<elem>::iterator it;
    };
    struct elem {
        elemRef* ref;
        typename LIST_THING_STORAGE<t>::iterator it;
        size_t id;
        elem(){}
        elem(size_t _id, const typename LIST_THING_STORAGE<t>::iterator& _it, elemRef* _ref) : id(_id), it(_it),ref(_ref) {};
    };
    struct bookmark{
        typename LIST_THING_STORAGE<elem>::iterator it;
        int count;
        bookmark(typename LIST_THING_STORAGE<elem>::iterator _it, int _count) : it(_it), count(_count){};
    };

    LIST_THING_STORAGE<t> data;
    LIST_THING_STORAGE<elem> ids;
    std::deque<bookmark> bookmarks;
    atomic<size_t> idGen;

    listThing(){
        elemRef* r = new elemRef();
        ids.push_back(elem(idGen++,data.end(),r));
        ids.front().ref->it = ids.begin();
        for(int i = 0; i < concurrency::numThreads; ++i)
            bookmarks.push_back(bookmark(ids.begin(),0));
    }

    void rebalance(){
        float dist = (float)data.size() / (float)bookmarks.size();
        bool isNotBalanced = true;

        for(int i = bookmarks.size() - 1; i > 0; --i){
            int to = bookmarks[i].count - (int)dist;
            for(int j = 0; j < to;j++){
                ++bookmarks[i].it;
                --bookmarks[i].count;
                ++bookmarks[i - 1].count;
            }
        }
        for(int i = 0; i < bookmarks.size() - 1; ++i){
            int to = bookmarks[i].count - (int)dist;
            for(int j = 0; j < to;j++){
                --bookmarks[i + 1].it;
                --bookmarks[i].count;
                ++bookmarks[i + 1].count;
            }
        }
    }

    typename LIST_THING_STORAGE<elem>::iterator insert(const t& input){
        data.push_back(input);
        ids.back() = elem(ids.back().id,--data.end(), ids.back().ref);//set back to new element

        elemRef* r = new elemRef();// add end to iterators
        ids.push_back(elem(idGen++,data.end(), r));
        ids.back().ref->it = --ids.end();

        (--(--ids.end()))->ref->it = (--(--ids.end()));//fix old back()'s ref

        bookmarks.back().count++;

        return --(--(ids.end()));
    }
    int findBookmark(int id){
        for(int i = 0; i < bookmarks.size() - 1; ++i){
            if(bookmarks[i].it->id <= id && bookmarks[i + 1].it->id > id )
                return i;
        }
        return bookmarks.size() - 1;
    }
    void erase(typename LIST_THING_STORAGE<elem>::iterator itr){
        if(itr->id == ids.front().id)
            cout << endl << "begin" << endl;
        int bookmarkIndex = findBookmark(itr->id);
        --bookmarks[bookmarkIndex].count;
        typename LIST_THING_STORAGE<t>::iterator d = data.erase(itr->it); // get iterator after erased
        typename LIST_THING_STORAGE<elem>::iterator i = itr;
        // repair itrs surrounding erased element;
//        --d; //go back one
//        --i; i->it = d; // set id itr to new itr
//        ++d;
//        ++i;++i; i->it = d;//good / done

        delete itr->ref;
        size_t id = itr->id;
        i = ids.erase(itr);


        --i;
        if(bookmarkIndex > 0 && i->id == bookmarks[bookmarkIndex - 1].it->id)
            bookmarks[bookmarkIndex].it = i;
        i->ref->it = i;


        ++i;
        if(id == bookmarks[bookmarkIndex].it->id){ // questionable
            bookmarks[bookmarkIndex].it = i;
            for(int j = bookmarkIndex + 1; j < concurrency::numThreads && bookmarks[j].it->id == id; ++j)
                bookmarks[j].it = i;
        }
        for(int j = bookmarkIndex + 1; j < concurrency::numThreads && bookmarks[j].it->id == i->id; ++j){
            bookmarks[j].it = i;
        }
        i->ref->it = i;//update reference
    }
};
