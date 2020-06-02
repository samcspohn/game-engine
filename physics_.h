#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "bullet/btBulletDynamicsCommon.h"
#include "bullet/BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h"
#include "bullet/BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h"
#include "bullet/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.h"
#include <vector>
#include "concurrency.h"
#include "iostream"

#ifndef PHYSICS
#define PHYSICS
class _physicsManager{

    btITaskScheduler* taskSchedular;
    btDynamicsWorld* world;	//every physical object go to the world
    btCollisionDispatcherMt* dispatcherMt;	//what collision algorithm to use?
    btCollisionConfiguration* collisionConfig;	//what collision algorithm to use?
    btBroadphaseInterface* broadphase;	//should Bullet examine every object, or just what close to each other
    btConstraintSolver* solver;					//solve collisions, apply forces, impulses
    btConstraintSolverPoolMt *pool;
    std::vector<btRigidBody*> bodies;
    btConstraintSolver *solverMt;

    // btDynamicsWorld* world;
    btDispatcher* dispatcher;
    // btBroadphaseInterface* broadphase;	//should Bullet examine every object, or just what close to each other	
public:
    btRigidBody* addSphere(float rad,float x,float y,float z,float mass)
    {
        btTransform t;	//position and rotation
        t.setIdentity();
        t.setOrigin(btVector3(x,y,z));	//put it to x,y,z coordinates
        btBoxShape* sphere=new btBoxShape(btVector3(1,1,1));	//it's a sphere, so use sphereshape
        // btSphereShape* sphere = new btSphereShape(rad);
        btVector3 inertia(0,0,0);	//inertia is 0,0,0 for static object, else
        if(mass!=0.0)
            sphere->calculateLocalInertia(mass,inertia);	//it can be determined by this function (for all kind of shapes)
        
        btMotionState* motion=new btDefaultMotionState(t);	//set the position (and motion)
        btRigidBody::btRigidBodyConstructionInfo info(mass,motion,sphere,inertia);	//create the constructioninfo, you can create multiple bodies with the same info
        // info.m_restitution = 0.8;
        btRigidBody* body=new btRigidBody(info);	//let's create the body itself
        body->setActivationState(DISABLE_DEACTIVATION);
        world->addRigidBody(body);	//and let the world know about it
        bodies.push_back(body);	//to be easier to clean, I store them a vector
        return body;
    }
    void addBody(btRigidBody* rb){
        world->addRigidBody(rb);
        bodies.push_back(rb);
    }
    void simulate(float dt){
        world->stepSimulation(dt, 1, 1.0 / 30.0);
    }
    _physicsManager(){
        //pretty much initialize everything logically
        btDefaultCollisionConstructionInfo cci;
        cci.m_defaultMaxPersistentManifoldPoolSize = 80000;
        cci.m_defaultMaxCollisionAlgorithmPoolSize = 80000;
        taskSchedular = btCreateDefaultTaskScheduler();
        taskSchedular->setNumThreads(concurrency::numThreads);
        btSetTaskScheduler(taskSchedular);
        auto ts = btGetTaskScheduler();
        std::cout << "bt threads: " << ts->getNumThreads() << std::endl;
        collisionConfig=new btDefaultCollisionConfiguration(cci);
        solverMt = new btSequentialImpulseConstraintSolverMt();
        // dispatcherMt=new btCollisionDispatcherMt(collisionConfig);
        broadphase=new btDbvtBroadphase();
        solver=new btSequentialImpulseConstraintSolver();
        pool = new btConstraintSolverPoolMt(concurrency::numThreads);
        // world=new btDiscreteDynamicsWorldMt(dispatcherMt,broadphase,pool,solver,collisionConfig);//btDiscreteDynamicsWorldMt(dispatcher,broadphase,solver,collisionConfig);
        
        dispatcher=new btCollisionDispatcher(collisionConfig);
        world=new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfig);
        world->setGravity(btVector3(0,-10,0));	//gravity on Earth
    }
    ~_physicsManager(){
        for(int i=0;i<bodies.size();i++)
        {
            world->removeCollisionObject(bodies[i]);
            btMotionState* motionState=bodies[i]->getMotionState();
            btCollisionShape* shape=bodies[i]->getCollisionShape();
            delete bodies[i];
            delete shape;
            delete motionState;
        }
        delete solverMt;
        // delete dispatcherMt;
        delete dispatcher;
        delete collisionConfig;
        delete solver;
        delete broadphase;
        delete world;
    }

};
#endif