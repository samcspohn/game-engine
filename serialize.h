#pragma once
// #include <boost/archive/text_iarchive.hpp>
// #include <boost/archive/text_oarchive.hpp>
// #include <boost/archive/binary_iarchive.hpp>
// #include <boost/archive/binary_oarchive.hpp>

// // using BIA = boost::archive::binary_iarchive;
// // using BOA = boost::archive::binary_oarchive;

// #include <boost/serialization/base_object.hpp>
// #include <boost/serialization/utility.hpp>
// #include <boost/serialization/list.hpp>
// #include <boost/serialization/assume_abstract.hpp>
// #include <boost/serialization/serialization.hpp>
// #include <boost/serialization/nvp.hpp>
// #include <boost/serialization/export.hpp>
// #include <boost/serialization/deque.hpp>
// #include <boost/serialization/set.hpp>
// #include <boost/serialization/map.hpp>
// #include <boost/serialization/unordered_map.hpp>
// #include <boost/serialization/vector.hpp>
// #include <boost/serialization/priority_queue.hpp>
// #include <boost/serialization/queue.hpp>
// #include <boost/serialization/unique_ptr.hpp>
// #include <boost/serialization/shared_ptr.hpp>
// #include <boost/serialization/array.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <yaml-cpp/yaml.h>
#include <array>

// #define IARCHIVE boost::archive::text_iarchive
// #define OARCHIVE boost::archive::text_oarchive

// // #define IARCHIVE boost::archive::binary_iarchive
// // #define OARCHIVE boost::archive::binary_oarchive

// #define SER_BASE(base) ar &boost::serialization::base_object<base>(*this);

// #define SER_BASE_ASSET SER_BASE(assets::asset)

// #define SER_HELPER()                         \
//   friend class boost::serialization::access; \
//                                              \
//   template <class Archive>                   \
//   inline void serialize(Archive &ar, const unsigned int /* file_version */)

// #define SER_OUT()                            \
//   friend class boost::serialization::access; \
//                                              \
//   inline void serialize(OARCHIVE &ar, const unsigned int /* file_version */)

// #define SER_IN()                             \
//   friend class boost::serialization::access; \
//                                              \
//   inline void serialize(IARCHIVE &ar, const unsigned int /* file_version */)
// // #define SER() SER_HELPER()

// namespace boost::serialization
// {
//   template <typename Archive>
//   inline void serialize(Archive &ar, glm::vec2 &v, unsigned /*unused*/)
//   {
//     ar &v.x &v.y;
//   }

//   template <typename Archive>
//   inline void serialize(Archive &ar, glm::vec3 &v, unsigned /*unused*/)
//   {
//     ar &v.x &v.y &v.z;
//   }
//   template <typename Archive>
//   inline void serialize(Archive &ar, glm::vec4 &v, unsigned /*unused*/)
//   {
//     ar &v.x &v.y &v.z &v.w;
//   }
//   template <typename Archive>
//   inline void serialize(Archive &ar, glm::quat &v, unsigned /*unused*/)
//   {
//     ar &v.x &v.y &v.z &v.w;
//   }

// } // namespace boost::serialization

namespace YAML
{

  template <>
  struct convert<glm::vec2>
  {
    static Node encode(const glm::vec2 &rhs)
    {
      Node node;
      node.push_back(rhs.x);
      node.push_back(rhs.y);
      return node;
    }

    static bool decode(const Node &node, glm::vec2 &rhs)
    {
      if (!node.IsSequence() || node.size() != 2)
      {
        return false;
      }

      rhs.x = node[0].as<float>();
      rhs.y = node[1].as<float>();
      return true;
    }
  };

  template <>
  struct convert<glm::vec3>
  {
    static Node encode(const glm::vec3 &rhs)
    {
      Node node;
      node.push_back(rhs.x);
      node.push_back(rhs.y);
      node.push_back(rhs.z);
      return node;
    }

    static bool decode(const Node &node, glm::vec3 &rhs)
    {
      if (!node.IsSequence() || node.size() != 3)
      {
        return false;
      }

      rhs.x = node[0].as<float>();
      rhs.y = node[1].as<float>();
      rhs.z = node[2].as<float>();
      return true;
    }
  };

