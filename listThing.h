#pragma once
#include <deque>
#include <plf_list.h>
#include <atomic>

template<typename t>
class l {
    struct elem{
        typename plf::list<t>::iterator i;
        size_t id;
    };
    struct bookmark{
        typename plf::list<elem>::iterator i;
        int count;
    };

    plf::list<t> data;
    plf::list<elem> ids;
    std::deque<bookmark> bookmarks;


    typename plf::list<elem>::iterator insert(const t& input){

    }
};
