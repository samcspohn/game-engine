//#include <boost/spirit/home/x3.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>
#include <glm/glm.hpp>

namespace boost::serialization {
    template <typename Ar>
    void serialize(Ar& ar, glm::vec3& v, unsigned /*unused*/) {
        ar & make_nvp("x", v.x) & make_nvp("y", v.y) & make_nvp("z", v.z);
    }
}

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <iostream>

struct s{
    glm::vec3 v;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & v;
    }
};
boost::archive::text_oarchive & operator<<(boost::archive::text_oarchive &os, const s &bs)
{
    return os << bs.v << ' ' ;
}
int __main()  {
    auto constexpr inf = std::numeric_limits<double>::infinity();
    auto constexpr nan = std::numeric_limits<double>::quiet_NaN();

    glm::vec3 a{1,2e8,3}, b{-inf, +inf, nan};
    s S;
    S.v = a;

    // {
    //     boost::archive::xml_oarchive oa(std::cout);
    //     oa << BOOST_NVP(a) << BOOST_NVP(b);
    // }
    std::cout << "\n-----\n";

    {
        boost::archive::text_oarchive oa(std::cout);
        oa << a << b << S;
    }
    return 0;
}