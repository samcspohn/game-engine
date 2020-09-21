#include <glm/glm.hpp>
#include <deque>
#include <vector>
#include <array>
#include <map>
#include <list>
#include "game_object.h"
#include "collision.h"
#include "Component.h"
#include "Transform.h"
#include "terrain.h"
#include <mutex>
#include <iostream>
#include <tbb/concurrent_unordered_set.h>
using namespace std;
using namespace glm;
#define MAX_OCT_SIZE 8

struct oct_node;
struct oct_obj;

struct collider_shape_base
{

    virtual void update(_transform &t, vec3 dim)
    {
    }
    virtual int type()
    {
        return -1;
    }
    virtual bool collide(collider_shape_base *c)
    {
        return false;
    }
};

struct boxCollider : collider_shape_base
{
    OBB o;
    int type()
    {
        return 0;
    }
    bool collide(collider_shape_base *b)
    {
        if (b->type() == 0)
        {
            return TestOBBOBB(((boxCollider *)b)->o, this->o);
        }
        return false;
    }
    void update(_transform &t, vec3 dim)
    {
        glm::vec3 sc = t.scale * dim;
        glm::mat3 rot = glm::toMat3(t.rotation);
        o.c = t.position;
        o.u[0] = rot * glm::vec3(1, 0, 0);
        o.u[1] = rot * glm::vec3(0, 1, 0);
        o.u[2] = rot * glm::vec3(0, 0, 1);
        o.e = sc;
    }
};

deque_heap<boxCollider> boxes;

struct collisionShapePointer
{
    virtual collider_shape_base *get() { return 0; }
    virtual void _delete() {}
};
struct boxShapePointer : collisionShapePointer
{
    deque_heap<boxCollider>::ref r;
    collider_shape_base *get()
    {
        return &*r;
    }
    boxShapePointer()
    {
        r = boxes._new();
    }
    ~boxShapePointer()
    {
        r._delete();
    }
};

atomic<ull> collider_id_generator;
class collider2 : public component
{
public:
    COPY(collider2);
    ull id;
    collisionShapePointer *shape;
    tbb::spin_mutex lock;
    oct_node *node = 0;
    vec3 dim = vec3(1);
    int layer;
    void setLayer(int l);
    collider2() = default;
    collider2(const collider2 &c)
    {
        this->dim = c.dim;
        this->layer = c.layer;
        node = 0;
        shape = 0;
    }
    void onStart();
    void update();
    void onDestroy();
};

set<int> collisionLayers;
map<int, set<int>> collisionGraph;

struct oct_obj
{
    // tbb::spin_mutex lock;
    AABB2 aabb;
    // oct_node *n;
    collider_shape_base *c;
    collider2 *comp;
    _transform *t;
    vec3 dim;
    int layer;
    bool collider_shape_updated;
    // oct_obj() : lock(){}
    // oct_obj(const oct_obj& o) : lock(){
    //     memcpy(this,&o,sizeof(oct_obj));
    // }
    // oct_obj& operator=(const oct_obj& o){
    //     memcpy(this,&o,sizeof(oct_obj));
    //     return *this;
    // }
};

// void lockObjs(oct_obj &o1, oct_obj &o2){
//     while (true)
//     {
//         o1.lock.lock();
//         if(o2.lock.try_lock())
//             return;
//         else
//         o1.lock.unlock();
//         tbb::this_tbb_thread::yield();
//     }
// }

void colObjs(oct_obj &o1, oct_obj &o2)
{
    if (o1.comp != o2.comp && testAABB(o1.aabb, o2.aabb) &&
        [&] {
            if (!o1.collider_shape_updated)
            {
                o1.c->update(*o1.t, o1.comp->dim);
                o1.collider_shape_updated = true;
            }
            if (!o2.collider_shape_updated)
            {
                o2.c->update(*o2.t, o2.comp->dim);
                o2.collider_shape_updated = true;
            }
            return o1.c->collide(o2.c);
        }())
    {
        // lockObjs(o1,o2);
        ((component *)o1.comp)->transform->gameObject->collide(((component *)o2.comp)->transform->gameObject, vec3(0), vec3(0));
        ((component *)o2.comp)->transform->gameObject->collide(((component *)o1.comp)->transform->gameObject, vec3(0), vec3(0));
        // o1.lock.unlock();
        // o2.lock.unlock();
    }
}

struct oct_node;
tbb::concurrent_unordered_set<oct_node *> populated_nodes;
tbb::concurrent_unordered_set<oct_node *> unpopulated_nodes;

