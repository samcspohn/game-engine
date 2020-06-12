#include "bullet/src/btBulletDynamicsCommon.h"
#include "bullet/src/BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h"
#include "bullet/src/BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h"
#include "bullet/src/BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.h"
#include "bullet/src/BulletCollision/Gimpact/btContactProcessing.h"
#include <vector>
#include "concurrency.h"
#include "iostream"
#include "game_object.h"
#include <glm/glm.hpp>
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
    // std::vector<btRigidBody*> bodies;
    btConstraintSolver *solverMt;

    // btDynamicsWorld* world;
    btDispatcher* dispatcher;
    btOverlappingPairCache* pairCache;

//     static bool myCustomMaterialCombinerCallback(
//     btManifoldPoint& cp,
//     const btCollisionObjectWrapper* colObj0Wrap,
//     int partId0,
//     int index0,
//     const btCollisionObjectWrapper* colObj1Wrap,
//     int partId1,
//     int index1
//     )
// {
//     // one-sided triangles
// 	if (colObj1Wrap->getCollisionShape()->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE)
//     {
//         auto triShape = static_cast<const btTriangleShape*>( colObj1Wrap->getCollisionShape() );
//         const btVector3* v = triShape->m_vertices1;
//         btVector3 faceNormalLs = btCross(v[1] - v[0], v[2] - v[0]);
//         faceNormalLs.normalize();
//         btVector3 faceNormalWs = colObj1Wrap->getWorldTransform().getBasis() * faceNormalLs;
//         float nDotF = btDot( faceNormalWs, cp.m_normalWorldOnB );
//         if ( nDotF <= 0.0f )
//         {
//             // flip the contact normal to be aligned with the face normal
//             cp.m_normalWorldOnB += -2.0f * nDotF * faceNormalWs;
//         }
//     }

//     //this return value is currently ignored, but to be on the safe side: return false if you don't calculate friction
//     return false;
// }
static void myTickCallback(btDynamicsWorld *world, btScalar timeStep) {
    int numManifolds = world->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold *contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
        auto *objA = contactManifold->getBody0();
        auto *objB = contactManifold->getBody1();
        // auto& collisionsA = objectsCollisions[(game_object*)objA->getUserPointer()];
        // auto& collisionsB = objectsCollisions[(game_object*)objB->getUserPointer()];

        // int numContacts = contactManifold->getNumContacts();
        // for (int j = 0; j < numContacts; j++) {
        //     contactManifold->getBody0()->getUserPointer();
        //     btManifoldPoint& pt = contactManifold->getContactPoint(j);
        //     collisionsA.push_back((game_object*)objB->getUserPointer());
        //     collisionsB.push_back((game_object*)objA->getUserPointer());
        // }
        int numContacts = contactManifold->getNumContacts();
        for (int j = 0; j < numContacts; j++) {
            btManifoldPoint& pt = contactManifold->getContactPoint(j);
            if (pt.getDistance()<0.f)
            {
                game_object* oA = (game_object*)objA->getUserPointer();
                game_object* oB = (game_object*)objB->getUserPointer();
                oA->collide(oB,*(glm::vec3*)(void*)contactManifold->getContactPoint(0).m_positionWorldOnB,glm::vec3(0,1,0));
                oB->collide(oA,*(glm::vec3*)(void*)contactManifold->getContactPoint(0).m_positionWorldOnA,glm::vec3(0,1,0));
            }
        }
    }
}
public:
    btRigidBody* addSphere(game_object* go, float rad,float x,float y,float z,float mass)
    {
        btTransform t;	//position and rotation
        t.setIdentity();
        t.setOrigin(btVector3(x,y,z));	//put it to x,y,z coordinates
        btCollisionShape* box=new btBoxShape(btVector3(1,1,1));	//it's a sphere, so use sphereshape
        // btSphereShape* sphere = new btSphereShape(rad);
        btVector3 inertia(0,0,0);	//inertia is 0,0,0 for static object, else
        if(mass!=0.0)
            box->calculateLocalInertia(mass,inertia);	//it can be determined by this function (for all kind of shapes)
        
        btMotionState* motion=new btDefaultMotionState(t);	//set the position (and motion)
        btRigidBody::btRigidBodyConstructionInfo info(mass,motion,box,inertia);	//create the constructioninfo, you can create multiple bodies with the same info
        // info.m_restitution = 0.8;
        btRigidBody* body=new btRigidBody(info);	//let's create the body itself
        body->setCcdMotionThreshold(2.f);
        body->setCcdSweptSphereRadius(0.2f);
        body->setUserPointer(go);
        // body->setActivationState(DISABLE_DEACTIVATION);
        world->addRigidBody(body);	//and let the world know about it
        // bodies.push_back(body);	//to be easier to clean, I store them a vector
        return body;
    }
    void addBody(btRigidBody* rb){
        world->addRigidBody(rb);
    }
    void simulate(float dt){
        world->stepSimulation(dt, 1, 1.0 / 30.0);

{

        // for (int j = world->getNumCollisionObjects() - 1; j >= 0; --j) {
        //     btCollisionObject *obj = world->getCollisionObjectArray()[j];
        //     btRigidBody *body = btRigidBody::upcast(obj);
        //     btTransform trans;
        //     if (body && body->getMotionState()) {
        //         body->getMotionState()->getWorldTransform(trans);
        //     } else {
        //         trans = obj->getWorldTransform();
        //     }
        //     btVector3 origin = trans.getOrigin();
        //     game_object* g = (game_object*)body->getUserPointer();
            
        //     auto& manifoldPoints = objectsCollisions[g];
        //     for(auto& i : manifoldPoints){
        //         if(g != i)
        //             g->collide(i,glm::vec3(0),glm::vec3(0,1,0));
        //     }
        //     // if (manifoldPoints.empty()) {
        //     //     std::printf("0");
        //     // } else {
        //     //     std::printf("1");
        //     // }
        //     // puts("");
        // }
}
    }
    _physicsManager(){
        //pretty much initialize everything logically
        btDefaultCollisionConstructionInfo cci;
        cci.m_defaultMaxPersistentManifoldPoolSize = 160000;
        cci.m_defaultMaxCollisionAlgorithmPoolSize = 160000;
        // taskSchedular = btCreateDefaultTaskScheduler();
        // taskSchedular->setNumThreads(concurrency::numThreads);
        // btSetTaskScheduler(taskSchedular);
        // auto ts = btGetTaskScheduler();
        // std::cout << "bt threads: " << ts->getNumThreads() << std::endl;
        collisionConfig=new btDefaultCollisionConfiguration(cci);
        solver=new btSequentialImpulseConstraintSolver();
        // solverMt = new btSequentialImpulseConstraintSolverMt();
        pairCache = new btHashedOverlappingPairCache();
        broadphase=new btDbvtBroadphase(pairCache);
        // dispatcherMt=new btCollisionDispatcherMt(collisionConfig, 2);
        dispatcher=new btCollisionDispatcher(collisionConfig);
        pool = new btConstraintSolverPoolMt(concurrency::numThreads);
        // world=new btDiscreteDynamicsWorldMt(dispatcher,broadphase,pool,solver,collisionConfig);//btDiscreteDynamicsWorldMt(dispatcher,broadphase,solver,collisionConfig);
        
        world=new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfig);
        world->setGravity(btVector3(0,-10,0));	//gravity on Earth
        world->setInternalTickCallback(myTickCallback);
        // gContactAddedCallback = myCustomMaterialCombinerCallback;
    }
    void removeRigidBody(btRigidBody* rb){
		this->world->removeCollisionObject(rb);
    }
    void destroy(){
    // delete solverMt;
        // delete dispatcherMt;
        delete world;
        delete dispatcher;
        delete collisionConfig;
        delete solver;
        delete broadphase;
        delete pairCache;
    }
};

