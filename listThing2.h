#pragma once
#include <boost/container/flat_map.hpp>
#include <set>
#include <deque>
#include <map>


template<typename key_type, typename element_type>
class mapThing{
    struct node {
        int count = 0;
        node* left = 0;
        node* right = 0;

        key_type key;
        element_type element;
    };
    node* root = 0;
    void insert(key_type _key, element_type _element){
        if(root == 0){
            this->root = new node();
            this->root.element = _element;
            this->root.count = 1;
        }else{
            node* curr = root;
            while(true){
                if(curr->key < _key){
                    if(curr->left != 0)
                        curr = curr->left;
                    else{
                        curr->left = new node();
                        curr->left->element = _element;
                        curr->left->count = 1;
                    }
                }else if(curr->key > _key){
                    curr = curr->right;
                    if(curr->right != 0)
                        curr = curr->right;
                    else{
                        curr->right = new node();
                        curr->right->element = _element;
                        curr->right->count = 1;
                    }
                }
            }
        }
    }
};


template<typename t>
class listThing2{
    public:
    struct node{
        unsigned int id;
        node* prev;
        node* next;
        // node* previous_enabled;
        // node* next_enabled;
        t value;// = {0};
        node(const t& val) : value(val){}
    };
//    struct iterator{
//        node* n;
//    };
    unsigned int extent = 0;
    std::map<unsigned int, node*> accessor;
    std::set<unsigned int> avail;
    std::deque<listThing2<t>::node> data;
    listThing2(){
    }
    node* operator[](unsigned int i) {
        auto it = accessor.begin();
        advance(it,i);
        return it->second;
//        return accessor.nth(i)->second;
    }
    unsigned int insert(const t& value){
        if(avail.size() > 0){
            unsigned int index = *avail.begin();
            data[index].value = value;
            accessor[index] = &(data[index]);
            avail.erase(avail.begin());
            data[index].id = index;
            if(index > 0){
                data[index].next = data[index - 1].next;
                data[index - 1].next = &data[index];
                data[index].prev = &data[index - 1];
                if(index < extent){
                    data[index].next->prev = &data[index];
                }else{
                    data[index].next = &data[index];
                    ++extent;
                }
            }
            return index;
        }else{
            unsigned int index = data.size();
            data.push_back(node(value));
            accessor[index] = &(data[index]);
//            data.back().value = value;
            data[index].next = &data[index];
            data[index].id = index;
            ++extent;
            if(index > 0){
                data[index].prev = &data[index - 1];
                data[index - 1].next = &data[index];
            }else{
                data[index].prev = &data[index];
            }
            return index;
        }
    }
    void erase(node* node){
        // node->value.~t();
        if(node->next != node)
            node->prev->next = node->next;
        else{
            node->prev->next = node->prev->next;
            extent -= node->id - node->prev->id;
        }

        if(node->prev != node)
            node->next->prev = node->prev;
        else
            node->next->prev = node->next->prev;

        accessor.erase(node->id);
        avail.insert(node->id);
    }
    void erase(unsigned int id){
        erase(&data[id]);
    }
};