struct oct_node
{
    mutex lock;
    oct_node *parent = 0;
    tbb::concurrent_unordered_map<ull, oct_obj> objects;
    AABB2 aabb;
    vec3 c;
    bool leaf;
    bool populated = false;
    array<oct_node *, 8> children = array<oct_node *, 8>();

    int getChild(float x, float y, float z)
    {
        return (x > c.x) * 4 + (y > c.y) * 2 + (z > c.z);
    }

    void push_obj(oct_obj o, ull id)
    {
        if (!populated)
        {
            populated_nodes.insert(this);
            populated = true;
        }
        objects.emplace(pair<ull, oct_obj>(id, o));
        // o.comp->lock.lock();
        objects.at(id).comp->node = this;
        // o.comp->lock.unlock();
        // objects.back().n = this;
    }

    // void calc_aabb(int childId, oct_node *parent){

    // }
    oct_node() {}
    oct_node(int childId, oct_node *parent)
    {
        bool x = childId & 0b100;
        bool y = childId & 0b010;
        bool z = childId & 0b001;
        vec3 p_c = (parent->aabb.max + parent->aabb.min) / 2.f;
        {
            if (x) // left
            {
                this->aabb.min.x = p_c.x;
                this->aabb.max.x = parent->aabb.max.x;
            }
            else
            {
                this->aabb.min.x = parent->aabb.min.x;
                this->aabb.max.x = p_c.x;
            }
        }
        {
            if (y) // up
            {
                this->aabb.min.y = p_c.y;
                this->aabb.max.y = parent->aabb.max.y;
            }
            else
            {
                this->aabb.min.y = parent->aabb.min.y;
                this->aabb.max.y = p_c.y;
            }
        }
        {
            if (z) // forward
            {
                this->aabb.min.z = p_c.z;
                this->aabb.max.z = parent->aabb.max.z;
            }
            else
            {
                this->aabb.min.z = parent->aabb.min.z;
                this->aabb.max.z = p_c.z;
            }
        }
        this->leaf = true;
        this->c = this->aabb.getCenter();
        this->parent = parent;
    }

    oct_obj get(ull id)
    {
        try
        {
            // lock.lock();
            // oct_obj ret = objects.at(id);
            // lock.unlock();
            return objects.at(id);;
        }
        catch (exception e)
        {
            cout << "could not get obj. does not exist" << endl;
            throw e;
        }
    }
    void set_obj(oct_obj o, ull id)
    {
        try
        {
            // lock.lock();
            objects.at(id) = o;
            // lock.unlock();
        }
        catch (exception e)
        {
            cout << "could not set obj. does not already exist" << endl;
            throw e;
        }
    }
    bool hasChildren()
    {
        for (auto &i : children)
        {
            if (i != 0)
                return true;
        }
        return false;
    }
    bool isLeaf()
    {
        return !hasChildren();
    }
    oct_obj remove_object(ull &id)
    {
        try{

        lock.lock();
        oct_obj ret = objects.at(id);
        objects.unsafe_erase(id);
        if (objects.size() == 0)
        {
            // populated_nodes.unsafe_erase(this);
            unpopulated_nodes.insert(this);
            populated = false;
        }
        lock.unlock();
        return ret;
        }catch(exception e){
            cout << "could not remove object. does not exist" << endl;
            throw e;
        }
    }
    // void insert_child(const oct_obj &o)
    // {
    //     vec3 o_c = o.aabb.getCenter();
    //     int child = getChild(o_c.x, o_c.y, o_c.z);
    //     if (children[child] == 0)
    //     {
    //         // lock.lock();
    //         if (children[child] == 0)
    //             children[child] = new oct_node(child, this);
    //         // lock.unlock();
    //     }
    //     children[child]->insert(o);
    // }

    // void _insert_child(const oct_obj &o, const ull &id)
    // {
    //     vec3 o_c = o.aabb.getCenter();
    //     int child = getChild(o_c.x, o_c.y, o_c.z);
    //     if (children[child] == 0)
    //     {
    //         if (children[child] == 0)
    //             children[child] = new oct_node(child, this);
    //     }
    //     children[child]->insert(o, id);
    // }

