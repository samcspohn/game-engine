// #include "bullet/src/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
// #include "bullet/src/btBulletDynamicsCommon.h"
// #include "bullet/src/BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h"
// #include "bullet/src/BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h"
// #include "bullet/src/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.h"
// #include "BulletDynamics/Dynamics/btSimulationIslandManagerMt.h" // for setSplitIslands()
#include <vector>
#include <unordered_set>
#include "concurrency.h"
#include "iostream"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Mesh.h"
#include <mutex>
#include "PhysX/physx/include/PxConfig.h"
#include "PhysX/physx/include/PxPhysicsAPI.h"

#ifndef PHYSICS
#define PHYSICS

using namespace physx;

class game_object;
class _physicsManager
{
    physx::PxDefaultAllocator gAllocator;
    physx::PxDefaultErrorCallback gErrorCallback;

    physx::PxFoundation *gFoundation = NULL;
    physx::PxPhysics *gPhysics = NULL;
    physx::PxCooking *gCooking = 0;

    physx::PxDefaultCpuDispatcher *gDispatcher = NULL;

    physx::PxMaterial *gMaterial = NULL;

    physx::PxPvd *gPvd = NULL;

    class callback : PxSimulationEventCallback
    {
        void onContact(const PxContactPairHeader &pairHeader, const PxContactPair *pairs, PxU32 nbPairs)
        {
            for (PxU32 i = 0; i < nbPairs; i++)
            {
                const PxContactPair &cp = pairs[i];

                if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
                {
                    // collision col;
                    // col.g_o = reinterpret_cast<game_object*>(pairHeader.actors[1]->userData);
                    // // col.normal = cp.
                    // reinterpret_cast<game_object*>(pairHeader.actors[0]->userData)->collide();
                    // if((pairHeader.actors[0] == mSubmarineActor) || (pairHeader.actors[1] == mSubmarineActor))
                    // {
                    //         PxActor* otherActor = (mSubmarineActor == pairHeader.actors[0]) ?
                    //                 pairHeader.actors[1] : pairHeader.actors[0];
                    //         Seamine* mine =  reinterpret_cast<Seamine*>(otherActor->userData);
                    //         // insert only once
                    //         if(std::find(mMinesToExplode.begin(), mMinesToExplode.end(), mine) == mMinesToExplode.end())
                    //                 mMinesToExplode.push_back(mine);

                    //         break;
                    // }
                }
            }
        }
    };

    void createStack(const PxTransform &t, PxU32 size, PxReal halfExtent)
    {
        PxShape *shape = gPhysics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial);
        for (PxU32 i = 0; i < size; i++)
        {
            for (PxU32 j = 0; j < size - i; j++)
            {
                PxTransform localTm(PxVec3(PxReal(j * 2) - PxReal(size - i), PxReal(i * 2 + 1), 0) * halfExtent);
                PxRigidDynamic *body = gPhysics->createRigidDynamic(t.transform(localTm));
                body->attachShape(*shape);
                PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
                gScene->addActor(*body);
            }
        }
        shape->release();
    }

    void createPhysicsAndScene()
    {
        gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

        gPvd = PxCreatePvd(*gFoundation);
        physx::PxPvdTransport *transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
        gPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

        gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, physx::PxTolerancesScale(), true, gPvd);
        gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
        gCooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, PxCookingParams(gPhysics->getTolerancesScale()));

        PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);

        // int numCores = concurrency::numThreads;
        gDispatcher = PxDefaultCpuDispatcherCreate(concurrency::numThreads);
        sceneDesc.cpuDispatcher = gDispatcher;
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
        // sceneDesc.simulationEventCallback ;


        gScene = gPhysics->createScene(sceneDesc);

        PxRigidStatic *groundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
        gScene->addActor(*groundPlane);

        for (int i = 0; i < 5; i++)
            createStack(PxTransform(PxVec3(0, 0, i * 10.0f)), 10, 2.0f);
    }

    // btITaskScheduler *taskSchedular;
    // btDynamicsWorld *m_world;                  // every physical object go to the world
    // btCollisionDispatcherMt *dispatcherMt;     // what collision algorithm to use?
    // btCollisionConfiguration *collisionConfig; // what collision algorithm to use?
    // btBroadphaseInterface *broadphase;         // should Bullet examine every object, or just what close to each other
    // btConstraintSolver *solver;                // solve collisions, apply forces, impulses
    // btConstraintSolverPoolMt *pool;
    // std::unordered_set<btRigidBody *> bodies;
    // btConstraintSolver *solverMt;

    // // btDynamicsWorld* world;
    // btDispatcher *dispatcher;
    std::mutex _m;
    // btBroadphaseInterface* broadphase;	//should Bullet examine every object, or just what close to each other
