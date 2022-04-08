#include "bullet/src/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "bullet/src/btBulletDynamicsCommon.h"
#include "bullet/src/BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h"
#include "bullet/src/BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h"
#include "bullet/src/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.h"
#include <vector>
#include <unordered_set>
#include "concurrency.h"
#include "iostream"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Mesh.h"

#ifndef PHYSICS
#define PHYSICS
class _physicsManager
{

    btITaskScheduler *taskSchedular;
    btDynamicsWorld *m_world;                    // every physical object go to the world
    btCollisionDispatcherMt *dispatcherMt;     // what collision algorithm to use?
    btCollisionConfiguration *collisionConfig; // what collision algorithm to use?
    btBroadphaseInterface *broadphase;         // should Bullet examine every object, or just what close to each other
    btConstraintSolver *solver;                // solve collisions, apply forces, impulses
    btConstraintSolverPoolMt *pool;
    std::unordered_set<btRigidBody *> bodies;
    btConstraintSolver *solverMt;

    // btDynamicsWorld* world;
    btDispatcher *dispatcher;
    // btBroadphaseInterface* broadphase;	//should Bullet examine every object, or just what close to each other
public:
    void setDebug()
    {
        // btIDebugDraw* btd = new btID()
        // world->setDebugDrawer()
        // world->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
    }
    void drawDebug()
    {
        // world->debugDrawWorld();
    }
    btRigidBody *addSphere(float rad, float x, float y, float z, float mass)
    {
        btTransform t; // position and rotation
        t.setIdentity();
        t.setOrigin(btVector3(x, y, z)); // put it to x,y,z coordinates
        // btSphereShape* sphere=new btBoxShape(btVector3(1,1,1));	//it's a sphere, so use sphereshape
        btSphereShape *sphere = new btSphereShape(rad);
        btVector3 inertia(0, 0, 0); // inertia is 0,0,0 for static object, else
        if (mass != 0.0)
            sphere->calculateLocalInertia(mass, inertia); // it can be determined by this function (for all kind of shapes)

        btMotionState *motion = new btDefaultMotionState(t);                          // set the position (and motion)
        btRigidBody::btRigidBodyConstructionInfo info(mass, motion, sphere, inertia); // create the constructioninfo, you can create multiple bodies with the same info
        // info.m_restitution = 0.8;
        btRigidBody *body = new btRigidBody(info); // let's create the body itself
        body->setActivationState(DISABLE_DEACTIVATION);
        m_world->addRigidBody(body); // and let the world know about it
        bodies.emplace(body);      // to be easier to clean, I store them a vector
        return body;
    }
    btRigidBody *addBox(glm::vec3 dims, glm::quat rot, glm::vec3 pos, float mass)
    {
        btTransform t; // position and rotation
        t.setIdentity();
        t.setOrigin(btVector3(pos.x, pos.y, pos.z)); // put it to x,y,z coordinates
        btQuaternion quat(rot.x, rot.y, rot.z, rot.w);
        t.setRotation(quat);
        btBoxShape *sphere = new btBoxShape(btVector3(dims.x, dims.y, dims.z)); // it's a sphere, so use sphereshape
        // btSphereShape* sphere = new btSphereShape(rad);
        btVector3 inertia(0, 0, 0); // inertia is 0,0,0 for static object, else
        if (mass != 0.0)
            sphere->calculateLocalInertia(mass, inertia); // it can be determined by this function (for all kind of shapes)

        btMotionState *motion = new btDefaultMotionState(t);                          // set the position (and motion)
        btRigidBody::btRigidBodyConstructionInfo info(mass, motion, sphere, inertia); // create the constructioninfo, you can create multiple bodies with the same info
        // info.m_restitution = 0.8;
        btRigidBody *body = new btRigidBody(info); // let's create the body itself
        body->setActivationState(DISABLE_DEACTIVATION);
        m_world->addRigidBody(body); // and let the world know about it
        bodies.emplace(body);      // to be easier to clean, I store them a vector
        return body;
    }
    btRigidBody *addTriangleMesh(Mesh *m, glm::vec3 pos, glm::quat rot, float mass)
    {
        
        btTriangleMesh *trimesh = new btTriangleMesh();
        // trimesh->addTriangle(p0, p1, p2);
        for(int i = 0; i < m->indices.size(); i += 3){
            glm::vec3 p0 = m->vertices[m->indices[i + 0]];
            glm::vec3 p1 = m->vertices[m->indices[i + 1]];
            glm::vec3 p2 = m->vertices[m->indices[i + 2]];
            trimesh->addTriangle(btVector3(p0.x,p0.y,p0.z),btVector3(p1.x,p1.y,p1.z),btVector3(p2.x,p2.y,p2.z));
        }

        btTransform t;
        t.setIdentity();
        t.setOrigin(btVector3(pos.x, pos.y, pos.z));
        t.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

        btCollisionShape *trimeshShape = new btBvhTriangleMeshShape(trimesh, false);
        btVector3 inertia(0, 0, 0); // inertia is 0,0,0 for static object, else
        if (mass != 0.0)
            trimeshShape->calculateLocalInertia(mass, inertia); // gives error

        btDefaultMotionState *motionstate = new btDefaultMotionState(t);

        btRigidBody::btRigidBodyConstructionInfo info(mass,motionstate,trimeshShape); // create the constructioninfo, you can create multiple bodies with the same info
        // info.m_restitution = 0.8;
        btRigidBody *body = new btRigidBody(info); // let's create the body itself
        // body->setRestitution(restitution);

        m_world->addRigidBody(body);

        // if (collision)
        bodies.emplace(body);

        // return m_triangleMeshBodies.size() - 1;
        return body;
    }
    // btRigidBody *addMesh(glm::vec3 dims, glm::quat rot, glm::vec3 pos, float mass)
    // {
    //     btTransform t; // position and rotation
    //     t.setIdentity();
    //     t.setOrigin(btVector3(pos.x, pos.y, pos.z)); // put it to x,y,z coordinates
    //     btQuaternion quat(rot.x, rot.y, rot.z, rot.w);
    //     t.setRotation(quat);
    //     btRidig
    //     // btSphereShape* sphere = new btSphereShape(rad);
    //     btVector3 inertia(0, 0, 0); // inertia is 0,0,0 for static object, else
    //     if (mass != 0.0)
    //         sphere->calculateLocalInertia(mass, inertia); // it can be determined by this function (for all kind of shapes)

