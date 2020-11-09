
#include "serialize.h"

namespace boost::serialization {
    template <typename Ar>
    void serialize(Ar& ar, glm::vec3& v, unsigned /*unused*/) {
        ar & make_nvp("x", v.x) & make_nvp("y", v.y) & make_nvp("z", v.z);
    }
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