    // void insert_intermediate(const oct_obj &o, const ull &id)
    // {
    //     if (o.aabb.sits(c))
    //     {
    //         // lock.lock();
    //         push_obj(o, id);
    //         lock.unlock();
    //     }
    //     else
    //     {
    //         vec3 o_c = o.aabb.getCenter();
    //         int child = getChild(o_c.x, o_c.y, o_c.z);
    //         if (children[child] == 0)
    //         {
    //             if (children[child] == 0)
    //                 children[child] = new oct_node(child, this);
    //         }
    //         lock.unlock();
    //         children[child]->insert(o, id);
    //     }
    // }
    void insert(const oct_obj &o, const ull &id)
    {
        lock.lock();
        if(aabb.fits(o.aabb) && o.aabb.sits(c)){
            push_obj(o,id);
            lock.unlock();
        }else{
            vec3 o_c = o.aabb.getCenter();
            int child = getChild(o_c.x, o_c.y, o_c.z);
            if (children[child] == 0)
            {
                if (children[child] == 0)
                    children[child] = new oct_node(child, this);
            }
            lock.unlock();
            children[child]->insert(o, id);
        }



        // lock.lock();
        // if (hasChildren())
        // {
        //     insert_intermediate(o, id);
        // }
        // else
        // { //leaf
        //     // lock.lock();
        //     if (isLeaf())
        //     {
        //         if (objects.size() < MAX_OCT_SIZE)
        //         {
        //             push_obj(o, id);
        //         }
        //         else
        //         {

        //             map<ull, oct_obj> temp;
        //             for (auto &i : objects)
        //             {
        //                 if (i.second.aabb.sits(c))
        //                     temp.emplace(i);
        //                 else
        //                 {
        //                     _insert_child(i.second, i.first);
        //                     leaf = false;
        //                 }
        //             }
        //             // leaf = false;
        //             objects.clear();
        //             for (auto &i : temp)
        //             {
        //                 push_obj(i.second, i.first);
        //             }

        //             if (o.aabb.sits(c))
        //             {
        //                 push_obj(o, id);
        //             }
        //             else
        //             {
        //                 _insert_child(o, id);
        //             }
        //             if (objects.size() == 0)
        //             {
        //                 populated_nodes.unsafe_erase(this);
        //                 populated = false;
        //             }
        //         }
        //         lock.unlock();
        //     }
        //     else
        //     {
        //         insert_intermediate(o, id);
        //     }
        // }
    }

    void getObjects(map<int, list<oct_obj *>> &objects_in_layers)
    {
        for (auto &i : objects)
        {
            objects_in_layers[i.second.layer].push_back(&i.second);
        }
        for (oct_node *o : children)
        {
            if (o != 0)
            {
                o->getObjects(objects_in_layers);
            }
        }
    }
    void collideObjects()
    {

        map<int, list<oct_obj *>> my_objects_in_layers;
        map<int, list<oct_obj *>> objects_in_layers;

        for (auto &i : objects)
        {
            my_objects_in_layers[i.second.layer].push_back(&i.second);
        }
        getObjects(objects_in_layers);

        for (auto &v : my_objects_in_layers)
        {
            list<oct_obj *> &a = v.second;
            for (int x : collisionGraph[v.first])
            {
                list<oct_obj *> &b = objects_in_layers[x];
                for (auto &ae : a)
                {
                    for (auto &be : b)
                    {
                        colObjs(*ae, *be);
                    }
                }
            }
        }
    }
    // void collideObjects()
    // {
    //     if (objects.size() == 0)
    //         return;
    //     for (int i = 0; i < objects.size() - 1; i++)
    //     {
    //         for (int j = i + 1; j < objects.size(); j++)
    //         {
    //             colObjs(objects[i], objects[j]);
    //         }
    //     }
    // }
};
tbb::spin_mutex phys_lock;

oct_node *oct_root = 0;

float get_oct_size(AABB2 a)
{
    vec3 v;
    v.x = std::max(abs(a.max.x), abs(a.min.x));
    v.y = std::max(abs(a.max.y), abs(a.min.y));
    v.x = std::max(abs(a.max.z), abs(a.min.z));
    float p = std::max(v.x, std::max(v.y, v.z));
    p = std::ceil(std::log2(p));
    p = std::pow(2, p);
    return p;
}
void oct_insert(const oct_obj &o, ull id)
{

    // if (!oct_root->aabb.fits(o.aabb))
    // {
    //     phys_lock.lock();
    //     if (!oct_root->aabb.fits(o.aabb))
    //     {
    //         AABB2 new_root_aabb;
    //         float p = get_oct_size(o.aabb);
    //         map<ull, oct_obj> oct_root_objs = oct_root->objects;
    //         while (p >= new_root_aabb.max.x)
    //         {
    //             new_root_aabb = AABB2(oct_root->aabb.max.x * 2);
    //             oct_node *new_root = new oct_node();
    //             new_root->aabb = new_root_aabb;
    //             int x, y, z;
    //             for (x = 0; x < 2; x++)
    //             {
    //                 for (y = 0; y < 2; y++)
    //                 {
    //                     for (z = 0; z < 2; z++)
    //                     {
    //                         int child = x * 4 + y * 2 + z;
    //                         if (oct_root->children[child] != 0)
    //                         {
    //                             new_root->children[child] = new oct_node(child, new_root);
    //                             new_root->children[child]->children[~child & 0b111] = oct_root->children[child];
    //                             new_root->children[child]->leaf = false;
    //                             oct_root->children[child]->parent = new_root->children[child];
    //                             new_root->leaf = false;
    //                         }
    //                     }
    //                 }
    //             }
    //             populated_nodes.unsafe_erase(oct_root);
    //             delete oct_root;
    //             oct_root = new_root;
    //         }
    //         for (auto &i : oct_root_objs)
    //         {
    //             oct_root->insert(i.second,i.first);
    //         }
    //     }
    //     phys_lock.unlock();
    // }
    // phys_lock.unlock();

    oct_root->insert(o, id);
}

