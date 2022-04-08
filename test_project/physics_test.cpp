#include "game_engine.h"
#include <iostream>

using namespace std;

struct physics_step : component
{

    void update()
    {
    }
    SER_FUNC()
    {
    }
};
REGISTER_COMPONENT(physics_step);

tbb::concurrent_vector<collision> collisions;
struct collision_register : component
{

    void onCollision(collision &col)
    {
        collisions.emplace_back(col);
    }
    SER_FUNC()
    {
    }
};
REGISTER_COMPONENT(collision_register)

struct solver : component
{

    void update()
    {
        for (auto &i : collisions)
        {
        }
    }
    SER_FUNC()
    {
    }
};
REGISTER_COMPONENT(solver);

struct sphere_test : component
{

    glm::vec3 vel;
    glm::vec3 ang_vel;
    float inverse_mass;
    collider *c;
    void onStart()
    {
        vel = glm::vec3(0.f);
        ang_vel = randomSphere() * 2.f;
        c = transform->gameObject()->getComponent<collider>();
        // inverse_mass = 1;
    }
    void update()
    {
        if(inverse_mass != 0)
            vel += glm::vec3(0.f, -10.f, 0.f) * Time.deltaTime;
        transform->move(vel * Time.deltaTime);

        glm::quat rot = transform->getRotation();
        glm::vec3 r = ang_vel * Time.deltaTime * 0.5f;
        rot = rot + glm::quat(r.x, r.y, r.z, 0.f) * rot;
        rot = glm::normalize(rot);
        transform->setRotation(rot);
        // transform->rotate(ang_vel_axis,ang_vel);
    }
    glm::mat3 getInertiaTensor()
    {
        return {{0.4, 0, 0}, {0, 0.4, 0}, {0, 0, 0.4}};
    }
    void onCollision(collision &col)
    {

        glm::vec3 normal = glm::normalize(transform.getPosition() - col.point);
        glm::vec3 penetration = col.point - transform->getPosition();
        float l = glm::length(penetration);
        penetration = penetration * (1.f - l);

        sphere_test *sp = col.g_o->getComponent<sphere_test>();

        float inverse_massB{0};
        if (sp)
            inverse_massB = sp->inverse_mass;
        float total_mass = inverse_mass + inverse_massB;

        // pos
        glm::vec3 relativeA = col.point - transform->getPosition();
        glm::vec3 relativeB = col.point - col.other_collider->transform->getPosition();

        transform.move(normal * -penetration * (inverse_mass / total_mass));

        // ang vel
        glm::vec3 angular_velA = glm::cross(ang_vel, relativeA);
        glm::vec3 angular_velB{0};
        if (sp)
            angular_velB = glm::cross(sp->ang_vel, relativeB);
            
        // vel
        glm::vec3 full_velA = vel + angular_velA;
        glm::vec3 full_velB{0};
        if (sp)
        {
            full_velB = sp->vel + angular_velB;
        }

        glm::vec3 contact_vel = full_velB - full_velA;
        float impulse_force = glm::dot(contact_vel, normal);

        using namespace glm;
        vec3 inertiaA = cross(cross(relativeA, normal), relativeA);
        vec3 inertiaB = cross(cross(relativeB, normal), relativeB);
        float ang_effect = dot(inertiaA + inertiaB, normal);
        float cRestitution = 0.66f;
        float j = (-(1.0f + cRestitution) * impulse_force) / (total_mass + ang_effect);
        vec3 full_imp = normal * j;

        vec3 force = cross(relativeA, -full_imp);

        vel += -full_imp * inverse_mass;
        ang_vel += force;

        // 4 PhysicsObject * physA = a . GetPhysicsObject ();
        // 5 PhysicsObject * physB = b . GetPhysicsObject ();
        // 6
        // 7 Transform & transformA = a . GetTransform ();
        // 8 Transform & transformB = b . GetTransform ();
        // 9
        // 10 float totalMass = physA - > GetInverseMass ()+ physB - > GetInverseMass ();
        // 11
        // 12 // Separate them out using projection
        // 13 transformA . SetWorldPosition ( transformA . GetWorldPosition () -
        // 14 ( p . normal * p . penetration *( physA - > GetInverseMass () / totalMass )));
        // 15
        // 16 transformB . SetWorldPosition ( transformB . GetWorldPosition () +
        // 17 ( p . normal * p . penetration *( physB - > GetInverseMass () / totalMass )));

        // 18 Vector3 relativeA = p.position - transformA . GetWorldPosition ();
        // 19 Vector3 relativeB = p.position - transformB . GetWorldPosition ();
        // 20
        // 21 Vector3 angVelocityA =
        // 22 Vector3 :: Cross ( physA - > GetAngularVelocity () , relativeA );
        // 23 Vector3 angVelocityB =
        // 24 Vector3 :: Cross ( physB - > GetAngularVelocity () , relativeB );
        // 25
        // 26 Vector3 fullVelocityA = physA - > GetLinearVelocity () + angVelocityA ;
        // 27 Vector3 fullVelocityB = physB - > GetLinearVelocity () + angVelocityB ;
        //  Vector3 contactVelocity = fullVelocityB - fullVelocityA ;
        // float impulseForce = Vector3 :: Dot ( contactVelocity , p . normal );
        // 31
        // 32 // now to work out the effect of inertia ....
        // 33 Vector3 inertiaA = Vector3 :: Cross ( physA - > GetInertiaTensor () * Vector3 :: Cross ( relativeA , p . normal ) , relativeA );
        // 35 Vector3 inertiaB = Vector3 :: Cross ( physB - > GetInertiaTensor () * Vector3 :: Cross ( relativeB , p . normal ) , relativeB );
        // 37 float angularEffect = Vector3 :: Dot ( inertiaA + inertiaB , p . normal );
        // 38
        // 39 float cRestitution = 0.66 f ; // disperse some kinectic energy
        // 40
        // 41 float j = ( -(1.0 f + cRestitution ) * impulseForce ) /
        // 42 ( totalMass + angularEffect );
        // 43
        // 44 Vector3 fullImpulse = p . normal * j ;
        // physA - > ApplyAngularImpulse ( Vector3 :: Cross ( relativeA , - fullImpulse ));
        // 49 physB - > ApplyAngularImpulse ( Vector3 :: Cross ( relativeB , fullImpulse ));
    }
    SER_FUNC()
    {
        SER(inverse_mass)
    }
};
REGISTER_COMPONENT(sphere_test)