_physicsManager* pm;


_transform renderSphere(btRigidBody* sphere)
{
	// if(sphere->getCollisionShape()->getShapeType()!=SPHERE_SHAPE_PROXYTYPE)	//only render, if it's a sphere
	// 	return;
	// glColor3f(1,0,0);
	// float r=((btSphereShape*)sphere->getCollisionShape())->getRadius();
	btTransform _t;
	sphere->getMotionState()->getWorldTransform(_t);	//get the transform
	mat4 mat;
	_t.getOpenGLMatrix(glm::value_ptr(mat));	//OpenGL matrix stores the rotation and orientation
	
	_transform t;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(mat, t.scale, t.rotation, t.position, skew, perspective);
	return t;
}

int colCount = 0;

glm::vec3 makeGlm(const btVector3& v){
	return glm::vec3(v.getX(),v.getY(),v.getZ());
}
glm::mat3 makeGlm(const btMatrix3x3& v){
	return glm::mat3(v[0].getX(),v[0].getY(),v[0].getZ(),
	v[1].getX(),v[1].getY(),v[1].getZ(),
	v[2].getX(),v[2].getY(),v[2].getZ());
}
mutex argggh;
class physicsObject : public component{

	btRigidBody* rb;
	public:
	void init(vec3 force){
		argggh.lock();
        rb->getWorldTransform().setOrigin(btVector3(transform->getPosition().x,transform->getPosition().y,transform->getPosition().z));
		rb->setLinearVelocity(btVector3(force.x,force.y,force.z));
		argggh.unlock();
	}
	void onStart(){
		argggh.lock();
		rb = pm->addSphere(transform->gameObject,1,transform->getPosition().x,transform->getPosition().y,transform->getPosition().z,1);
		argggh.unlock();

		// init(transform->getPosition().x,transform->getPosition().y,transform->getPosition().z)
	}
	void update(){
		_transform _t = renderSphere(rb);
		transform->setPosition(*(glm::vec3*)(void*)&rb->getWorldTransform().getOrigin());
		transform->setRotation(glm::toQuat(glm::transpose(*(glm::mat4*)(void*)&rb->getWorldTransform().getBasis())));
		// transform->setRotation(_t.rotation);
		// transform->setScale(_t.scale);
		terrainHit hit = terrains.begin()->second->getHeight(_t.position.x,_t.position.z);
		if(_t.position.y < hit.height){
			btTransform t = rb->getWorldTransform();
			btVector3 v = t.getOrigin();
			v.setY(hit.height + 1);
			t.setOrigin(v);
			rb->setWorldTransform(t);
			rb->setLinearVelocity(btVector3(0,0,0));
			// rb->applyForce(btVector3(0,0.1,0),btVector3(v.getX(),v.getY() - 0.9,v.getZ()));
		}
		// get transform from btTransform
	}
	void onDestroy(){
		argggh.lock();
		pm->removeRigidBody(rb);
		btMotionState* motionState = rb->getMotionState();
		btCollisionShape* shape = rb->getCollisionShape();
		delete rb;
		delete shape;
		delete motionState;
		argggh.unlock();
	}
	UPDATE(physicsObject, update);
	COPY(physicsObject);
};

#endif