  template <>
  struct convert<glm::vec4>
  {
    static Node encode(const glm::vec4 &rhs)
    {
      Node node;
      node.push_back(rhs.x);
      node.push_back(rhs.y);
      node.push_back(rhs.z);
      node.push_back(rhs.w);
      return node;
    }

    static bool decode(const Node &node, glm::vec4 &rhs)
    {
      if (!node.IsSequence() || node.size() != 4)
      {
        return false;
      }

      rhs.x = node[0].as<float>();
      rhs.y = node[1].as<float>();
      rhs.z = node[2].as<float>();
      rhs.w = node[3].as<float>();
      return true;
    }
  };

  template <>
  struct convert<glm::quat>
  {
    static Node encode(const glm::quat &rhs)
    {
      Node node;
      node.push_back(rhs.x);
      node.push_back(rhs.y);
      node.push_back(rhs.z);
      node.push_back(rhs.w);
      return node;
    }

    static bool decode(const Node &node, glm::quat &rhs)
    {
      if (!node.IsSequence() || node.size() != 4)
      {
        return false;
      }

      rhs.x = node[0].as<float>();
      rhs.y = node[1].as<float>();
      rhs.z = node[2].as<float>();
      rhs.w = node[3].as<float>();
      return true;
    }
  };

  // template <>
  // struct convert<std::array<float,3>>
  // {
  //   static Node encode(const std::array<float,3>& v)
  //   {
  //     Node node;
  //     for (int i = 0; i < 3; i++)
  //     {
  //       node.push_back(v[i]);
  //     }
  //     return node;
  //   }

  //   static bool decode(const Node &node, std::array<float,3>& v)
  //   {
  //     for (int i = 0; i < 3; i++)
  //     {
  //       v[i] = node[i].as<float>();
  //     }
  //     return true;
  //   }
  // };

  // template <typename t, size_t u>
  // struct convert<std::array<t,u>>
  // {
  //   static Node encode(const std::array<t,u>& v);

  //   static bool decode(const Node &node, std::array<t,u>& v);
  // };

  // template <typename t, size_t u>
  // struct convert<t[u]>
  // {
  //   static Node encode(const t (&v)[u])
  //   {
  //     Node node;
  //     for (int i = 0; i < u; i++)
  //     {
  //       node.push_back(v[i]);
  //     }
  //     return node;
  //   }

  //   static bool decode(const Node &node, t (&v)[u])
  //   {
  //     for (int i = 0; i < u; i++)
  //     {
  //       v[i] = node[i].as<t>();
  //     }
  //     return true;
  //   }
  // };

}

template <typename t> // built in
YAML::Node YAML_encode(const t &v)
{
  YAML::Node node;
  node = v;
  return node;
}
template <typename t>
bool YAML_decode(const YAML::Node &node, t &v)
{
  v = node.as<t>();
  return true;
}

// array
template <typename t, size_t u>
YAML::Node YAML_encode(const std::array<t, u> &v)
{
  YAML::Node node;
  for (auto &i : v)
  {
    // node.push_back(i);
    node.push_back(YAML_encode(i));
  }
  return node;
}
template <typename t, size_t u>
bool YAML_decode(const YAML::Node &node, std::array<t, u> &v)
{
  for (int i = 0; i < u; i++)
  {
    // v[i] = node[i].as<t>();
    YAML_decode(node[i], v[i]);
  }
  return true;
}

// []
template <typename t, size_t u>
YAML::Node YAML_encode(const t (&v)[u])
{
  YAML::Node node;
  for (int i = 0; i < u; i++)
  {
    // YAML::Node e = YAML_encode(v[i]);
    node.push_back(YAML_encode(v[i]));
  }
  return node;
}
template <typename t, size_t u>
bool YAML_decode(const YAML::Node &node, t (&v)[u])
{
  for (int i = 0; i < u; i++)
  {
    YAML_decode(node[i], v[i]);
    // v[i] = node[i].as<t>();
  }
  return true;
}