    //     btMotionState *motion = new btDefaultMotionState(t);                          // set the position (and motion)
    //     btRigidBody::btRigidBodyConstructionInfo info(mass, motion, sphere, inertia); // create the constructioninfo, you can create multiple bodies with the same info
    //     // info.m_restitution = 0.8;
    //     btRigidBody *body = new btRigidBody(info); // let's create the body itself
    //     body->setActivationState(DISABLE_DEACTIVATION);
    //     m_world->addRigidBody(body); // and let the world know about it
    //     bodies.emplace(body);      // to be easier to clean, I store them a vector
    //     return body;
    // }
    void addBody(btRigidBody *rb)
    {
        m_world->addRigidBody(rb);
        bodies.emplace(rb);
    }
    void simulate(float dt)
    {
        m_world->stepSimulation(dt, 1, 1.0 / 30.0);
    }
    _physicsManager()
    {
        // pretty much initialize everything logically
        btDefaultCollisionConstructionInfo cci;
        cci.m_defaultMaxPersistentManifoldPoolSize = 80000;
        cci.m_defaultMaxCollisionAlgorithmPoolSize = 80000;
        taskSchedular = btCreateDefaultTaskScheduler();
        taskSchedular->setNumThreads(concurrency::numThreads);
        btSetTaskScheduler(taskSchedular);
        auto ts = btGetTaskScheduler();
        std::cout << "bt threads: " << ts->getNumThreads() << std::endl;
        collisionConfig = new btDefaultCollisionConfiguration(cci);
        solverMt = new btSequentialImpulseConstraintSolverMt();
        // dispatcherMt=new btCollisionDispatcherMt(collisionConfig);
        broadphase = new btDbvtBroadphase();
        solver = new btSequentialImpulseConstraintSolver();
        pool = new btConstraintSolverPoolMt(concurrency::numThreads);
        // world=new btDiscreteDynamicsWorldMt(dispatcherMt,broadphase,pool,solver,collisionConfig);//btDiscreteDynamicsWorldMt(dispatcher,broadphase,solver,collisionConfig);

        dispatcher = new btCollisionDispatcher(collisionConfig);
        m_world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);
        m_world->setGravity(btVector3(0, -10, 0)); // gravity on Earth
    }
    void removeRigidBody(btRigidBody *rb)
    {
        m_world->removeCollisionObject(rb);
        btMotionState *motionState = rb->getMotionState();
        btCollisionShape *shape = rb->getCollisionShape();
        bodies.erase(rb);
        delete rb;
        delete shape;
        delete motionState;
    }
    ~_physicsManager()
    {
        for (auto i : bodies)
        {
            m_world->removeCollisionObject(i);
            btMotionState *motionState = i->getMotionState();
            btCollisionShape *shape = i->getCollisionShape();
            delete i;
            delete shape;
            delete motionState;
        }
        delete solverMt;
        delete dispatcher;
        delete collisionConfig;
        delete solver;
        delete broadphase;
        delete m_world;
    }
};
#endif