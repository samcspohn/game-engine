

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>
#include <glm/glm.hpp>


#define SERIALIZE_CLASS(comp) \
void forceSerialize(){}\
friend boost::archive::text_oarchive & operator<<(boost::archive::text_oarchive  &os, const comp &c);\
    friend class boost::serialization::access;\
\
	template<class Archive>\
    void serialize(Archive & ar, const unsigned int /* file_version */){\
        ar & boost::serialization::base_object<component>(*this)

#define SCE \
;\
}


#define SERIALIZE_STREAM(obj) \
inline boost::archive::text_oarchive  & operator<<(boost::archive::text_oarchive  &os, const obj &o)\
{ return os << ' '\

#define SSE \
;\
}


namespace boost::serialization {
    template <typename Ar>
    void serialize(Ar& ar, glm::vec3& v, unsigned /*unused*/);
}
// namespace boost::serialization {
//     template <typename Ar>
//     void serialize(Ar& ar, glm::vec3& v, unsigned /*unused*/) {
//         ar & make_nvp("x", v.x) & make_nvp("y", v.y) & make_nvp("z", v.z);
//     }
// }



// friend std::ostream & operator<<(std::ostream &os, const component &c);
//     friend class boost::serialization::access;

// 	template<class Archive>
//     void serialize(Archive & ar, const unsigned int /* file_version */){
//         ar & transform;
//     }