void collider2::onStart()
{
    oct_obj o;
    o.comp = this;
    o.layer = this->layer;
    o.t = &(transform->_T.data());
    o.dim = dim;
    glm::dvec3 sc = dvec3(transform->getScale()) * dvec3(dim);
    glm::mat3 rot = glm::toMat3(transform->getRotation());
    vec3 r = vec3(1) * (float)length(sc) * 1.5f;
    o.aabb = AABB2(transform->getPosition() - r, transform->getPosition() + r);
    shape = new boxShapePointer();
    o.c = shape->get();
    id = collider_id_generator.fetch_add(1);
    // phys_lock.lock();
    oct_insert(o, id);
    // phys_lock.unlock();
}

void collider2::update()
{
    // phys_lock.lock();
    // phys_lock.lock();
    // data->n->lock.lock(); // make sure o doesnt move
    // phys_lock.unlock();

    phys_lock.lock();
    // lock.lock();
    // phys_lock.lock();
    oct_obj o = node->get(id);
    // phys_lock.unlock();
    o.collider_shape_updated = false;
    glm::dvec3 sc = dvec3(o.t->scale) * dvec3(o.dim);
    vec3 r = vec3((float)length(sc) * 1.5f);
    AABB2 a = AABB2(o.t->position - r, o.t->position + r);
    o.dim = dim;
    o.layer = layer;
    o.aabb = a;
    // // o.c->update(*o.t, dim);

    if (node->aabb.fits(o.aabb))
    {
        if (o.aabb.sits(node->c))
        {
            node->set_obj(o, id);
            // lock.unlock();
        }
        else
        {
            node->remove_object(id);
            node->insert(o, id);
            // lock.unlock();
        }
    }
    else
    {
        node->remove_object(id);
        // obj.n->lock.unlock();
        // o is invalid

        oct_node *_node = node;
        lock.unlock();
        if (_node == 0)
        {
            oct_root->insert(o, id);
        }
        else
        {
            while (true)
            {
                if (_node->parent == 0 || _node->aabb.fits(o.aabb))
                    break;
                else
                    _node = _node->parent;
            }
            _node->insert(o, id);
        }
        // oct_insert(obj);
    }
    phys_lock.unlock();
    // phys_lock.unlock();
}

void collider2::onDestroy()
{
    phys_lock.lock();
    // data->n->lock.lock();
    node->remove_object(id);
    // o.n->lock.unlock();
    phys_lock.unlock();

    delete shape;
}
void collider2::setLayer(int l)
{
    this->layer = l;
    if (this->node != 0)
    {
        node->lock.lock();
        node->objects.at(id).layer = l;
        node->lock.unlock();
    }
}

void cleanUpColliderNodes()
{
    for (oct_node *n : unpopulated_nodes)
    {
        if (!n->populated)
        {
            populated_nodes.unsafe_erase(n);
        }
    }
    unpopulated_nodes.clear();
}
void updateColliders()
{
    // tbb::parallel_for_each(populated_nodes.range(), [](oct_node *n) { oct_update(n); });
}

void calcCollisions()
{
    // int size = populated_nodes.size();
    // for (auto &i : populated_nodes)
    // {
    //     i->collideObjects();
    // }
    tbb::parallel_for_each(populated_nodes.range(), [](oct_node *n) { n->collideObjects(); });

    // n->collideObjects();
    // for (auto &c : n->children)
    // {
    //     if (c != 0)
    //     {
    //         calcCollisions(c);
    //     }
    // }
}