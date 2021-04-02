#pragma once
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

// using BIA = boost::archive::binary_iarchive;
// using BOA = boost::archive::binary_oarchive;

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/priority_queue.hpp>
#include <boost/serialization/queue.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/array.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>


#define IARCHIVE boost::archive::text_iarchive
#define OARCHIVE boost::archive::text_oarchive


// #define IARCHIVE boost::archive::binary_iarchive
// #define OARCHIVE boost::archive::binary_oarchive

#define REGISTER_ASSET(asset) \
    BOOST_CLASS_EXPORT(asset) 

#define REGISTER_BASE(base)
BOOST_SERIALIZATION_ASSUME_ABSTRACT(base)

#define SER_BASE(base) ar &boost::serialization::base_object<base>(*this);\

#define SER_BASE_ASSET SER_BASE(assets::asset)

#define SER_HELPER()                                                            \
    friend class boost::serialization::access;                                \
                                                                              \
    template <class Archive>                                                  \
    inline void serialize(Archive &ar, const unsigned int /* file_version */) 


#define SER_OUT()                                                            \
    friend class boost::serialization::access;                                \
                                                                              \
    inline void serialize(OARCHIVE &ar, const unsigned int /* file_version */) 

#define SER_IN()                                                            \
    friend class boost::serialization::access;                                \
                                                                              \
    inline void serialize(IARCHIVE &ar, const unsigned int /* file_version */) 
// #define SER() SER_HELPER()


namespace boost::serialization
{
    template <typename Archive>
    inline void serialize(Archive &ar, glm::vec2 &v, unsigned /*unused*/)
    {
        ar &v.x &v.y;
    }

    template <typename Archive>
    inline void serialize(Archive &ar, glm::vec3 &v, unsigned /*unused*/)
    {
        ar &v.x &v.y &v.z;
    }
    template <typename Archive>
    inline void serialize(Archive &ar, glm::vec4 &v, unsigned /*unused*/)
    {
        ar &v.x &v.y &v.z &v.w;
    }
    template <typename Archive>
    inline void serialize(Archive &ar, glm::quat &v, unsigned /*unused*/)
    {
        ar &v.x &v.y &v.z &v.w;
    }

} // namespace boost::serialization