public:
    physx::PxScene *gScene = NULL;
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
    PxRigidDynamic *addPxShpere(glm::vec3 pos, game_object *g)
    {
        // PxTransform t = PxTransform(PxVec3(0,0,0));
        PxShape *shape = gPhysics->createShape(PxSphereGeometry(1), *gMaterial);
        PxTransform localTm(PxVec3(pos.x, pos.y, pos.z));
        PxRigidDynamic *body = gPhysics->createRigidDynamic(localTm);
        body->attachShape(*shape);
        body->userData = g;
        PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
        _m.lock();
        gScene->addActor(*body);
        shape->release();
        _m.unlock();
        return body;
    }
    PxRigidStatic *addPxMesh(glm::vec3 pos, Mesh *m, game_object *g)
    {

        PxTriangleMeshDesc meshDesc;
        meshDesc.points.count = m->vertices.size();
        meshDesc.points.stride = sizeof(glm::vec3);
        meshDesc.points.data = m->vertices.data();

        meshDesc.triangles.count = m->indices.size() / 3;
        meshDesc.triangles.stride = 3 * sizeof(GLuint);
        meshDesc.triangles.data = m->indices.data();

        PxDefaultMemoryOutputStream writeBuffer;
        PxTriangleMeshCookingResult::Enum result;
        bool status = gCooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
        if (!status)
            return NULL;

        PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
        auto mesh = gPhysics->createTriangleMesh(readBuffer);

        PxShape *shape = gPhysics->createShape(PxTriangleMeshGeometry(mesh), *gMaterial);
        PxTransform localTm(PxVec3(pos.x, pos.y, pos.z));
        // PxRigidStatic *body = PxCreateStatic(*gPhysics, localTm, *shape);

        auto *body = gPhysics->createRigidStatic(localTm);
        body->userData = g;
        body->attachShape(*shape);
        // PxRigidBodyExt::updateMassAndInertia(*body, 0);
        _m.lock();
        gScene->addActor(*body);
        shape->release();
        _m.unlock();
        return body;
    }
    PxRigidStatic *addPxHeightMap(glm::vec3 pos, int numCols, int numRows, float *samples)
    {
        int len = numCols * numRows;
        auto _samples = (PxHeightFieldSample *)malloc(sizeof(PxHeightFieldSample) * (numCols * numRows));

        for (int i = 0; i < len; i++)
        {
            _samples[i].height = samples[i];
            _samples[i].clearTessFlag();
        }
        PxHeightFieldDesc hfDesc;
        hfDesc.format = PxHeightFieldFormat::eS16_TM;
        hfDesc.nbColumns = numCols;
        hfDesc.nbRows = numRows;
        hfDesc.samples.data = _samples;
        hfDesc.samples.stride = sizeof(PxHeightFieldSample);

        PxHeightField *aHeightField = gCooking->createHeightField(hfDesc,
                                                                  gPhysics->getPhysicsInsertionCallback());

        PxShape *shape = gPhysics->createShape(PxHeightFieldGeometry(aHeightField, PxMeshGeometryFlags(), 1.f, 10.f, 10.f), *gMaterial);
        PxTransform localTm(PxVec3(pos.x, pos.y, pos.z));
        PxRigidStatic *body = PxCreateStatic(*gPhysics, localTm, *shape);

        // PxRigidStatic *body = gPhysics->createRigidStatic(localTm);
        // body->attachShape(*shape);
        // PxRigidBodyExt::updateMassAndInertia(*body, 0);
        _m.lock();
        gScene->addActor(*body);
        shape->release();
        _m.unlock();
        return body;
    }

    // btRigidBody *addSphere(float rad, float x, float y, float z, float mass)
    // {
    //     btTransform t; // position and rotation
    //     t.setIdentity();
    //     t.setOrigin(btVector3(x, y, z)); // put it to x,y,z coordinates
    //     // btSphereShape* sphere=new btBoxShape(btVector3(1,1,1));	//it's a sphere, so use sphereshape
    //     btSphereShape *sphere = new btSphereShape(rad);
    //     btVector3 inertia(0, 0, 0); // inertia is 0,0,0 for static object, else
    //     if (mass != 0.0)
    //         sphere->calculateLocalInertia(mass, inertia); // it can be determined by this function (for all kind of shapes)

    //     btMotionState *motion = new btDefaultMotionState(t);                          // set the position (and motion)
    //     btRigidBody::btRigidBodyConstructionInfo info(mass, motion, sphere, inertia); // create the constructioninfo, you can create multiple bodies with the same info
    //     // info.m_restitution = 0.8;
    //     btRigidBody *body = new btRigidBody(info); // let's create the body itself
    //     body->setActivationState(DISABLE_DEACTIVATION);
    //     {
    //         // _m.lock();
    //         m_world->addRigidBody(body); // and let the world know about it
    //         bodies.emplace(body);        // to be easier to clean, I store them a vector
    //         // _m.unlock();                 // to be easier to clean, I store them a vector
    //     }
    //     return body;
    //     // return 0;
    // }
    // btRigidBody *addBox(glm::vec3 dims, glm::quat rot, glm::vec3 pos, float mass)
    // {
    //     btTransform t; // position and rotation
    //     t.setIdentity();
    //     t.setOrigin(btVector3(pos.x, pos.y, pos.z)); // put it to x,y,z coordinates
    //     btQuaternion quat(rot.x, rot.y, rot.z, rot.w);
    //     t.setRotation(quat);
    //     btBoxShape *sphere = new btBoxShape(btVector3(dims.x, dims.y, dims.z)); // it's a sphere, so use sphereshape
    //     // btSphereShape* sphere = new btSphereShape(rad);
    //     btVector3 inertia(0, 0, 0); // inertia is 0,0,0 for static object, else
    //     if (mass != 0.0)
    //         sphere->calculateLocalInertia(mass, inertia); // it can be determined by this function (for all kind of shapes)

    //     btMotionState *motion = new btDefaultMotionState(t);                          // set the position (and motion)
    //     btRigidBody::btRigidBodyConstructionInfo info(mass, motion, sphere, inertia); // create the constructioninfo, you can create multiple bodies with the same info
    //     // info.m_restitution = 0.8;
    //     btRigidBody *body = new btRigidBody(info); // let's create the body itself
    //     body->setActivationState(DISABLE_DEACTIVATION);
    //     {
    //         // _m.lock();
    //         m_world->addRigidBody(body); // and let the world know about it
    //         bodies.emplace(body);        // to be easier to clean, I store them a vector
    //         // _m.unlock();
    //     }
    //     return body;
    // }
    // btRigidBody *addTriangleMesh(Mesh *m, glm::vec3 pos, glm::quat rot, float mass)
    // {

    //     btTriangleMesh *trimesh = new btTriangleMesh();
    //     // trimesh->addTriangle(p0, p1, p2);
    //     for (int i = 0; i < m->indices.size(); i += 3)
    //     {
    //         glm::vec3 p0 = m->vertices[m->indices[i + 0]];
    //         glm::vec3 p1 = m->vertices[m->indices[i + 1]];
    //         glm::vec3 p2 = m->vertices[m->indices[i + 2]];
    //         trimesh->addTriangle(btVector3(p0.x, p0.y, p0.z), btVector3(p1.x, p1.y, p1.z), btVector3(p2.x, p2.y, p2.z));
    //     }

    //     btTransform t;
    //     t.setIdentity();
    //     t.setOrigin(btVector3(pos.x, pos.y, pos.z));
    //     t.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

    //     btCollisionShape *trimeshShape = new btBvhTriangleMeshShape(trimesh, false);
    //     btVector3 inertia(0, 0, 0); // inertia is 0,0,0 for static object, else
    //     if (mass != 0.0)
    //         trimeshShape->calculateLocalInertia(mass, inertia); // gives error

    //     btDefaultMotionState *motionstate = new btDefaultMotionState(t);

    //     btRigidBody::btRigidBodyConstructionInfo info(mass, motionstate, trimeshShape, inertia); // create the constructioninfo, you can create multiple bodies with the same info
    //     // info.m_restitution = 0.8;
    //     btRigidBody *body = new btRigidBody(info); // let's create the body itself
    //                                                // body->setRestitution(restitution);

    //     {
    //         // _m.lock();
    //         m_world->addRigidBody(body); // and let the world know about it
    //         bodies.emplace(body);        // to be easier to clean, I store them a vector
    //         // _m.unlock();
    //     }

    //     // return m_triangleMeshBodies.size() - 1;
    //     return body;
    // }
    // btRigidBody *addTerrain(float *heightMap, int width, int depth, float minHeight, float maxHeight, glm::vec3 scaling, glm::vec3 position)
    // {
    //     // auto hmap = new btHeightfieldTerrainShape(width,depth,heightMap,minHeight,maxHeight,1,false);
    //     auto hmap = new btHeightfieldTerrainShape(width, depth, heightMap, btScalar(1.0), btScalar(minHeight), btScalar(maxHeight), 1, PHY_FLOAT, true);
    //     btVector3 localScaling(scaling.x, scaling.y, scaling.z);
    //     hmap->setLocalScaling(localScaling);
    //     // btTriangleMesh *trimesh = new btTriangleMesh();
    //     // trimesh->addTriangle(p0, p1, p2);
    //     // for(int i = 0; i < m->indices.size(); i += 3){
    //     //     glm::vec3 p0 = m->vertices[m->indices[i + 0]];
    //     //     glm::vec3 p1 = m->vertices[m->indices[i + 1]];
    //     //     glm::vec3 p2 = m->vertices[m->indices[i + 2]];
    //     //     trimesh->addTriangle(btVector3(p0.x,p0.y,p0.z),btVector3(p1.x,p1.y,p1.z),btVector3(p2.x,p2.y,p2.z));
    //     // }

    //     btTransform t;
    //     t.setIdentity();
    //     t.setOrigin(btVector3(position.x, position.y, position.z));
    //     // t.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

    //     // btCollisionShape *trimeshShape = new btBvhTriangleMeshShape(trimesh, false);
    //     // btVector3 inertia(0, 0, 0); // inertia is 0,0,0 for static object, else
    //     // if (mass != 0.0)
    //     //     trimeshShape->calculateLocalInertia(mass, inertia); // gives error

    //     btDefaultMotionState *motionstate = new btDefaultMotionState(t);

    //     btRigidBody::btRigidBodyConstructionInfo info(0, motionstate, hmap); // create the constructioninfo, you can create multiple bodies with the same info
    //     // info.m_restitution = 0.8;
    //     btRigidBody *body = new btRigidBody(info); // let's create the body itself
    //                                                // body->setRestitution(restitution);

    //     {
    //         // _m.lock();
    //         m_world->addRigidBody(body); // and let the world know about it
    //         bodies.emplace(body);        // to be easier to clean, I store them a vector
    //         // _m.unlock();
    //     }

    //     // return m_triangleMeshBodies.size() - 1;
    //     return body;
    // }
    // // btRigidBody *addMesh(glm::vec3 dims, glm::quat rot, glm::vec3 pos, float mass)
    // // {
    // //     btTransform t; // position and rotation
    // //     t.setIdentity();
    // //     t.setOrigin(btVector3(pos.x, pos.y, pos.z)); // put it to x,y,z coordinates
    // //     btQuaternion quat(rot.x, rot.y, rot.z, rot.w);
    // //     t.setRotation(quat);
    // //     btRidig
    // //     // btSphereShape* sphere = new btSphereShape(rad);
    // //     btVector3 inertia(0, 0, 0); // inertia is 0,0,0 for static object, else
    // //     if (mass != 0.0)
    // //         sphere->calculateLocalInertia(mass, inertia); // it can be determined by this function (for all kind of shapes)

    // //     btMotionState *motion = new btDefaultMotionState(t);                          // set the position (and motion)
    // //     btRigidBody::btRigidBodyConstructionInfo info(mass, motion, sphere, inertia); // create the constructioninfo, you can create multiple bodies with the same info
    // //     // info.m_restitution = 0.8;
    // //     btRigidBody *body = new btRigidBody(info); // let's create the body itself
    // //     body->setActivationState(DISABLE_DEACTIVATION);
    // //     m_world->addRigidBody(body); // and let the world know about it
    // //     bodies.emplace(body);      // to be easier to clean, I store them a vector
    // //     return body;
    // // }
    // void addBody(btRigidBody *rb)
    // {
    //     m_world->addRigidBody(rb);
    //     bodies.emplace(rb);
    // }
    // void stepPhysics()
    // {
    //     gScene->fetchResults(true);
    //     // Start simulation
    //     gScene->simulate(1.0f / 30.0f);

    //     // Fetch simulation results
    // }
    // void simulate(float dt)
    // {
    //     // m_world->stepSimulation(dt, 1, 1.0 / 30.0);
    //     stepPhysics();
    // }
    _physicsManager()
    {
        createPhysicsAndScene();

        // // pretty much initialize everything logically
        // btDefaultCollisionConstructionInfo cci;
        // cci.m_defaultMaxPersistentManifoldPoolSize = 80000;
        // cci.m_defaultMaxCollisionAlgorithmPoolSize = 80000;
        // taskSchedular = btCreateDefaultTaskScheduler();
        // taskSchedular->setNumThreads(concurrency::numThreads);
        // btSetTaskScheduler(taskSchedular);
        // auto ts = btGetTaskScheduler();
        // std::cout << "bt threads: " << ts->getNumThreads() << std::endl;
        // collisionConfig = new btDefaultCollisionConfiguration(cci);
        // solverMt = new btSequentialImpulseConstraintSolverMt();
        // dispatcherMt = new btCollisionDispatcherMt(collisionConfig);
        // broadphase = new btDbvtBroadphase();
        // // broadphase = new btSimpleBroadphase();
        // solver = new btSequentialImpulseConstraintSolver();
        // pool = new btConstraintSolverPoolMt(concurrency::numThreads);
        // m_world = new btDiscreteDynamicsWorldMt(dispatcherMt, broadphase, pool, solver, collisionConfig); // btDiscreteDynamicsWorldMt(dispatcher,broadphase,solver,collisionConfig);

        // // dispatcher = new btCollisionDispatcher(collisionConfig);
        // // m_world = new btDiscreteDynamicsWorld(dispatcherMt, broadphase, solverMt, collisionConfig);
        // m_world->setGravity(btVector3(0, -10, 0)); // gravity on Earth
    }
    void removeRigidBody(PxRigidBody *rb)
    {
        // m_world->removeCollisionObject(rb);
        // btMotionState *motionState = rb->getMotionState();
        // btCollisionShape *shape = rb->getCollisionShape();
        // bodies.erase(rb);
        // delete rb;
        // delete shape;
        // delete motionState;
    }
    ~_physicsManager()
    {
        // for (auto i : bodies)
        // {
        //     m_world->removeCollisionObject(i);
        //     btMotionState *motionState = i->getMotionState();
        //     btCollisionShape *shape = i->getCollisionShape();
        //     delete i;
        //     delete shape;
        //     delete motionState;
        // }
        // delete solverMt;
        // delete dispatcher;
        // delete collisionConfig;
        // delete solver;
        // delete broadphase;
        // delete m_world;
    }
};

extern _physicsManager *pm;
#endif