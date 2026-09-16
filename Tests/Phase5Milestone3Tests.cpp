#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <expected>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusGenshinClientBooleanFilterDescriptorSourceAdapter.h"

using namespace MiliastraPlusPlus;

namespace
{
    using JsonValue = nlohmann::json;

    [[noreturn]] void Fail(
        const char* Expression,
        const std::source_location& Location
    )
    {
        std::fprintf(
            stderr,
            "Check failed: %s (%s:%u)\n",
            Expression,
            Location.file_name(),
            Location.line()
        );
        std::exit(EXIT_FAILURE);
    }

    void Check(
        bool Condition,
        const char* Expression,
        const std::source_location& Location = std::source_location::current()
    )
    {
        if (!Condition)
        {
            Fail(Expression, Location);
        }
    }

#define MPP_CHECK(Condition) Check((Condition), #Condition, std::source_location::current())

    constexpr char AuthenticNodeMetadataJson[] =
        R"P53N([
{
    "subType": "bool_filter",
    "nodeType": "_3d_vector_angle",
    "displayName": "\u4e09\u7ef4\u5411\u91cf\u5939\u89d2",
    "graphType": 20001,
    "genericId": 200067,
    "concreteId": 135,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u4e09\u7ef4\u5411\u91cf1",
        "connectable": true,
        "connectionType": 11
      },
      {
        "index": 1,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u4e09\u7ef4\u5411\u91cf2",
        "connectable": true,
        "connectionType": 11
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u4e09\u7ef4\u5411\u91cf\u5939\u89d2_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "_3d_vector_cross_product",
    "displayName": "\u4e09\u7ef4\u5411\u91cf\u5916\u79ef",
    "graphType": 20001,
    "genericId": 200064,
    "concreteId": 132,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u4e09\u7ef4\u5411\u91cf1",
        "connectable": true,
        "connectionType": 11
      },
      {
        "index": 1,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u4e09\u7ef4\u5411\u91cf2",
        "connectable": true,
        "connectionType": 11
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u4e09\u7ef4\u5411\u91cf\u5916\u79ef_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "_3d_vector_dot_product",
    "displayName": "\u4e09\u7ef4\u5411\u91cf\u5185\u79ef",
    "graphType": 20001,
    "genericId": 200063,
    "concreteId": 131,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u4e09\u7ef4\u5411\u91cf1",
        "connectable": true,
        "connectionType": 11
      },
      {
        "index": 1,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u4e09\u7ef4\u5411\u91cf2",
        "connectable": true,
        "connectionType": 11
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u4e09\u7ef4\u5411\u91cf\u5185\u79ef_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "_3d_vector_modulo_operation",
    "displayName": "\u4e09\u7ef4\u5411\u91cf\u6a21\u8fd0\u7b97",
    "graphType": 20001,
    "genericId": 200069,
    "concreteId": 137,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u4e09\u7ef4\u5411\u91cf",
        "connectable": true,
        "connectionType": 11
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u4e09\u7ef4\u5411\u91cf\u6a21\u8fd0\u7b97_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "_3d_vector_normalization",
    "displayName": "\u4e09\u7ef4\u5411\u91cf\u5f52\u4e00\u5316",
    "graphType": 20001,
    "genericId": 200100,
    "concreteId": 138,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u4e09\u7ef4\u5411\u91cf",
        "connectable": true,
        "connectionType": 11
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u671d\u5411\u8f6c\u65cb\u8f6c_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "_3d_vector_rotation",
    "displayName": "\u4e09\u7ef4\u5411\u91cf\u65cb\u8f6c",
    "graphType": 20001,
    "genericId": 200068,
    "concreteId": 136,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u88ab\u65cb\u8f6c\u7684\u4e09\u7ef4\u5411\u91cf",
        "connectable": true,
        "connectionType": 11
      },
      {
        "index": 1,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u65cb\u8f6c",
        "connectable": true,
        "connectionType": 11
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u4e09\u7ef4\u5411\u91cf\u52a0\u6cd5_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "_3d_vector_zoom",
    "displayName": "\u4e09\u7ef4\u5411\u91cf\u7f29\u653e",
    "graphType": 20001,
    "genericId": 200066,
    "concreteId": 134,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "float",
        "clientVarType": 7,
        "defaultValue": 0,
        "name": "\u7f29\u653e\u500d\u7387",
        "connectable": true,
        "connectionType": 7
      },
      {
        "index": 1,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u4e09\u7ef4\u5411\u91cf",
        "connectable": true,
        "conne)P53N"
        R"P53N(ctionType": 11
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u4e09\u7ef4\u5411\u91cf\u5f52\u4e00\u5316_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "create3d_vector",
    "displayName": "\u521b\u5efa\u4e09\u7ef4\u5411\u91cf",
    "graphType": 20001,
    "genericId": 200070,
    "concreteId": 1024,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "float",
        "clientVarType": 7,
        "defaultValue": 0,
        "name": "X\u5206\u91cf",
        "connectable": true,
        "connectionType": 7
      },
      {
        "index": 1,
        "kind": "input",
        "type": "float",
        "clientVarType": 7,
        "defaultValue": 0,
        "name": "Y\u5206\u91cf",
        "connectable": true,
        "connectionType": 7
      },
      {
        "index": 2,
        "kind": "input",
        "type": "float",
        "clientVarType": 7,
        "defaultValue": 0,
        "name": "Z\u5206\u91cf",
        "connectable": true,
        "connectionType": 7
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u521b\u5efa\u4e09\u7ef4\u5411\u91cf_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "direction_vector_to_rotation",
    "displayName": "\u65b9\u5411\u5411\u91cf\u8f6c\u65cb\u8f6c",
    "graphType": 20001,
    "genericId": 200073,
    "concreteId": 139,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u5411\u524d\u5411\u91cf",
        "connectable": true,
        "connectionType": 11
      },
      {
        "index": 1,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u5411\u4e0a\u5411\u91cf",
        "connectable": true,
        "connectionType": 11
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u65b9\u5411\u5411\u91cf\u8f6c\u65cb\u8f6c_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "get_current_character",
    "displayName": "\u83b7\u53d6\u5f53\u524d\u89d2\u8272",
    "graphType": 20001,
    "genericId": 200076,
    "concreteId": 1032,
    "inputs": [],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u83b7\u53d6\u5f53\u524d\u89d2\u8272_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "get_list_of_player_entities_on_the_field",
    "displayName": "\u83b7\u53d6\u5728\u573a\u73a9\u5bb6\u5b9e\u4f53\u5217\u8868",
    "graphType": 20001,
    "genericId": 200026,
    "concreteId": 1004,
    "inputs": [],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u83b7\u53d6\u5728\u573a\u73a9\u5bb6\u5b9e\u4f53\u5217\u8868_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "get_player_client_input_device_type",
    "displayName": "\u83b7\u5f97\u73a9\u5bb6\u5ba2\u6237\u7aef\u8f93\u5165\u8bbe\u5907\u7c7b\u578b",
    "graphType": 20001,
    "genericId": 200123,
    "concreteId": 3004,
    "inputs": [],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u83b7\u5f97\u73a9\u5bb6\u5ba2\u6237\u7aef\u8f93\u5165\u8bbe\u5907\u7c7b\u578b_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "get_player_movement_input",
    "displayName": "\u83b7\u53d6\u73a9\u5bb6\u79fb\u52a8\u8f93\u5165",
    "graphType": 20001,
    "genericId": 200255,
    "concreteId": 1070,
    "inputs": [],
    "outputs": [
      {
        "index": 0,
        "kind": "output",
        "type": "float",
        "clientVarType": 7,
        "name": "\u8f93\u5165\u65b9\u5411",
        "connectionType": 7
      },
      {
        "index": 1,
        "kind": "output",
        "type": "float",
        "clientVarType": 7,
        "name": "\u8f93\u5165\u529b\u5ea6",
        "connectionType": 7
      }
    ],
    "sampleFile": "\u89d2\u8272\u64cd\u63a7\u6280\u80fd\u8282\u70b9\u56fe\\\u83b7\u53d6\u73a9\u5bb6\u79fb\u52a8\u8f93\u5165_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "get_self_entity",
    "displayName": "\u83b7\u53d6\u81ea\u8eab\u5b9e\u4f53",
    "graphType": 20001,
    "genericId": 200033,
    "concreteId": 1013,
    "inputs": [],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u83b7\u53d6\u5355\u4f4d\u653b\u51fb\u76ee\u6807_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "get_target_entity",
    "displayName": "\u83b7\u53d6\u76ee\u6807\u5b9e\u4f53",
    "graphType": 20001,
    "genericId": 200034,
    "concreteId": 1014,
    "inputs": [],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u83b7\u53d6\u76ee\u6807\u5b9e\u4f53_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "logical_and_operation",
    "displayName": "\u903b\u8f91\u4e0e\u8fd0\u7b97",
    "graphType": 20001,
    "genericId": 200001,
    "concreteId": 1,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "bool",
        "clientVarType": 5,
        "defaultValue": 0,
        "name": "\u6761\u4ef61",
        "connectable": true,
        "connectionType": 5
      },
      {
        "index": 1,
        "kind": "input",
        "type": "bool",
        "clientVarType": 5,
        "defaultValue": 0,
        "name": "\u6761\u4ef62",
        "connectable": true,
        "connectionType": 5
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u903b\u8f91\u975e\u8fd0\u7b97_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "b)P53N"
        R"P53N(ool_filter",
    "nodeType": "logical_not_operation",
    "displayName": "\u903b\u8f91\u975e\u8fd0\u7b97",
    "graphType": 20001,
    "genericId": 200003,
    "concreteId": 3,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "bool",
        "clientVarType": 5,
        "defaultValue": 0,
        "name": "\u6761\u4ef6",
        "connectable": true,
        "connectionType": 5
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u903b\u8f91\u975e\u8fd0\u7b97_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "logical_or_operation",
    "displayName": "\u903b\u8f91\u6216\u8fd0\u7b97",
    "graphType": 20001,
    "genericId": 200002,
    "concreteId": 2,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "bool",
        "clientVarType": 5,
        "defaultValue": 0,
        "name": "\u6761\u4ef61",
        "connectable": true,
        "connectionType": 5
      },
      {
        "index": 1,
        "kind": "input",
        "type": "bool",
        "clientVarType": 5,
        "defaultValue": 0,
        "name": "\u6761\u4ef62",
        "connectable": true,
        "connectionType": 5
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u903b\u8f91\u6216\u8fd0\u7b97_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "logical_xor_operation",
    "displayName": "\u903b\u8f91\u5f02\u6216\u8fd0\u7b97",
    "graphType": 20001,
    "genericId": 200004,
    "concreteId": 4,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "bool",
        "clientVarType": 5,
        "defaultValue": 0,
        "name": "\u6761\u4ef61",
        "connectable": true,
        "connectionType": 5
      },
      {
        "index": 1,
        "kind": "input",
        "type": "bool",
        "clientVarType": 5,
        "defaultValue": 0,
        "name": "\u6761\u4ef62",
        "connectable": true,
        "connectionType": 5
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u903b\u8f91\u975e\u8fd0\u7b97_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "orientation_to_rotation",
    "displayName": "\u671d\u5411\u8f6c\u65cb\u8f6c",
    "graphType": 20001,
    "genericId": 200074,
    "concreteId": 139,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u671d\u5411",
        "connectable": true,
        "connectionType": 11
      },
      {
        "index": 1,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          1,
          0
        ],
        "name": "010",
        "connectable": true,
        "connectionType": 11
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u62c6\u5206\u4e09\u7ef4\u5411\u91cf_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "query_entity_by_guid",
    "displayName": "\u4ee5GUID\u67e5\u8be2\u5b9e\u4f53",
    "graphType": 20001,
    "genericId": 200023,
    "concreteId": 1001,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "guid",
        "clientVarType": 14,
        "defaultValue": 0,
        "name": "GUID",
        "connectable": true,
        "connectionType": 14
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u83b7\u53d6\u5b9e\u4f53\u4f4d\u7f6e_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "query_if_faction_is_hostile",
    "displayName": "\u67e5\u8be2\u9635\u8425\u662f\u5426\u654c\u5bf9",
    "graphType": 20001,
    "genericId": 200093,
    "concreteId": 1037,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "faction",
        "clientVarType": 16,
        "defaultValue": 0,
        "name": "\u9635\u84251",
        "connectable": true,
        "connectionType": 16
      },
      {
        "index": 1,
        "kind": "input",
        "type": "faction",
        "clientVarType": 16,
        "defaultValue": 0,
        "name": "\u9635\u84252",
        "connectable": true,
        "connectionType": 16
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u67e5\u8be2\u9635\u8425\u662f\u5426\u654c\u5bf9_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "query_if_self_is_in_combat",
    "displayName": "\u67e5\u8be2\u81ea\u8eab\u662f\u5426\u5df2\u5165\u6218",
    "graphType": 20001,
    "genericId": 200037,
    "concreteId": 1017,
    "inputs": [],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u67e5\u8be2\u81ea\u8eab\u662f\u5426\u5df2\u5165\u6218_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter",
    "nodeType": "query_skill_variable_value",
    "displayName": "\u67e5\u8be2\u6280\u80fd\u53d8\u91cf\u5bf9\u5e94\u503c",
    "graphType": 20001,
    "genericId": 200259,
    "concreteId": 1071,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "config_id",
        "clientVarType": 18,
        "defaultValue": 0,
        "name": "\u6280\u80fd\u53d8\u91cf\u914d\u7f6eID",
        "connectable": false
      }
    ],
    "outputs": [
      {
        "index": 0,
        "kind": "output",
        "type": "float",
        "clientVarType": 7,
        "name": "\u53d8\u91cf\u503c",
        "connectionType": 7
      }
    ],
    "sampleFile": "\u89d2\u8272\u64cd\u63a7\u6280\u80fd\u8282\u70b9\u56fe\\\u67e5\u8be2\u6280\u80fd\u53d8\u91cf\u5bf9\u5e94\u503c_\u8fde\u7ebf.gia",
    "flows": []
  },
{
    "subType": "bool_filter")P53N"
        R"P53N(,
    "nodeType": "split3d_vector",
    "displayName": "\u62c6\u5206\u4e09\u7ef4\u5411\u91cf",
    "graphType": 20001,
    "genericId": 200065,
    "concreteId": 133,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "vec3",
        "clientVarType": 11,
        "defaultValue": [
          0,
          0,
          0
        ],
        "name": "\u4e09\u7ef4\u5411\u91cf",
        "connectable": true,
        "connectionType": 11
      }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u62c6\u5206\u4e09\u7ef4\u5411\u91cf_\u8fde\u7ebf.gia",
    "flows": []
  }
])P53N";

    constexpr char AuthenticNodeModesJson[] =
        R"P53M({
  "format": 1,
  "source": {
    "beyondGlobal": {
      "relativePath": "Resource/Json/Beyond/BeyondGlobal/18125831855067188558.mihoyobin",
      "sha256": "91c2d65a7ed5f2ecfc15ea8186ca125e7a30a00914a4b4f142f3b184e7c828b0",
      "decodedSectionsSha256": "8bf4ebc5d8c840c4cc3c0e6d1322392f898e097f6fcc264dd02144e471356cb1"
    },
    "chsTextMap": {
      "relativePath": "Resource/Json/TextMap/CHS/17720714722766726369.mihoyobin",
      "sha256": "355815f5143c6749b9341ff62b0a6d6dbd751664c4e3c6f00bc9bcd6c37b3e48"
    }
  },
  "staticNodeEvidence": [
    {
      "genericId": 200242,
      "relativePath": "Resource/Json/Beyond/Node/16783183026819652111.mihoyobin",
      "sha256": "1578caf1ac26d4c8acfb3431e83de3cda19fd8f21a016344c5ab0b88aaabf417"
    },
    {
      "genericId": 200251,
      "relativePath": "Resource/Json/Beyond/Node/17432833879509313657.mihoyobin",
      "sha256": "4c3d3e0a992b4af1b83017faad0f0f323dac90f737154fafac9d4506deeba143"
    },
    {
      "genericId": 200254,
      "relativePath": "Resource/Json/Beyond/Node/279208459246344190.mihoyobin",
      "sha256": "3b46be73d29be1c0809f3002f8a964867108d3f5e54c08902af10b57d6618388"
    },
    {
      "genericId": 200249,
      "relativePath": "Resource/Json/Beyond/Node/9073923717836444300.mihoyobin",
      "sha256": "b7357cec4fa89fa901f5d8ffae0f0bdb8e7685b8568e2188a2efa52ecdddd3e0"
    }
  ],
  "graphs": {
    "bool_filter": {
      "entryGenericId": 200000,
      "beyond": {
        "status": "available",
        "reason": "",
        "groupKey": 3,
        "groupName": "\u3010\u8d85\u9650\u3011Filter",
        "genericIds": [
          200001,
          200002,
          200003,
          200004,
          200005,
          200006,
          200007,
          200008,
          200009,
          200010,
          200011,
          200012,
          200013,
          200014,
          200015,
          200016,
          200017,
          200018,
          200019,
          200020,
          200021,
          200022,
          200023,
          200024,
          200025,
          200026,
          200027,
          200028,
          200029,
          200030,
          200031,
          200032,
          200033,
          200034,
          200035,
          200037,
          200043,
          200044,
          200045,
          200047,
          200048,
          200049,
          200050,
          200063,
          200064,
          200065,
          200066,
          200067,
          200068,
          200069,
          200070,
          200071,
          200072,
          200073,
          200074,
          200076,
          200093,
          200094,
          200095,
          200096,
          200097,
          200098,
          200099,
          200100,
          200101,
          200102,
          200103,
          200107,
          200109,
          200110,
          200118,
          200119,
          200120,
          200121,
          200123,
          200152,
          200153,
          200154,
          200155,
          200156,
          200157,
          200158,
          200159,
          200160,
          200161,
          200243,
          200244,
          200255,
          200259,
          200267,
          200268,
          200269,
          200270,
          200271,
          200272,
          200273,
          200274,
          200275,
          200276,
          200277,
          200278,
          200279,
          200280,
          200281,
          200282,
          200283,
          200284,
          200285,
          200286,
          200287,
          200290,
          200291,
          200292,
          200293
        ]
      },
      "classic": {
        "status": "available",
        "reason": "",
        "groupKey": 13,
        "groupName": "\u3010\u7ecf\u5178\u3011Filter",
        "genericIds": [
          200001,
          200002,
          200003,
          200004,
          200005,
          200006,
          200007,
          200008,
          200009,
          200010,
          200011,
          200012,
          200013,
          200014,
          200015,
          200016,
          200017,
          200018,
          200019,
          200020,
          200021,
          200022,
          200023,
          200025,
          200026,
          200027,
          200028,
          200029,
          200030,
          200031,
          200032,
          200033,
          200034,
          200035,
          200037,
          200043,
          200044,
          200045,
          200047,
          200048,
          200049,
          200050,
          200063,
          200064,
          200065,
          200066,
          200067,
          200068,
          200069,
          200070,
          200071,
          200072,
          200073,
          200074,
          200076,
          200093,
          200094,
          200095,
          200096,
          200097,
          200098,
          200099,
          200100,
          200101,
          200102,
          200103,
          200107,
          200109,
          200110,
          200123,
          200152,
          200153,
          200154,
          200155,
          200156,
          200157,
          200158,
          200159,
          200160,
          200161,
          200242,
          200243,
          200244,
          200251,
          200254,
          200255,
          200259,
          200271
        ]
      }
    },
    "int_filter": {
      "entryGenericId": 200122,
      "beyond": {
        "status": "available",
        "reason": "",
        "groupKey": 3,
        "groupName": "\u3010\u8d85\u9650\u3011Filter",
        "genericIds": [
          200001,
          200002,
          200003,
          200004,
          200005,
          200006,
          200007,
          200008,
          200009,
          200010,
          200011,
          200012,
          200013,
          200014,
          200015,
          200016,
          200017,
 )P53M"
        R"P53M(         200018,
          200019,
          200020,
          200021,
          200022,
          200023,
          200024,
          200025,
          200026,
          200027,
          200028,
          200029,
          200030,
          200031,
          200032,
          200033,
          200034,
          200035,
          200037,
          200043,
          200044,
          200045,
          200047,
          200048,
          200049,
          200050,
          200063,
          200064,
          200065,
          200066,
          200067,
          200068,
          200069,
          200070,
          200071,
          200072,
          200073,
          200074,
          200076,
          200093,
          200094,
          200095,
          200096,
          200097,
          200098,
          200099,
          200100,
          200101,
          200102,
          200103,
          200107,
          200109,
          200110,
          200118,
          200119,
          200120,
          200121,
          200123,
          200152,
          200153,
          200154,
          200155,
          200156,
          200157,
          200158,
          200159,
          200160,
          200161,
          200243,
          200244,
          200255,
          200259,
          200267,
          200268,
          200269,
          200270,
          200271,
          200272,
          200273,
          200274,
          200275,
          200276,
          200277,
          200278,
          200279,
          200280,
          200281,
          200282,
          200283,
          200284,
          200285,
          200286,
          200287,
          200290,
          200291,
          200292,
          200293
        ]
      },
      "classic": {
        "status": "available",
        "reason": "",
        "groupKey": 13,
        "groupName": "\u3010\u7ecf\u5178\u3011Filter",
        "genericIds": [
          200001,
          200002,
          200003,
          200004,
          200005,
          200006,
          200007,
          200008,
          200009,
          200010,
          200011,
          200012,
          200013,
          200014,
          200015,
          200016,
          200017,
          200018,
          200019,
          200020,
          200021,
          200022,
          200023,
          200025,
          200026,
          200027,
          200028,
          200029,
          200030,
          200031,
          200032,
          200033,
          200034,
          200035,
          200037,
          200043,
          200044,
          200045,
          200047,
          200048,
          200049,
          200050,
          200063,
          200064,
          200065,
          200066,
          200067,
          200068,
          200069,
          200070,
          200071,
          200072,
          200073,
          200074,
          200076,
          200093,
          200094,
          200095,
          200096,
          200097,
          200098,
          200099,
          200100,
          200101,
          200102,
          200103,
          200107,
          200109,
          200110,
          200123,
          200152,
          200153,
          200154,
          200155,
          200156,
          200157,
          200158,
          200159,
          200160,
          200161,
          200242,
          200243,
          200244,
          200251,
          200254,
          200255,
          200259,
          200271
        ]
      }
    },
    "character_skill": {
      "entryGenericId": 200042,
      "beyond": {
        "status": "available",
        "reason": "",
        "groupKey": 4,
        "groupName": "\u3010\u8d85\u9650\u3011\u89d2\u8272\u6280\u80fd\u84dd\u56fe",
        "genericIds": [
          200001,
          200002,
          200003,
          200004,
          200005,
          200006,
          200007,
          200008,
          200009,
          200010,
          200011,
          200012,
          200013,
          200014,
          200015,
          200016,
          200017,
          200018,
          200019,
          200020,
          200021,
          200022,
          200023,
          200024,
          200025,
          200026,
          200027,
          200028,
          200029,
          200030,
          200031,
          200032,
          200033,
          200034,
          200035,
          200037,
          200038,
          200039,
          200040,
          200041,
          200043,
          200044,
          200045,
          200047,
          200048,
          200049,
          200050,
          200051,
          200052,
          200053,
          200055,
          200056,
          200057,
          200058,
          200059,
          200060,
          200061,
          200062,
          200063,
          200064,
          200065,
          200066,
          200067,
          200068,
          200069,
          200070,
          200071,
          200072,
          200073,
          200074,
          200075,
          200076,
          200077,
          200078,
          200079,
          200080,
          200081,
          200082,
          200083,
          200084,
          200085,
          200086,
          200087,
          200088,
          200089,
          200090,
          200091,
          200092,
          200093,
          200094,
          200095,
          200096,
          200097,
          200098,
          200099,
          200100,
          200101,
          200102,
          200103,
          200105,
          200106,
          200107,
          200108,
          200109,
          200110,
          200111,
          200112,
          200113,
          200114,
          200115,
          200116,
          200118,
          200119,
          200120,
          200121,
          200123,
          200124,
          200152,
          200153,
          200154,
          200155,
          200156)P53M"
        R"P53M(,
          200157,
          200158,
          200159,
          200160,
          200161,
          200243,
          200244,
          200255,
          200256,
          200257,
          200258,
          200259,
          200261,
          200262,
          200263,
          200264,
          200265,
          200266,
          200267,
          200268,
          200269,
          200270,
          200271,
          200272,
          200273,
          200274,
          200275,
          200276,
          200277,
          200278,
          200279,
          200280,
          200281,
          200282,
          200283,
          200284,
          200285,
          200286,
          200287,
          200288,
          200290,
          200291,
          200292,
          200293
        ]
      },
      "classic": {
        "status": "unavailable",
        "reason": "BeyondEditor exposes an explicitly empty classic character skill group",
        "groupKey": 14,
        "groupName": "\u3010XXX\u3011\u3010\u7ecf\u5178\u3011\u89d2\u8272\u6280\u80fd",
        "genericIds": []
      }
    },
    "character_control_skill": {
      "entryGenericId": 200042,
      "beyond": {
        "status": "available",
        "reason": "",
        "groupKey": 18,
        "groupName": "\u3010\u8d85\u9650\u3011\u64cd\u63a7\u8fd0\u52a8\u5668\u84dd\u56fe\uff08\u64cd\u63a7\u6280\u80fd\uff09",
        "genericIds": [
          200001,
          200002,
          200003,
          200004,
          200005,
          200006,
          200007,
          200008,
          200009,
          200010,
          200011,
          200012,
          200013,
          200014,
          200015,
          200016,
          200017,
          200018,
          200019,
          200020,
          200021,
          200022,
          200023,
          200024,
          200025,
          200026,
          200027,
          200028,
          200029,
          200030,
          200031,
          200032,
          200033,
          200034,
          200035,
          200037,
          200038,
          200039,
          200040,
          200041,
          200043,
          200044,
          200045,
          200047,
          200048,
          200049,
          200050,
          200051,
          200052,
          200053,
          200055,
          200056,
          200057,
          200058,
          200059,
          200060,
          200061,
          200062,
          200063,
          200064,
          200065,
          200066,
          200067,
          200068,
          200069,
          200070,
          200071,
          200072,
          200073,
          200074,
          200075,
          200076,
          200077,
          200078,
          200079,
          200080,
          200081,
          200082,
          200083,
          200084,
          200085,
          200086,
          200087,
          200088,
          200089,
          200090,
          200091,
          200092,
          200093,
          200094,
          200095,
          200096,
          200097,
          200098,
          200099,
          200100,
          200101,
          200102,
          200103,
          200105,
          200106,
          200107,
          200108,
          200109,
          200110,
          200111,
          200112,
          200113,
          200114,
          200115,
          200116,
          200118,
          200119,
          200120,
          200121,
          200123,
          200124,
          200152,
          200153,
          200154,
          200155,
          200156,
          200157,
          200158,
          200159,
          200160,
          200161,
          200243,
          200244,
          200255,
          200256,
          200257,
          200258,
          200259,
          200261,
          200262,
          200263,
          200264,
          200265,
          200266,
          200267,
          200268,
          200269,
          200270,
          200271,
          200272,
          200273,
          200274,
          200275,
          200276,
          200277,
          200278,
          200279,
          200280,
          200281,
          200282,
          200283,
          200284,
          200285,
          200286,
          200287,
          200288,
          200289,
          200290,
          200291,
          200292,
          200293,
          200294,
          200295,
          200296,
          200297,
          200298,
          200299,
          200300,
          200304,
          200305,
          200306
        ]
      },
      "classic": {
        "status": "unavailable",
        "reason": "BeyondEditor exposes no classic character control skill group",
        "groupKey": null,
        "groupName": null,
        "genericIds": []
      }
    },
    "creation_skill": {
      "entryGenericId": 200042,
      "beyond": {
        "status": "available",
        "reason": "",
        "groupKey": 10,
        "groupName": "\u3010\u8d85\u9650\u3011\u9020\u7269\u6280\u80fd\u84dd\u56fe",
        "genericIds": [
          200001,
          200002,
          200003,
          200004,
          200005,
          200006,
          200007,
          200008,
          200009,
          200010,
          200011,
          200012,
          200013,
          200014,
          200015,
          200016,
          200017,
          200018,
          200019,
          200020,
          200021,
          200022,
          200023,
          200025,
          200026,
          200027,
          200028,
          200029,
          200030,
          200031,
          200032,
          200033,
          200035,
          200038,
          200039,
          200043,
          200044,
          200045,
          200047,
          200048,
          200049,
          200050,
          200051,
          200052,
          200055,
          200056,
          200057,
          200058,
          200059,
       )P53M"
        R"P53M(   200060,
          200063,
          200064,
          200065,
          200066,
          200067,
          200068,
          200069,
          200070,
          200071,
          200072,
          200073,
          200074,
          200077,
          200078,
          200079,
          200080,
          200081,
          200082,
          200083,
          200084,
          200085,
          200086,
          200087,
          200088,
          200089,
          200090,
          200091,
          200092,
          200093,
          200094,
          200095,
          200096,
          200097,
          200098,
          200099,
          200100,
          200101,
          200102,
          200103,
          200107,
          200109,
          200110,
          200111,
          200112,
          200113,
          200114,
          200115,
          200116,
          200124,
          200152,
          200153,
          200154,
          200155,
          200156,
          200157,
          200158,
          200159,
          200160,
          200161,
          200213,
          200214,
          200215,
          200216,
          200217,
          200218,
          200219,
          200220,
          200221,
          200243,
          200244,
          200245,
          200247,
          200248,
          200249,
          200257,
          200258,
          200259,
          200284,
          200285,
          200286,
          200287,
          200290,
          200291,
          200292,
          200293
        ]
      },
      "classic": {
        "status": "available",
        "reason": "",
        "groupKey": 16,
        "groupName": "\u3010\u7ecf\u5178\u3011\u9020\u7269\u6280\u80fd\u84dd\u56fe",
        "genericIds": [
          200001,
          200002,
          200003,
          200004,
          200005,
          200006,
          200007,
          200008,
          200009,
          200010,
          200011,
          200012,
          200013,
          200014,
          200015,
          200016,
          200017,
          200018,
          200019,
          200020,
          200021,
          200022,
          200023,
          200025,
          200026,
          200027,
          200028,
          200029,
          200030,
          200031,
          200032,
          200033,
          200035,
          200038,
          200043,
          200044,
          200045,
          200047,
          200048,
          200049,
          200050,
          200051,
          200052,
          200055,
          200056,
          200057,
          200058,
          200059,
          200060,
          200063,
          200064,
          200065,
          200066,
          200067,
          200068,
          200069,
          200070,
          200071,
          200072,
          200073,
          200074,
          200077,
          200078,
          200079,
          200080,
          200081,
          200082,
          200093,
          200094,
          200095,
          200096,
          200097,
          200098,
          200099,
          200100,
          200101,
          200102,
          200103,
          200107,
          200109,
          200110,
          200111,
          200112,
          200113,
          200114,
          200115,
          200116,
          200124,
          200152,
          200153,
          200154,
          200155,
          200156,
          200157,
          200158,
          200159,
          200160,
          200161,
          200213,
          200214,
          200215,
          200216,
          200217,
          200218,
          200219,
          200220,
          200221,
          200242,
          200243,
          200244,
          200245,
          200247,
          200248,
          200249,
          200251,
          200254,
          200257,
          200258,
          200259
        ]
      }
    },
    "creation_status_decision": {
      "entryGenericId": 200126,
      "beyond": {
        "status": "available",
        "reason": "",
        "groupKey": 9,
        "groupName": "\u3010\u8d85\u9650\u3011AI\u51b3\u7b56\u84dd\u56fe",
        "genericIds": [
          200125,
          200127,
          200128,
          200142,
          200143,
          200144,
          200145,
          200146,
          200147,
          200148,
          200149,
          200150,
          200151,
          200162,
          200163,
          200164,
          200165,
          200166,
          200167,
          200168,
          200169,
          200170,
          200171,
          200172,
          200173,
          200174,
          200175,
          200176,
          200177,
          200178,
          200179,
          200180,
          200181,
          200182,
          200183,
          200184,
          200185,
          200186,
          200187,
          200188,
          200189,
          200190,
          200191,
          200192,
          200193,
          200194,
          200195,
          200196,
          200197,
          200198,
          200199,
          200200,
          200201,
          200202,
          200203,
          200204,
          200205,
          200206,
          200207,
          200208,
          200209,
          200210,
          200211,
          200212,
          200222,
          200223,
          200224,
          200225,
          200226,
          200227,
          200228,
          200229,
          200230,
          200231,
          200232,
          200233,
          200234,
          200235,
          200236,
          200237,
          200238,
          200239,
          200240,
          200241,
          200250,
          200252
        ]
      },
      "classic": {
        "status": "available",
        "reason": "",
        "groupKey": 15,
        "groupName": "\u3010\u7ecf\u5178\u3011AI\u51b3\u7b56\u84dd\u56fe",
        "genericIds": [
          200125,
          200127,
          2001)P53M"
        R"P53M(28,
          200142,
          200143,
          200144,
          200145,
          200146,
          200147,
          200148,
          200149,
          200150,
          200151,
          200162,
          200163,
          200164,
          200165,
          200166,
          200167,
          200168,
          200169,
          200170,
          200171,
          200172,
          200173,
          200174,
          200175,
          200176,
          200177,
          200178,
          200179,
          200180,
          200181,
          200182,
          200183,
          200184,
          200185,
          200186,
          200187,
          200188,
          200189,
          200190,
          200191,
          200192,
          200193,
          200194,
          200195,
          200196,
          200197,
          200198,
          200199,
          200200,
          200201,
          200202,
          200203,
          200204,
          200205,
          200206,
          200207,
          200208,
          200209,
          200210,
          200211,
          200212,
          200222,
          200223,
          200224,
          200225,
          200226,
          200227,
          200228,
          200229,
          200230,
          200231,
          200232,
          200233,
          200234,
          200235,
          200236,
          200237,
          200238,
          200239,
          200240,
          200241,
          200250,
          200252
        ]
      }
    },
    "creation_status": {
      "entryGenericId": 200126,
      "beyond": {
        "status": "available",
        "reason": "",
        "groupKey": 11,
        "groupName": "\u3010\u8d85\u9650\u3011AI\u72b6\u6001\u84dd\u56fe",
        "genericIds": [
          200125,
          200127,
          200129,
          200130,
          200131,
          200133,
          200134,
          200135,
          200136,
          200137,
          200138,
          200139,
          200140,
          200141,
          200142,
          200143,
          200144,
          200145,
          200146,
          200147,
          200148,
          200149,
          200150,
          200151,
          200162,
          200163,
          200164,
          200165,
          200166,
          200167,
          200168,
          200169,
          200170,
          200171,
          200172,
          200173,
          200174,
          200175,
          200176,
          200177,
          200178,
          200179,
          200180,
          200181,
          200182,
          200183,
          200184,
          200185,
          200186,
          200187,
          200188,
          200189,
          200190,
          200191,
          200192,
          200193,
          200194,
          200195,
          200196,
          200197,
          200198,
          200199,
          200200,
          200201,
          200202,
          200203,
          200204,
          200205,
          200206,
          200207,
          200208,
          200209,
          200210,
          200211,
          200212,
          200222,
          200223,
          200224,
          200225,
          200226,
          200227,
          200228,
          200229,
          200230,
          200231,
          200232,
          200233,
          200234,
          200235,
          200236,
          200237,
          200238,
          200239,
          200240,
          200241,
          200246,
          200250,
          200252,
          200253
        ]
      },
      "classic": {
        "status": "available",
        "reason": "",
        "groupKey": 17,
        "groupName": "\u3010\u7ecf\u5178\u3011AI\u72b6\u6001\u84dd\u56fe",
        "genericIds": [
          200125,
          200127,
          200129,
          200130,
          200131,
          200133,
          200134,
          200135,
          200136,
          200137,
          200138,
          200139,
          200140,
          200141,
          200142,
          200143,
          200144,
          200145,
          200146,
          200147,
          200148,
          200149,
          200150,
          200151,
          200162,
          200163,
          200164,
          200165,
          200166,
          200167,
          200168,
          200169,
          200170,
          200171,
          200172,
          200173,
          200174,
          200175,
          200176,
          200177,
          200178,
          200179,
          200180,
          200181,
          200182,
          200183,
          200184,
          200185,
          200186,
          200187,
          200188,
          200189,
          200190,
          200191,
          200192,
          200193,
          200194,
          200195,
          200196,
          200197,
          200198,
          200199,
          200200,
          200201,
          200202,
          200203,
          200204,
          200205,
          200206,
          200207,
          200208,
          200209,
          200210,
          200211,
          200212,
          200222,
          200223,
          200224,
          200225,
          200226,
          200227,
          200228,
          200229,
          200230,
          200231,
          200232,
          200233,
          200234,
          200235,
          200236,
          200237,
          200238,
          200239,
          200240,
          200241,
          200246,
          200250,
          200252,
          200253
        ]
      }
    }
  }
}
)P53M";

    std::string GetAuthenticNodeMetadataJson()
    {
        return AuthenticNodeMetadataJson;
    }

    std::string GetAuthenticNodeModesJson()
    {
        return AuthenticNodeModesJson;
    }

    std::string GetAuthenticDisplayName(const JsonValue& Record)
    {
        return Record.at("displayName").get<std::string>();
    }

    JsonValue GetAuthenticNodeDocument()
    {
        return JsonValue::parse(GetAuthenticNodeMetadataJson());
    }

    JsonValue GetAuthenticModeDocument()
    {
        return JsonValue::parse(GetAuthenticNodeModesJson());
    }

    std::string GetAuthenticModeMetadataJson();

    std::string GetAuthenticModeMetadataJson()
    {
        return AuthenticNodeModesJson;
    }

    JsonValue* FindJsonRecord(JsonValue& NodeDocument, std::uint64_t GenericId)
    {
        for (JsonValue& Record : NodeDocument)
        {
            if (Record.at("genericId").get<std::uint64_t>() == GenericId)
            {
                return &Record;
            }
        }

        return nullptr;
    }

    const NormalizedNodeDescriptorRecord* FindNormalizedRecord(
        const std::vector<NormalizedNodeDescriptorRecord>& Records,
        std::string_view ExternalKey
    )
    {
        for (const NormalizedNodeDescriptorRecord& Record : Records)
        {
            if (Record.GetExternalIdentity().GetKey() == ExternalKey)
            {
                return &Record;
            }
        }

        return nullptr;
    }

    bool HasDiagnosticCode(
        const DiagnosticCollection& Diagnostics,
        DiagnosticCode Code
    )
    {
        return std::any_of(
            Diagnostics.begin(),
            Diagnostics.end(),
            [Code](const Diagnostic& Diagnostic)
            {
                return Diagnostic.Code == Code;
            }
        );
    }

    std::size_t CountDiagnosticCode(
        const DiagnosticCollection& Diagnostics,
        DiagnosticCode Code
    )
    {
        return static_cast<std::size_t>(std::count_if(
            Diagnostics.begin(),
            Diagnostics.end(),
            [Code](const Diagnostic& Diagnostic)
            {
                return Diagnostic.Code == Code;
            }
        ));
    }

    bool SameDiagnostics(
        const DiagnosticCollection& Left,
        const DiagnosticCollection& Right
    )
    {
        if (Left.size() != Right.size())
        {
            return false;
        }

        for (std::size_t Index = 0U; Index < Left.size(); ++Index)
        {
            if (Left[Index].Severity != Right[Index].Severity ||
                Left[Index].Code != Right[Index].Code ||
                Left[Index].Message != Right[Index].Message ||
                Left[Index].PrimarySourceProvenance !=
                    Right[Index].PrimarySourceProvenance ||
                Left[Index].RelatedSourceProvenance !=
                    Right[Index].RelatedSourceProvenance)
            {
                return false;
            }
        }

        return true;
    }

    template<typename Mutation>
    std::string MakeMutatedNodeMetadata(Mutation MutationFunction)
    {
        JsonValue NodeDocument = GetAuthenticNodeDocument();
        MutationFunction(NodeDocument);
        return NodeDocument.dump();
    }

    template<typename Mutation>
    std::string MakeMutatedModeMetadata(Mutation MutationFunction)
    {
        JsonValue ModeDocument = GetAuthenticModeDocument();
        MutationFunction(ModeDocument);
        return ModeDocument.dump();
    }

    template<typename Mutation>
    void CheckRejectedMutation(
        Mutation MutationFunction,
        DiagnosticCode ExpectedCode =
            DiagnosticCode::UnsupportedGenshinClientBooleanFilterDescriptorSourceForm
    )
    {
        const auto Result =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                MakeMutatedNodeMetadata(MutationFunction),
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasDiagnosticCode(Result.error(), ExpectedCode));
    }

    std::vector<NormalizedNodeDescriptorRecord> AdaptAuthenticFixture()
    {
        const auto Result =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                GetAuthenticNodeMetadataJson(),
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    void CheckCanonicalRecordOrdering(
        const std::vector<NormalizedNodeDescriptorRecord>& Records
    )
    {
        for (std::size_t Index = 1U; Index < Records.size(); ++Index)
        {
            MPP_CHECK(
                Records[Index - 1U].GetExternalIdentity() <
                Records[Index].GetExternalIdentity()
            );
        }
    }

    template<typename Type>
    concept HasDescriptorIdentifierGetter = requires(const Type& Value)
    {
        Value.GetDescriptorIdentifier();
    };

    template<typename Type>
    concept HasNodeDescriptorRegistryGetter = requires(const Type& Value)
    {
        Value.GetNodeDescriptorRegistry();
    };

    template<typename Type>
    concept HasFutureAdapterSurface = requires(const Type& Value)
    {
        Value.GetGraphIR();
        Value.GetGraphBuilder();
        Value.GetSnapshotSchema();
        Value.GetSpecializationArguments();
    };

    void TestGenshinClientBooleanFilterDescriptorSourceAdapterSuccess()
    {
        static_assert(!HasDescriptorIdentifierGetter<NormalizedNodeDescriptorRecord>);
        static_assert(!HasNodeDescriptorRegistryGetter<
            GenshinClientBooleanFilterDescriptorSourceAdapter
        >);
        static_assert(!std::is_default_constructible_v<
            GenshinClientBooleanFilterDescriptorSourceAdapter
        >);

        const auto Records = AdaptAuthenticFixture();
        MPP_CHECK(Records.size() == 25U);
        CheckCanonicalRecordOrdering(Records);

        std::size_t PinCount = 0U;
        for (const NormalizedNodeDescriptorRecord& Record : Records)
        {
            MPP_CHECK(Record.IsValid());
            MPP_CHECK(Record.GetAvailability().size() == 1U);
            MPP_CHECK(Record.GetAvailability()[0U] == NodeAvailability::Client);
            MPP_CHECK(!Record.GetExecutionControlSchema().has_value());
            PinCount += Record.GetPins().size();
        }

        MPP_CHECK(PinCount == 34U);
        MPP_CHECK(FindNormalizedRecord(Records, "200026")->GetPins().empty());
        MPP_CHECK(FindNormalizedRecord(Records, "200001") != nullptr);
    }

    void TestGenshinClientBooleanFilterDescriptorSourceFieldMapping()
    {
        const JsonValue SourceDocument = GetAuthenticNodeDocument();
        const JsonValue* SourceRecord = nullptr;
        for (const JsonValue& Record : SourceDocument)
        {
            if (Record.at("genericId").get<std::uint64_t>() == 200001U)
            {
                SourceRecord = &Record;
                break;
            }
        }
        MPP_CHECK(SourceRecord != nullptr);

        const auto Records = AdaptAuthenticFixture();
        const NormalizedNodeDescriptorRecord* Record =
            FindNormalizedRecord(Records, "200001");
        MPP_CHECK(Record != nullptr);
        MPP_CHECK(Record->GetExternalIdentity().GetKey() == "200001");
        MPP_CHECK(Record->GetDisplayName() == GetAuthenticDisplayName(*SourceRecord));
        MPP_CHECK(Record->GetSourceProvenance().has_value());
        MPP_CHECK(Record->GetSourceProvenance()->GetSourceDocumentIdentifier() ==
            SourceRecord->at("sampleFile").get<std::string>());
        MPP_CHECK(Record->GetSourceProvenance()->GetSourceRecordIdentifier() == "200001");

        MPP_CHECK(Record->GetPins().size() == 2U);
        MPP_CHECK(Record->GetPins()[0U].GetName() ==
            SourceRecord->at("inputs")[0U].at("name").get<std::string>());
        MPP_CHECK(Record->GetPins()[1U].GetName() ==
            SourceRecord->at("inputs")[1U].at("name").get<std::string>());
        for (const NormalizedPinRecord& Pin : Record->GetPins())
        {
            MPP_CHECK(Pin.GetDirection() == PinDirection::Input);
            MPP_CHECK(Pin.GetCategory() == PinCategory::Data);
            MPP_CHECK(Pin.GetCardinality() == PinCardinality::Single);
            MPP_CHECK(Pin.AllowsLiteral());
            MPP_CHECK(Pin.GetDefaultValue().has_value());
        }

        const NormalizedNodeDescriptorRecord* OutputRecord =
            FindNormalizedRecord(Records, "200255");
        MPP_CHECK(OutputRecord != nullptr);
        MPP_CHECK(OutputRecord->GetPins().size() == 2U);
        for (const NormalizedPinRecord& Pin : OutputRecord->GetPins())
        {
            MPP_CHECK(Pin.GetDirection() == PinDirection::Output);
            MPP_CHECK(Pin.GetCategory() == PinCategory::Data);
            MPP_CHECK(Pin.GetCardinality() == PinCardinality::Single);
            MPP_CHECK(!Pin.AllowsLiteral());
            MPP_CHECK(!Pin.GetDefaultValue().has_value());
        }

        const NormalizedNodeDescriptorRecord* MixedRecord =
            FindNormalizedRecord(Records, "200259");
        MPP_CHECK(MixedRecord != nullptr);
        MPP_CHECK(MixedRecord->GetPins().size() == 2U);
        MPP_CHECK(MixedRecord->GetPins()[0U].GetDirection() == PinDirection::Input);
        MPP_CHECK(MixedRecord->GetPins()[1U].GetDirection() == PinDirection::Output);
    }

    void TestGenshinClientBooleanFilterDescriptorSourceTypeAndDefaultMapping()
    {
        const auto Records = AdaptAuthenticFixture();

        const NormalizedPinRecord* BooleanPin =
            &FindNormalizedRecord(Records, "200001")->GetPins()[0U];
        MPP_CHECK(BooleanPin->GetType() == TypeDesc::Boolean());
        MPP_CHECK(BooleanPin->GetDefaultValue()->Is<bool>());
        MPP_CHECK(!*BooleanPin->GetDefaultValue()->TryGet<bool>());

        const NormalizedPinRecord* FloatPin =
            &FindNormalizedRecord(Records, "200066")->GetPins()[0U];
        MPP_CHECK(FloatPin != nullptr);
        MPP_CHECK(FloatPin->GetType() == TypeDesc::Float());
        MPP_CHECK(FloatPin->GetDefaultValue()->Is<double>());
        MPP_CHECK(*FloatPin->GetDefaultValue()->TryGet<double>() == 0.0);

        const NormalizedPinRecord* VectorPin =
            &FindNormalizedRecord(Records, "200063")->GetPins()[0U];
        MPP_CHECK(VectorPin->GetType() == TypeDesc::Vector3());
        MPP_CHECK(VectorPin->GetDefaultValue()->Is<Vector3Value>());
        MPP_CHECK(VectorPin->GetDefaultValue()->TryGet<Vector3Value>()->X == 0.0F);
        MPP_CHECK(VectorPin->GetDefaultValue()->TryGet<Vector3Value>()->Y == 0.0F);
        MPP_CHECK(VectorPin->GetDefaultValue()->TryGet<Vector3Value>()->Z == 0.0F);

        const NormalizedPinRecord* GuidPin =
            &FindNormalizedRecord(Records, "200023")->GetPins()[0U];
        MPP_CHECK(GuidPin->GetType() == TypeDesc::GUID());
        MPP_CHECK(GuidPin->GetDefaultValue()->Is<GuidValue>());

        const NormalizedPinRecord* ConfigPin =
            &FindNormalizedRecord(Records, "200259")->GetPins()[0U];
        MPP_CHECK(ConfigPin->GetType() == TypeDesc::ConfigId());
        MPP_CHECK(ConfigPin->AllowsLiteral());
        MPP_CHECK(ConfigPin->GetDefaultValue()->Is<ConfigIdValue>());
        MPP_CHECK(!ConfigPin->GetDefaultValue()->TryGet<ConfigIdValue>()->Value);

        const NormalizedPinRecord* FactionPin =
            &FindNormalizedRecord(Records, "200093")->GetPins()[0U];
        MPP_CHECK(FactionPin->GetType() == TypeDesc::Faction());
        MPP_CHECK(FactionPin->GetDefaultValue()->Is<FactionValue>());

        JsonValue BooleanOneDocument = GetAuthenticNodeDocument();
        FindJsonRecord(BooleanOneDocument, 200001U)->at("inputs")[0U]["defaultValue"] = 1;
        const auto BooleanOneResult =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                BooleanOneDocument.dump(),
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(BooleanOneResult.has_value());
        MPP_CHECK(BooleanOneResult->front().GetPins()[0U].GetDefaultValue()->Is<bool>());
        MPP_CHECK(*BooleanOneResult->front().GetPins()[0U]
            .GetDefaultValue()->TryGet<bool>());
    }

    void TestGenshinClientBooleanFilterDescriptorSourceUnsupportedForms()
    {
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U][""] = 0;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["defaultValue"] = 2;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            JsonValue& DefaultValue =
                FindJsonRecord(Document, 200063U)->at("inputs")[0U]["defaultValue"];
            DefaultValue = JsonValue::array();
            DefaultValue.push_back(0);
            DefaultValue.push_back(0);
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200063U)->at("inputs")[0U]
                .at("defaultValue")[1U] = "invalid";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200259U)->at("inputs")[0U]["defaultValue"] = -1;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200259U)->at("inputs")[0U]["defaultValue"] = 1.5;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200259U)->at("inputs")[0U]["defaultValue"] =
                JsonValue::parse("18446744073709551616");
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200066U)->at("inputs")[0U]["defaultValue"] =
                JsonValue::parse("3.4028236e38");
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["type"] = "int";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["type"] = "str";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["type"] = "entity";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["type"] = "prefab_id";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["type"] = "enum";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["type"] = "structure";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["type"] = "generic";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["type"] = "list";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["type"] = "dictionary";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["type"] = "unknown";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("subType") = "other";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("graphType") = 20002;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("genericId") = -1;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("genericId") = 200001.5;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("genericId") =
                JsonValue::parse("18446744073709551616");
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("genericId") = 299999;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["kind"] = "in_flow";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["kind"] = "client_exec";
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            (*FindJsonRecord(Document, 200001U))["reflectMap"] = JsonValue::object();
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["reflective"] = true;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["indexOfConcrete"] = 0;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["variants"] = JsonValue::array();
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["i2Index"] = 1;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            (*FindJsonRecord(Document, 200001U))["unknownRecordField"] = 1;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["unknownPinField"] = 1;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U].erase("defaultValue");
        }, DiagnosticCode::MissingGenshinClientBooleanFilterDescriptorSourceField);
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200255U)->at("outputs")[0U]["defaultValue"] = 0;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200001U)->at("inputs")[0U]["clientVarType"] = 7;
        });
        CheckRejectedMutation([](JsonValue& Document)
        {
            FindJsonRecord(Document, 200255U)->at("outputs")[0U]["connectionType"] = 5;
        });
    }

    void TestGenshinClientBooleanFilterDescriptorSourceDeterminism()
    {
        const auto First =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                GetAuthenticNodeMetadataJson(),
                GetAuthenticModeMetadataJson()
            );
        const auto Repeated =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                GetAuthenticNodeMetadataJson(),
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(First.has_value());
        MPP_CHECK(Repeated.has_value());
        MPP_CHECK(*First == *Repeated);

        JsonValue PermutedNodes = GetAuthenticNodeDocument();
        std::reverse(PermutedNodes.begin(), PermutedNodes.end());
        const auto Permuted =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                PermutedNodes.dump(),
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(Permuted.has_value());
        MPP_CHECK(*First == *Permuted);

        JsonValue PermutedModes = GetAuthenticModeDocument();
        std::reverse(
            PermutedModes["graphs"]["bool_filter"]["beyond"]["genericIds"].begin(),
            PermutedModes["graphs"]["bool_filter"]["beyond"]["genericIds"].end()
        );
        std::reverse(
            PermutedModes["graphs"]["bool_filter"]["classic"]["genericIds"].begin(),
            PermutedModes["graphs"]["bool_filter"]["classic"]["genericIds"].end()
        );
        const auto PermutedModeResult =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                GetAuthenticNodeMetadataJson(),
                PermutedModes.dump()
            );
        MPP_CHECK(PermutedModeResult.has_value());
        MPP_CHECK(*First == *PermutedModeResult);

        JsonValue FailingNodesA = GetAuthenticNodeDocument();
        FailingNodesA[0U]["displayName"] = "";
        FailingNodesA[1U]["inputs"][0U]["type"] = "int";
        JsonValue FailingNodesB = FailingNodesA;
        std::reverse(FailingNodesB.begin(), FailingNodesB.end());
        const auto DiagnosticsA =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                FailingNodesA.dump(),
                GetAuthenticModeMetadataJson()
            );
        const auto DiagnosticsB =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                FailingNodesB.dump(),
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(!DiagnosticsA.has_value());
        MPP_CHECK(!DiagnosticsB.has_value());
        MPP_CHECK(SameDiagnostics(DiagnosticsA.error(), DiagnosticsB.error()));
    }

    void TestGenshinClientBooleanFilterDescriptorSourceDiagnostics()
    {
        const auto Malformed =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                "[",
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(!Malformed.has_value());
        MPP_CHECK(HasDiagnosticCode(
            Malformed.error(),
            DiagnosticCode::MalformedGenshinClientBooleanFilterDescriptorSource
        ));

        const auto WrongRoot =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                "{}",
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(!WrongRoot.has_value());
        MPP_CHECK(CountDiagnosticCode(
            WrongRoot.error(),
            DiagnosticCode::MalformedGenshinClientBooleanFilterDescriptorSource
        ) == 1U);

        const auto MissingRecordField =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                MakeMutatedNodeMetadata([](JsonValue& Document)
                {
                    FindJsonRecord(Document, 200001U)->erase("displayName");
                }),
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(!MissingRecordField.has_value());
        MPP_CHECK(HasDiagnosticCode(
            MissingRecordField.error(),
            DiagnosticCode::MissingGenshinClientBooleanFilterDescriptorSourceField
        ));
        MPP_CHECK(MissingRecordField.error()[0U].PrimarySourceProvenance.has_value());

        const auto MissingPinField =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                MakeMutatedNodeMetadata([](JsonValue& Document)
                {
                    FindJsonRecord(Document, 200001U)->at("inputs")[0U].erase("name");
                }),
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(!MissingPinField.has_value());
        MPP_CHECK(HasDiagnosticCode(
            MissingPinField.error(),
            DiagnosticCode::MissingGenshinClientBooleanFilterDescriptorSourceField
        ));

        const auto MissingModeField =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                GetAuthenticNodeMetadataJson(),
                MakeMutatedModeMetadata([](JsonValue& Document)
                {
                    Document["graphs"]["bool_filter"]["beyond"].erase("status");
                })
            );
        MPP_CHECK(!MissingModeField.has_value());
        MPP_CHECK(HasDiagnosticCode(
            MissingModeField.error(),
            DiagnosticCode::MissingGenshinClientBooleanFilterDescriptorSourceField
        ));

        const auto Unsupported =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                MakeMutatedNodeMetadata([](JsonValue& Document)
                {
                    FindJsonRecord(Document, 200001U)->at("inputs")[0U]["type"] = "int";
                }),
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(!Unsupported.has_value());
        MPP_CHECK(HasDiagnosticCode(
            Unsupported.error(),
            DiagnosticCode::UnsupportedGenshinClientBooleanFilterDescriptorSourceForm
        ));

        const auto Duplicate =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                MakeMutatedNodeMetadata([](JsonValue& Document)
                {
                    Document.push_back(Document[0U]);
                }),
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(!Duplicate.has_value());
        MPP_CHECK(CountDiagnosticCode(
            Duplicate.error(),
            DiagnosticCode::DuplicateExternalNodeIdentity
        ) == 1U);
        MPP_CHECK(Duplicate.error()[0U].PrimarySourceProvenance.has_value());
        MPP_CHECK(Duplicate.error()[0U].RelatedSourceProvenance.has_value());
    }

    void TestGenshinClientBooleanFilterDescriptorSourceFailureAtomicity()
    {
        std::string NodeMetadata = GetAuthenticNodeMetadataJson();
        const std::string OriginalNodeMetadata = NodeMetadata;
        std::string ModeMetadata = GetAuthenticModeMetadataJson();
        const std::string OriginalModeMetadata = ModeMetadata;
        const auto Invalid =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                MakeMutatedNodeMetadata([](JsonValue& Document)
                {
                    FindJsonRecord(Document, 200001U)->at("inputs")[0U]["type"] = "int";
                }),
                ModeMetadata
            );
        MPP_CHECK(!Invalid.has_value());
        MPP_CHECK(NodeMetadata == OriginalNodeMetadata);
        MPP_CHECK(ModeMetadata == OriginalModeMetadata);

        const auto Duplicate =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                MakeMutatedNodeMetadata([](JsonValue& Document)
                {
                    Document.push_back(Document[0U]);
                }),
                ModeMetadata
            );
        MPP_CHECK(!Duplicate.has_value());

        const auto Malformed =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                "not-json",
                ModeMetadata
            );
        MPP_CHECK(!Malformed.has_value());

        const auto ValidAfterFailures =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                NodeMetadata,
                ModeMetadata
            );
        MPP_CHECK(ValidAfterFailures.has_value());
        MPP_CHECK(ValidAfterFailures->size() == 25U);
    }

    void TestGenshinClientBooleanFilterDescriptorSourceCatalogueIntegration()
    {
        const auto Records = AdaptAuthenticFixture();
        const std::string SourceNamespace =
            "genshin.client-bool-filter-descriptor-source";
        const std::string SourceRevision =
            "genshin-ts@26bdf2a9a3fadba934423940489236f0b53eb3ea;"
            "client_node_metadata.json@93237c724f6453650ae9394077620c6e0fddb3d3;"
            "client_node_modes.json@b7e14a0dd7102ccd682235cf958a2d3d36378033";

        const auto Catalogue = DescriptorCatalogueBuilder::Build(
            SourceNamespace,
            SourceRevision,
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            Records
        );
        MPP_CHECK(Catalogue.has_value());
        MPP_CHECK(Catalogue->IsValid());
        MPP_CHECK(Catalogue->GetEntryCount() == 25U);
        MPP_CHECK(Catalogue->GetIdentity().GetSourceNamespace() == SourceNamespace);
        MPP_CHECK(Catalogue->GetIdentity().GetSourceRevision() == SourceRevision);
        for (std::size_t Index = 0U; Index < Catalogue->GetEntryCount(); ++Index)
        {
            MPP_CHECK(Catalogue->GetEntries()[Index].GetDescriptorIdentifier() ==
                NodeDescriptorId(static_cast<std::uint32_t>(Index + 1U)));
        }

        const auto RepeatedCatalogue = DescriptorCatalogueBuilder::Build(
            SourceNamespace,
            SourceRevision,
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            AdaptAuthenticFixture()
        );
        MPP_CHECK(RepeatedCatalogue.has_value());
        MPP_CHECK(*Catalogue == *RepeatedCatalogue);
        MPP_CHECK(!HasDescriptorIdentifierGetter<NormalizedNodeDescriptorRecord>);
    }

    void TestGenshinClientBooleanFilterDescriptorSourceProvenance()
    {
        const JsonValue SourceDocument = GetAuthenticNodeDocument();
        const JsonValue* SourceRecord = nullptr;
        for (const JsonValue& Record : SourceDocument)
        {
            if (Record.at("genericId").get<std::uint64_t>() == 200259U)
            {
                SourceRecord = &Record;
                break;
            }
        }
        MPP_CHECK(SourceRecord != nullptr);

        const auto Records = AdaptAuthenticFixture();
        const NormalizedNodeDescriptorRecord* Record =
            FindNormalizedRecord(Records, "200259");
        MPP_CHECK(Record != nullptr);
        MPP_CHECK(Record->GetSourceProvenance().has_value());
        MPP_CHECK(Record->GetSourceProvenance()->GetSourceDocumentIdentifier() ==
            SourceRecord->at("sampleFile").get<std::string>());
        MPP_CHECK(Record->GetSourceProvenance()->GetSourceRecordIdentifier() == "200259");
        MPP_CHECK(Record->GetSourceProvenance()->GetSourceDocumentIdentifier().find(':') ==
            std::string::npos);
        MPP_CHECK(Record->GetSourceProvenance()->GetSourceDocumentIdentifier().find('/') ==
            std::string::npos);

        JsonValue ChangedSourceDocument = GetAuthenticNodeDocument();
        FindJsonRecord(ChangedSourceDocument, 200259U)->at("sampleFile") =
            "another-logical-source.gia";
        const auto ChangedRecords =
            GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                ChangedSourceDocument.dump(),
                GetAuthenticModeMetadataJson()
            );
        MPP_CHECK(ChangedRecords.has_value());
        MPP_CHECK(FindNormalizedRecord(ChangedRecords.value(), "200259")
            ->GetExternalIdentity() == Record->GetExternalIdentity());
        MPP_CHECK(FindNormalizedRecord(ChangedRecords.value(), "200259")
            ->GetSourceProvenance() != Record->GetSourceProvenance());

        const auto OriginalContent = DeriveDescriptorCatalogueContentIdentifier(Records);
        const auto ChangedContent = DeriveDescriptorCatalogueContentIdentifier(
            ChangedRecords.value());
        MPP_CHECK(OriginalContent.has_value());
        MPP_CHECK(ChangedContent.has_value());
        MPP_CHECK(*OriginalContent == *ChangedContent);
    }

    void TestGenshinClientBooleanFilterDescriptorSourceScopeBoundaries()
    {
        using ExpectedAdaptation = std::expected<
            std::vector<NormalizedNodeDescriptorRecord>,
            DiagnosticCollection
        >;
        static_assert(std::is_same_v<
            decltype(GenshinClientBooleanFilterDescriptorSourceAdapter::Adapt(
                std::string(), std::string())),
            ExpectedAdaptation
        >);
        static_assert(!HasDescriptorIdentifierGetter<NormalizedNodeDescriptorRecord>);
        static_assert(!HasNodeDescriptorRegistryGetter<
            GenshinClientBooleanFilterDescriptorSourceAdapter
        >);
        static_assert(!HasFutureAdapterSurface<
            GenshinClientBooleanFilterDescriptorSourceAdapter
        >);

        const auto Records = AdaptAuthenticFixture();
        MPP_CHECK(Records.front().GetSourceProvenance().has_value());
        MPP_CHECK(Records.front().GetSourceProvenance()
            ->GetSourceDocumentIdentifier().find(':') == std::string::npos);
    }
}

int main()
{
    TestGenshinClientBooleanFilterDescriptorSourceAdapterSuccess();
    TestGenshinClientBooleanFilterDescriptorSourceFieldMapping();
    TestGenshinClientBooleanFilterDescriptorSourceTypeAndDefaultMapping();
    TestGenshinClientBooleanFilterDescriptorSourceUnsupportedForms();
    TestGenshinClientBooleanFilterDescriptorSourceDeterminism();
    TestGenshinClientBooleanFilterDescriptorSourceDiagnostics();
    TestGenshinClientBooleanFilterDescriptorSourceFailureAtomicity();
    TestGenshinClientBooleanFilterDescriptorSourceCatalogueIntegration();
    TestGenshinClientBooleanFilterDescriptorSourceProvenance();
    TestGenshinClientBooleanFilterDescriptorSourceScopeBoundaries();
    return EXIT_SUCCESS;
}
