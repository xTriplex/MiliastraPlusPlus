#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusDescriptorSpecialization.h"
#include "MiliastraPlusPlusGenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapter.h"
#include "nlohmann/json.hpp"

using namespace MiliastraPlusPlus;

#define MPP_CHECK(Expression)                                                   \
    do                                                                          \
    {                                                                           \
        if (!(Expression))                                                      \
        {                                                                       \
            std::fprintf(stderr, "Check failed at line %d.\n", __LINE__);      \
            std::abort();                                                       \
        }                                                                       \
    } while (false)

namespace
{
    using JsonValue = nlohmann::json;

    constexpr char AuthenticNodeMetadataJson[] =
        R"P54N([
  {
    "subType": "bool_filter",
    "nodeType": "get_random_number",
    "displayName": "获取随机数",
    "graphType": 20001,
    "genericId": 200032,
    "concreteId": null,
    "inputs": [
      {
        "index": 0,
        "kind": "input",
        "type": "generic",
        "reflective": true,
        "name": "下限",
        "connectable": true
      },
      {
        "index": 1,
        "kind": "input",
        "type": "generic",
        "reflective": true,
        "name": "上限",
        "connectable": true
      }
    ],
    "outputs": [
      {
        "index": 0,
        "kind": "output",
        "type": "generic",
        "reflective": true,
        "name": "随机数"
      }
    ],
    "sampleFile": "布尔过滤器节点\\除法运算_连线.gia",
    "reflectMap": [
      {
        "concreteId": 1011,
        "variantKey": "3,3",
        "pins": [
          {
            "index": 0,
            "kind": "input",
            "type": "int",
            "indexOfConcrete": 0,
            "clientVarType": 3,
            "connectable": true,
            "connectionType": 3
          },
          {
            "index": 1,
            "kind": "input",
            "type": "int",
            "indexOfConcrete": 0,
            "clientVarType": 3,
            "connectable": true,
            "connectionType": 3
          },
          {
            "index": 0,
            "kind": "output",
            "type": "int",
            "indexOfConcrete": 0,
            "clientVarType": 3,
            "connectionType": 3
          }
        ]
      },
      {
        "concreteId": 1012,
        "variantKey": "7,7",
        "pins": [
          {
            "index": 0,
            "kind": "input",
            "type": "float",
            "indexOfConcrete": 1,
            "clientVarType": 7,
            "connectable": true,
            "connectionType": 7
          },
          {
            "index": 1,
            "kind": "input",
            "type": "float",
            "indexOfConcrete": 1,
            "clientVarType": 7,
            "connectable": true,
            "connectionType": 7
          },
          {
            "index": 0,
            "kind": "output",
            "type": "float",
            "indexOfConcrete": 1,
            "clientVarType": 7,
            "connectionType": 7
          }
        ]
      }
    ],
    "specialKind": "reflect",
    "flows": []
  }
])P54N";

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

    JsonValue GetAuthenticNodeDocument()
    {
        return JsonValue::parse(GetAuthenticNodeMetadataJson());
    }

    JsonValue GetAuthenticModeDocument()
    {
        return JsonValue::parse(GetAuthenticNodeModesJson());
    }

    template<typename Mutation>
    std::string MakeMutatedNodeMetadata(Mutation MutationFunction)
    {
        JsonValue Document = GetAuthenticNodeDocument();
        MutationFunction(Document);
        return Document.dump();
    }

    template<typename Mutation>
    std::string MakeMutatedModeMetadata(Mutation MutationFunction)
    {
        JsonValue Document = GetAuthenticModeDocument();
        MutationFunction(Document);
        return Document.dump();
    }

    bool HasDiagnosticCode(
        const DiagnosticCollection& Diagnostics,
        DiagnosticCode Code
    )
    {
        return std::any_of(
            Diagnostics.begin(),
            Diagnostics.end(),
            [Code](const Diagnostic& Current)
            {
                return Current.Code == Code;
            }
        );
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
    void CheckSourceRejected(
        Mutation MutationFunction,
        DiagnosticCode Expected =
            DiagnosticCode::
                UnsupportedGenshinClientBooleanFilterReflectedDescriptorFamilySourceForm
    )
    {
        const auto Result =
            GenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapter::Adapt(
                MakeMutatedNodeMetadata(MutationFunction),
                GetAuthenticNodeModesJson()
            );
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasDiagnosticCode(Result.error(), Expected));
    }

    std::vector<DescriptorSpecializationFamily> AdaptAuthenticFixture()
    {
        const auto Result =
            GenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapter::Adapt(
                GetAuthenticNodeMetadataJson(),
                GetAuthenticNodeModesJson()
            );
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    DescriptorSpecializationResult SpecializeAuthenticFixture()
    {
        const auto Result = DescriptorFamilySpecializer::Specialize(
            AdaptAuthenticFixture()
        );
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    const NormalizedNodeDescriptorRecord* FindConcreteRecord(
        const DescriptorSpecializationResult& Result,
        std::string_view Key
    )
    {
        for (const NormalizedNodeDescriptorRecord& Record :
            Result.GetConcreteRecords())
        {
            if (Record.GetExternalIdentity().GetKey() == Key)
            {
                return &Record;
            }
        }

        return nullptr;
    }

    DescriptorSpecializationFamily MakeModelFamily(
        std::string FamilyKey,
        std::string ConcreteKey,
        std::string SpecializationKey,
        std::vector<DescriptorSpecializationPin> Pins,
        std::vector<DescriptorSpecializationPinBinding> Bindings,
        std::optional<SourceProvenance> Provenance = std::nullopt
    )
    {
        std::vector<DescriptorSpecializationVariant> Variants;
        Variants.emplace_back(
            ExternalNodeIdentity(std::move(ConcreteKey)),
            std::move(SpecializationKey),
            std::move(Bindings)
        );
        return DescriptorSpecializationFamily(
            ExternalNodeIdentity(std::move(FamilyKey)),
            "Model Family",
            {NodeAvailability::Client},
            std::move(Pins),
            std::nullopt,
            std::move(Provenance),
            std::move(Variants)
        );
    }

    std::vector<DescriptorSpecializationPin> MakeTwoReflectedPins()
    {
        std::vector<DescriptorSpecializationPin> Pins;
        Pins.emplace_back(
            "First",
            std::nullopt,
            true,
            PinDirection::Input,
            PinCategory::Data,
            PinCardinality::Single,
            false
        );
        Pins.emplace_back(
            "Second",
            std::nullopt,
            true,
            PinDirection::Output,
            PinCategory::Data,
            PinCardinality::Single,
            false
        );
        return Pins;
    }

    template<typename Type>
    concept HasDescriptorIdentifierGetter = requires(const Type& Value)
    {
        Value.GetDescriptorIdentifier();
    };

    template<typename Type>
    concept HasGraphSurface = requires(const Type& Value)
    {
        Value.GetGraphIR();
        Value.GetGraphBuilder();
        Value.GetNodeDescriptorRegistry();
        Value.GetSnapshotSchema();
    };

    void TestGenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapterSuccess()
    {
        const auto Families = AdaptAuthenticFixture();
        MPP_CHECK(Families.size() == 1U);
        const DescriptorSpecializationFamily& Family = Families[0U];
        MPP_CHECK(Family.IsValid());
        MPP_CHECK(Family.GetVariants().size() == 2U);
        MPP_CHECK(Family.GetPins().size() == 3U);
        MPP_CHECK(std::all_of(
            Family.GetPins().begin(),
            Family.GetPins().end(),
            [](const DescriptorSpecializationPin& Pin)
            {
                return Pin.IsReflected();
            }
        ));
        MPP_CHECK(!HasDescriptorIdentifierGetter<DescriptorSpecializationFamily>);

        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["reflectMap"][0U]["pins"][0U]["type"] = "bool";
        });
    }

    void TestDescriptorSpecializationFamilyFieldMapping()
    {
        const auto Families = AdaptAuthenticFixture();
        const DescriptorSpecializationFamily& Family = Families[0U];
        MPP_CHECK(Family.GetFamilyExternalIdentity().GetKey() == "200032");
        MPP_CHECK(Family.GetDisplayName() == "获取随机数");
        MPP_CHECK(Family.GetAvailability() ==
            std::vector<NodeAvailability>{NodeAvailability::Client});
        MPP_CHECK(!Family.GetExecutionControlSchema().has_value());
        MPP_CHECK(Family.GetSourceProvenance().has_value());
        MPP_CHECK(
            Family.GetSourceProvenance()->GetSourceDocumentIdentifier() ==
            "布尔过滤器节点\\除法运算_连线.gia"
        );
        MPP_CHECK(
            Family.GetSourceProvenance()->GetSourceRecordIdentifier() == "200032"
        );

        const auto& Pins = Family.GetPins();
        MPP_CHECK(Pins[0U].GetName() == "下限");
        MPP_CHECK(Pins[1U].GetName() == "上限");
        MPP_CHECK(Pins[2U].GetName() == "随机数");
        MPP_CHECK(Pins[0U].GetDirection() == PinDirection::Input);
        MPP_CHECK(Pins[1U].GetDirection() == PinDirection::Input);
        MPP_CHECK(Pins[2U].GetDirection() == PinDirection::Output);
        for (const DescriptorSpecializationPin& Pin : Pins)
        {
            MPP_CHECK(Pin.GetCategory() == PinCategory::Data);
            MPP_CHECK(Pin.GetCardinality() == PinCardinality::Single);
            MPP_CHECK(!Pin.AllowsLiteral());
            MPP_CHECK(!Pin.GetDefaultValue().has_value());
        }
    }

    void TestDescriptorSpecializationFamilyVariantIdentity()
    {
        const auto Families = AdaptAuthenticFixture();
        const auto& Variants = Families[0U].GetVariants();
        MPP_CHECK(
            Variants[0U].GetConcreteExternalIdentity().GetKey() ==
            "family=200032;concrete=1011;variant-bytes=3:3,3"
        );
        MPP_CHECK(
            Variants[1U].GetConcreteExternalIdentity().GetKey() ==
            "family=200032;concrete=1012;variant-bytes=3:7,7"
        );
        MPP_CHECK(Variants[0U].GetSpecializationKey() == "3,3");
        MPP_CHECK(Variants[1U].GetSpecializationKey() == "7,7");

        auto OpaqueFamily = MakeModelFamily(
            "opaque-family",
            "not-a-source-identity",
            "not,a,numeric,key",
            {DescriptorSpecializationPin(
                "Value",
                std::nullopt,
                true,
                PinDirection::Input,
                PinCategory::Data,
                PinCardinality::Single,
                false
            )},
            {DescriptorSpecializationPinBinding(PinIndex(0U), TypeDesc::String())}
        );
        const auto OpaqueResult = DescriptorFamilySpecializer::Specialize(
            {std::move(OpaqueFamily)}
        );
        MPP_CHECK(OpaqueResult.has_value());
        MPP_CHECK(OpaqueResult->IsValid());
        MPP_CHECK(!HasDescriptorIdentifierGetter<DescriptorSpecializationResult>);
    }

    void TestDescriptorSpecializationFamilyReflectionMapping()
    {
        const DescriptorSpecializationResult Result = SpecializeAuthenticFixture();
        MPP_CHECK(Result.IsValid());
        MPP_CHECK(Result.GetConcreteRecords().size() == 2U);

        const NormalizedNodeDescriptorRecord* IntegerRecord = FindConcreteRecord(
            Result,
            "family=200032;concrete=1011;variant-bytes=3:3,3"
        );
        const NormalizedNodeDescriptorRecord* FloatRecord = FindConcreteRecord(
            Result,
            "family=200032;concrete=1012;variant-bytes=3:7,7"
        );
        MPP_CHECK(IntegerRecord != nullptr);
        MPP_CHECK(FloatRecord != nullptr);
        for (const NormalizedPinRecord& Pin : IntegerRecord->GetPins())
        {
            MPP_CHECK(Pin.GetType() == TypeDesc::Integer());
        }
        for (const NormalizedPinRecord& Pin : FloatRecord->GetPins())
        {
            MPP_CHECK(Pin.GetType() == TypeDesc::Float());
        }

        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["reflectMap"][0U]["pins"].erase(2U);
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["reflectMap"][0U]["pins"][2U]["type"] = "float";
            Document[0U]["reflectMap"][0U]["pins"][2U]["clientVarType"] = 7;
            Document[0U]["reflectMap"][0U]["pins"][2U]["connectionType"] = 7;
        });
    }

    void TestDescriptorSpecializationFamilyFixedPinAndTypeBoundaries()
    {
        std::vector<DescriptorSpecializationPin> Pins;
        Pins.emplace_back(
            "Fixed",
            TypeDesc::Integer(),
            false,
            PinDirection::Input,
            PinCategory::Data,
            PinCardinality::Single,
            true,
            LiteralValue(std::int64_t(42))
        );
        Pins.emplace_back(
            "Reflected",
            std::nullopt,
            true,
            PinDirection::Output,
            PinCategory::Data,
            PinCardinality::Single,
            false
        );
        auto Family = MakeModelFamily(
            "model-fixed",
            "model-fixed-vector",
            "opaque-vector",
            std::move(Pins),
            {DescriptorSpecializationPinBinding(PinIndex(1U), TypeDesc::Vector3())}
        );
        const auto Result = DescriptorFamilySpecializer::Specialize(
            {std::move(Family)}
        );
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->IsValid());
        const auto& RecordPins = Result->GetConcreteRecords()[0U].GetPins();
        MPP_CHECK(RecordPins[0U].GetType() == TypeDesc::Integer());
        MPP_CHECK(RecordPins[0U].AllowsLiteral());
        MPP_CHECK(RecordPins[0U].GetDefaultValue().has_value());
        MPP_CHECK(RecordPins[1U].GetType() == TypeDesc::Vector3());

        auto IndependentFamily = MakeModelFamily(
            "model-independent",
            "model-independent-concrete",
            "opaque-independent",
            MakeTwoReflectedPins(),
            {
                DescriptorSpecializationPinBinding(PinIndex(0U), TypeDesc::String()),
                DescriptorSpecializationPinBinding(PinIndex(1U), TypeDesc::Vector3())
            }
        );
        const auto IndependentResult = DescriptorFamilySpecializer::Specialize(
            {std::move(IndependentFamily)}
        );
        MPP_CHECK(IndependentResult.has_value());
        MPP_CHECK(
            IndependentResult->GetConcreteRecords()[0U].GetPins()[0U].GetType() ==
            TypeDesc::String()
        );
        MPP_CHECK(
            IndependentResult->GetConcreteRecords()[0U].GetPins()[1U].GetType() ==
            TypeDesc::Vector3()
        );

        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["reflectMap"][0U]["pins"][0U]["type"] = "vec3";
            Document[0U]["reflectMap"][0U]["pins"][0U]["clientVarType"] = 11;
            Document[0U]["reflectMap"][0U]["pins"][0U]["connectionType"] = 11;
        });
    }

    void TestDescriptorSpecializationFamilyDefaultsAndProvenance()
    {
        const auto Families = AdaptAuthenticFixture();
        for (const DescriptorSpecializationPin& Pin : Families[0U].GetPins())
        {
            MPP_CHECK(!Pin.GetDefaultValue().has_value());
        }

        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["inputs"][0U]["defaultValue"] = 0;
        });

        std::vector<DescriptorSpecializationPin> Pins;
        Pins.emplace_back(
            "Reflected",
            std::nullopt,
            true,
            PinDirection::Input,
            PinCategory::Data,
            PinCardinality::Single,
            true,
            LiteralValue(std::int64_t(7))
        );
        auto Family = MakeModelFamily(
            "default-family",
            "default-concrete",
            "default-key",
            std::move(Pins),
            {DescriptorSpecializationPinBinding(PinIndex(0U), TypeDesc::Integer())},
            SourceProvenance("logical-family-source", "default-family")
        );
        const auto Result = DescriptorFamilySpecializer::Specialize(
            {std::move(Family)}
        );
        MPP_CHECK(Result.has_value());
        const auto& Record = Result->GetConcreteRecords()[0U];
        MPP_CHECK(Record.GetPins()[0U].GetDefaultValue().has_value());
        MPP_CHECK(Record.GetSourceProvenance().has_value());
        MPP_CHECK(
            Record.GetSourceProvenance()->GetSourceDocumentIdentifier() ==
            "logical-family-source"
        );
        MPP_CHECK(Record.GetExternalIdentity().GetKey() == "default-concrete");
    }

    void TestDescriptorSpecializationFamilyDeterminism()
    {
        const auto FirstAdaptation = AdaptAuthenticFixture();
        const auto SecondAdaptation = AdaptAuthenticFixture();
        MPP_CHECK(FirstAdaptation == SecondAdaptation);

        const auto FirstResult = DescriptorFamilySpecializer::Specialize(
            FirstAdaptation
        );
        MPP_CHECK(FirstResult.has_value());

        // Family construction canonicalizes variants; reconstruct to exercise input order.
        std::vector<DescriptorSpecializationVariant> ReversedVariants =
            SecondAdaptation[0U].GetVariants();
        std::reverse(ReversedVariants.begin(), ReversedVariants.end());
        DescriptorSpecializationFamily Reconstructed(
            SecondAdaptation[0U].GetFamilyExternalIdentity(),
            SecondAdaptation[0U].GetDisplayName(),
            SecondAdaptation[0U].GetAvailability(),
            SecondAdaptation[0U].GetPins(),
            SecondAdaptation[0U].GetExecutionControlSchema(),
            SecondAdaptation[0U].GetSourceProvenance(),
            std::move(ReversedVariants)
        );
        const auto SecondResult = DescriptorFamilySpecializer::Specialize(
            {std::move(Reconstructed)}
        );
        MPP_CHECK(SecondResult.has_value());
        MPP_CHECK(*FirstResult == *SecondResult);

        auto ModelFamilyA = MakeModelFamily(
            "deterministic-family-a",
            "deterministic-concrete-a",
            "deterministic-key-a",
            MakeTwoReflectedPins(),
            {
                DescriptorSpecializationPinBinding(
                    PinIndex(1U), TypeDesc::Vector3()),
                DescriptorSpecializationPinBinding(
                    PinIndex(0U), TypeDesc::String())
            }
        );
        auto ModelFamilyB = MakeModelFamily(
            "deterministic-family-b",
            "deterministic-concrete-b",
            "deterministic-key-b",
            MakeTwoReflectedPins(),
            {
                DescriptorSpecializationPinBinding(
                    PinIndex(1U), TypeDesc::Float()),
                DescriptorSpecializationPinBinding(
                    PinIndex(0U), TypeDesc::Integer())
            }
        );
        const auto CanonicalModelResult = DescriptorFamilySpecializer::Specialize(
            {ModelFamilyA, ModelFamilyB}
        );
        const auto PermutedModelResult = DescriptorFamilySpecializer::Specialize(
            {std::move(ModelFamilyB), std::move(ModelFamilyA)}
        );
        MPP_CHECK(CanonicalModelResult.has_value());
        MPP_CHECK(PermutedModelResult.has_value());
        MPP_CHECK(*CanonicalModelResult == *PermutedModelResult);
        MPP_CHECK(
            CanonicalModelResult->GetFamilies()[0U].GetVariants()[0U]
                .GetPinBindings()[0U].GetFamilyPinIndex() == PinIndex(0U)
        );

        const auto FirstFailure = DescriptorFamilySpecializer::Specialize(
            {MakeModelFamily(
                "failure-family",
                "failure-concrete",
                "failure-key",
                MakeTwoReflectedPins(),
                {DescriptorSpecializationPinBinding(
                    PinIndex(0U), TypeDesc::Integer())}
            )}
        );
        const auto SecondFailure = DescriptorFamilySpecializer::Specialize(
            {MakeModelFamily(
                "failure-family",
                "failure-concrete",
                "failure-key",
                MakeTwoReflectedPins(),
                {DescriptorSpecializationPinBinding(
                    PinIndex(0U), TypeDesc::Integer())}
            )}
        );
        MPP_CHECK(!FirstFailure.has_value());
        MPP_CHECK(!SecondFailure.has_value());
        MPP_CHECK(SameDiagnostics(FirstFailure.error(), SecondFailure.error()));
    }

    void TestDescriptorSpecializationFamilyDiagnostics()
    {
        const auto Malformed =
            GenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapter::Adapt(
                "[",
                GetAuthenticNodeModesJson()
            );
        MPP_CHECK(!Malformed.has_value());
        MPP_CHECK(HasDiagnosticCode(
            Malformed.error(),
            DiagnosticCode::
                MalformedGenshinClientBooleanFilterReflectedDescriptorFamilySource
        ));

        const auto WrongRoot =
            GenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapter::Adapt(
                "{}",
                GetAuthenticNodeModesJson()
            );
        MPP_CHECK(!WrongRoot.has_value());
        MPP_CHECK(HasDiagnosticCode(
            WrongRoot.error(),
            DiagnosticCode::
                MalformedGenshinClientBooleanFilterReflectedDescriptorFamilySource
        ));

        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U].erase("displayName");
        }, DiagnosticCode::
            MissingGenshinClientBooleanFilterReflectedDescriptorFamilySourceField);
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["subType"] = "int_filter";
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["nodeType"] = "other";
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["graphType"] = 20002;
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["genericId"] = 200033;
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["unknown"] = true;
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["inputs"][0U]["name"] = "wrong-name";
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["reflectMap"][0U]["concreteId"] = 1012;
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["reflectMap"][0U]["variantKey"] = "7,7";
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["reflectMap"][0U]["pins"][0U]["indexOfConcrete"] = 1;
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["reflectMap"][0U]["pins"][0U]["clientVarType"] = 7;
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["reflectMap"][0U]["pins"][0U]["connectionType"] = 7;
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["reflectMap"][0U]["pins"][0U]["index"] = 1;
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["reflectMap"][0U]["pins"][2U]["type"] = "float";
        });
        CheckSourceRejected([](JsonValue& Document)
        {
            Document[0U]["reflectMap"][0U]["pins"][0U]["type"] = "str";
        });

        const auto ModeFailure =
            GenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapter::Adapt(
                GetAuthenticNodeMetadataJson(),
                MakeMutatedModeMetadata([](JsonValue& Document)
                {
                    auto& Values =
                        Document["graphs"]["bool_filter"]["classic"]["genericIds"];
                    Values.erase(std::find(Values.begin(), Values.end(), 200032));
                })
            );
        MPP_CHECK(!ModeFailure.has_value());
        MPP_CHECK(HasDiagnosticCode(
            ModeFailure.error(),
            DiagnosticCode::
                UnsupportedGenshinClientBooleanFilterReflectedDescriptorFamilySourceForm
        ));

        auto MissingBindingFamily = MakeModelFamily(
            "missing-binding-family",
            "missing-binding-concrete",
            "missing-binding-key",
            MakeTwoReflectedPins(),
            {DescriptorSpecializationPinBinding(PinIndex(0U), TypeDesc::Integer())}
        );
        const auto MissingBinding = DescriptorFamilySpecializer::Specialize(
            {std::move(MissingBindingFamily)}
        );
        MPP_CHECK(!MissingBinding.has_value());
        MPP_CHECK(HasDiagnosticCode(
            MissingBinding.error(),
            DiagnosticCode::InvalidDescriptorSpecializationVariantBinding
        ));
        MPP_CHECK(!HasDiagnosticCode(
            MissingBinding.error(),
            DiagnosticCode::InvalidDescriptorSpecializationFamily
        ));

        auto DuplicateBindingFamily = MakeModelFamily(
            "duplicate-binding-family",
            "duplicate-binding-concrete",
            "duplicate-binding-key",
            {DescriptorSpecializationPin(
                "Value",
                std::nullopt,
                true,
                PinDirection::Input,
                PinCategory::Data,
                PinCardinality::Single,
                false
            )},
            {
                DescriptorSpecializationPinBinding(PinIndex(0U), TypeDesc::Integer()),
                DescriptorSpecializationPinBinding(PinIndex(0U), TypeDesc::Float())
            }
        );
        const auto DuplicateBinding = DescriptorFamilySpecializer::Specialize(
            {std::move(DuplicateBindingFamily)}
        );
        MPP_CHECK(!DuplicateBinding.has_value());
        MPP_CHECK(HasDiagnosticCode(
            DuplicateBinding.error(),
            DiagnosticCode::InvalidDescriptorSpecializationVariantBinding
        ));
        MPP_CHECK(!HasDiagnosticCode(
            DuplicateBinding.error(),
            DiagnosticCode::InvalidDescriptorSpecializationFamily
        ));

        auto UnknownBindingFamily = MakeModelFamily(
            "unknown-binding-family",
            "unknown-binding-concrete",
            "unknown-binding-key",
            {DescriptorSpecializationPin(
                "Value",
                std::nullopt,
                true,
                PinDirection::Input,
                PinCategory::Data,
                PinCardinality::Single,
                false
            )},
            {DescriptorSpecializationPinBinding(PinIndex(3U), TypeDesc::Integer())}
        );
        const auto UnknownBinding = DescriptorFamilySpecializer::Specialize(
            {std::move(UnknownBindingFamily)}
        );
        MPP_CHECK(!UnknownBinding.has_value());
        MPP_CHECK(HasDiagnosticCode(
            UnknownBinding.error(),
            DiagnosticCode::InvalidDescriptorSpecializationVariantBinding
        ));
        MPP_CHECK(!HasDiagnosticCode(
            UnknownBinding.error(),
            DiagnosticCode::InvalidDescriptorSpecializationFamily
        ));

        std::vector<DescriptorSpecializationPin> FixedPins;
        FixedPins.emplace_back(
            "Fixed",
            TypeDesc::Integer(),
            false,
            PinDirection::Input,
            PinCategory::Data,
            PinCardinality::Single,
            false
        );
        FixedPins.emplace_back(
            "Reflected",
            std::nullopt,
            true,
            PinDirection::Output,
            PinCategory::Data,
            PinCardinality::Single,
            false
        );
        auto FixedBindingFamily = MakeModelFamily(
            "fixed-binding-family",
            "fixed-binding-concrete",
            "fixed-binding-key",
            std::move(FixedPins),
            {
                DescriptorSpecializationPinBinding(PinIndex(0U), TypeDesc::Integer()),
                DescriptorSpecializationPinBinding(PinIndex(1U), TypeDesc::Integer())
            }
        );
        const auto FixedBinding = DescriptorFamilySpecializer::Specialize(
            {std::move(FixedBindingFamily)}
        );
        MPP_CHECK(!FixedBinding.has_value());
        MPP_CHECK(HasDiagnosticCode(
            FixedBinding.error(),
            DiagnosticCode::InvalidDescriptorSpecializationVariantBinding
        ));

        auto InvalidTypeFamily = MakeModelFamily(
            "invalid-type-family",
            "invalid-type-concrete",
            "invalid-type-key",
            {DescriptorSpecializationPin(
                "Value",
                std::nullopt,
                true,
                PinDirection::Input,
                PinCategory::Data,
                PinCardinality::Single,
                false
            )},
            {DescriptorSpecializationPinBinding(PinIndex(0U), TypeDesc())}
        );
        const auto InvalidType = DescriptorFamilySpecializer::Specialize(
            {std::move(InvalidTypeFamily)}
        );
        MPP_CHECK(!InvalidType.has_value());
        MPP_CHECK(HasDiagnosticCode(
            InvalidType.error(),
            DiagnosticCode::InvalidDescriptorSpecializationVariantBinding
        ));

        std::vector<DescriptorSpecializationPin> DuplicatePins;
        DuplicatePins.emplace_back(
            "Value",
            std::nullopt,
            true,
            PinDirection::Input,
            PinCategory::Data,
            PinCardinality::Single,
            false
        );
        std::vector<DescriptorSpecializationVariant> DuplicateVariants;
        DuplicateVariants.emplace_back(
            ExternalNodeIdentity("duplicate-concrete-a"),
            "duplicate-key",
            std::vector<DescriptorSpecializationPinBinding>{
                DescriptorSpecializationPinBinding(
                    PinIndex(0U), TypeDesc::Integer())
            }
        );
        DuplicateVariants.emplace_back(
            ExternalNodeIdentity("duplicate-concrete-b"),
            "duplicate-key",
            std::vector<DescriptorSpecializationPinBinding>{
                DescriptorSpecializationPinBinding(
                    PinIndex(0U), TypeDesc::Float())
            }
        );
        DescriptorSpecializationFamily DuplicateVariantFamily(
            ExternalNodeIdentity("duplicate-variant-family"),
            "Duplicate Variant Family",
            {NodeAvailability::Client},
            std::move(DuplicatePins),
            std::nullopt,
            std::nullopt,
            std::move(DuplicateVariants)
        );
        const auto DuplicateVariant = DescriptorFamilySpecializer::Specialize(
            {std::move(DuplicateVariantFamily)}
        );
        MPP_CHECK(!DuplicateVariant.has_value());
        MPP_CHECK(HasDiagnosticCode(
            DuplicateVariant.error(),
            DiagnosticCode::DuplicateDescriptorSpecializationVariant
        ));
        MPP_CHECK(!HasDiagnosticCode(
            DuplicateVariant.error(),
            DiagnosticCode::InvalidDescriptorSpecializationFamily
        ));

        std::vector<DescriptorSpecializationVariant> DuplicateIdentityVariants;
        DuplicateIdentityVariants.emplace_back(
            ExternalNodeIdentity("duplicate-shared-concrete"),
            "identity-key-a",
            std::vector<DescriptorSpecializationPinBinding>{
                DescriptorSpecializationPinBinding(
                    PinIndex(0U), TypeDesc::Integer())
            }
        );
        DuplicateIdentityVariants.emplace_back(
            ExternalNodeIdentity("duplicate-shared-concrete"),
            "identity-key-b",
            std::vector<DescriptorSpecializationPinBinding>{
                DescriptorSpecializationPinBinding(
                    PinIndex(0U), TypeDesc::Float())
            }
        );
        DescriptorSpecializationFamily DuplicateIdentityFamily(
            ExternalNodeIdentity("duplicate-identity-family"),
            "Duplicate Identity Family",
            {NodeAvailability::Client},
            {DescriptorSpecializationPin(
                "Value",
                std::nullopt,
                true,
                PinDirection::Input,
                PinCategory::Data,
                PinCardinality::Single,
                false
            )},
            std::nullopt,
            std::nullopt,
            std::move(DuplicateIdentityVariants)
        );
        const auto DuplicateIdentity = DescriptorFamilySpecializer::Specialize(
            {std::move(DuplicateIdentityFamily)}
        );
        MPP_CHECK(!DuplicateIdentity.has_value());
        MPP_CHECK(HasDiagnosticCode(
            DuplicateIdentity.error(),
            DiagnosticCode::DuplicateDescriptorSpecializationVariant
        ));
        MPP_CHECK(!HasDiagnosticCode(
            DuplicateIdentity.error(),
            DiagnosticCode::InvalidDescriptorSpecializationFamily
        ));

        auto CrossFamilyA = MakeModelFamily(
            "cross-family-a",
            "shared-concrete",
            "key-a",
            {DescriptorSpecializationPin(
                "ValueA",
                std::nullopt,
                true,
                PinDirection::Input,
                PinCategory::Data,
                PinCardinality::Single,
                false
            )},
            {DescriptorSpecializationPinBinding(PinIndex(0U), TypeDesc::Integer())}
        );
        auto CrossFamilyB = MakeModelFamily(
            "cross-family-b",
            "shared-concrete",
            "key-b",
            {DescriptorSpecializationPin(
                "ValueB",
                std::nullopt,
                true,
                PinDirection::Input,
                PinCategory::Data,
                PinCardinality::Single,
                false
            )},
            {DescriptorSpecializationPinBinding(PinIndex(0U), TypeDesc::Float())}
        );
        const auto CrossFamily = DescriptorFamilySpecializer::Specialize(
            {std::move(CrossFamilyB), std::move(CrossFamilyA)}
        );
        MPP_CHECK(!CrossFamily.has_value());
        MPP_CHECK(HasDiagnosticCode(
            CrossFamily.error(),
            DiagnosticCode::DuplicateExternalNodeIdentity
        ));
    }

    void TestDescriptorSpecializationFamilyFailureAtomicity()
    {
        const auto SourceFailure =
            GenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapter::Adapt(
                MakeMutatedNodeMetadata([](JsonValue& Document)
                {
                    Document[0U]["reflectMap"][0U]["pins"].erase(0U);
                }),
                GetAuthenticNodeModesJson()
            );
        MPP_CHECK(!SourceFailure.has_value());

        auto BindingFailureFamily = MakeModelFamily(
            "atomic-family",
            "atomic-concrete",
            "atomic-key",
            MakeTwoReflectedPins(),
            {DescriptorSpecializationPinBinding(PinIndex(0U), TypeDesc::Integer())}
        );
        const auto BindingFailure = DescriptorFamilySpecializer::Specialize(
            {std::move(BindingFailureFamily)}
        );
        MPP_CHECK(!BindingFailure.has_value());

        std::vector<DescriptorSpecializationVariant> DuplicateVariants;
        DuplicateVariants.emplace_back(
            ExternalNodeIdentity("atomic-duplicate-concrete-a"),
            "atomic-duplicate-key",
            std::vector<DescriptorSpecializationPinBinding>{
                DescriptorSpecializationPinBinding(
                    PinIndex(0U), TypeDesc::Integer())
            }
        );
        DuplicateVariants.emplace_back(
            ExternalNodeIdentity("atomic-duplicate-concrete-b"),
            "atomic-duplicate-key",
            std::vector<DescriptorSpecializationPinBinding>{
                DescriptorSpecializationPinBinding(
                    PinIndex(0U), TypeDesc::Float())
            }
        );
        const auto DuplicateVariantFailure = DescriptorFamilySpecializer::Specialize(
            {DescriptorSpecializationFamily(
                ExternalNodeIdentity("atomic-duplicate-family"),
                "Atomic Duplicate Family",
                {NodeAvailability::Client},
                MakeTwoReflectedPins(),
                std::nullopt,
                std::nullopt,
                std::move(DuplicateVariants)
            )}
        );
        MPP_CHECK(!DuplicateVariantFailure.has_value());
        MPP_CHECK(HasDiagnosticCode(
            DuplicateVariantFailure.error(),
            DiagnosticCode::DuplicateDescriptorSpecializationVariant
        ));
        MPP_CHECK(!HasDiagnosticCode(
            DuplicateVariantFailure.error(),
            DiagnosticCode::InvalidDescriptorSpecializationFamily
        ));

        std::vector<DescriptorSpecializationPin> InvalidDefaultPins;
        InvalidDefaultPins.emplace_back(
            "InvalidDefault",
            std::nullopt,
            true,
            PinDirection::Input,
            PinCategory::Data,
            PinCardinality::Single,
            true,
            LiteralValue(std::int64_t(1))
        );
        auto InvalidDefaultFamily = MakeModelFamily(
            "invalid-default-family",
            "invalid-default-concrete",
            "invalid-default-key",
            std::move(InvalidDefaultPins),
            {DescriptorSpecializationPinBinding(PinIndex(0U), TypeDesc::Vector3())}
        );
        const auto InvalidDefault = DescriptorFamilySpecializer::Specialize(
            {std::move(InvalidDefaultFamily)}
        );
        MPP_CHECK(!InvalidDefault.has_value());
        MPP_CHECK(HasDiagnosticCode(
            InvalidDefault.error(),
            DiagnosticCode::InvalidNormalizedDescriptorRecord
        ));

        const auto ValidAfterFailures = DescriptorFamilySpecializer::Specialize(
            AdaptAuthenticFixture()
        );
        MPP_CHECK(ValidAfterFailures.has_value());
        MPP_CHECK(ValidAfterFailures->IsValid());
        MPP_CHECK(ValidAfterFailures->GetConcreteRecords().size() == 2U);
    }

    void TestDescriptorSpecializationFamilyCatalogueIntegration()
    {
        const DescriptorSpecializationResult Result = SpecializeAuthenticFixture();
        MPP_CHECK(Result.IsValid());

        const std::string SourceNamespace =
            "genshin.client-bool-filter-reflected-descriptor-family-source";
        const std::string SourceRevision =
            "genshin-ts@26bdf2a9a3fadba934423940489236f0b53eb3ea;"
            "client_node_metadata.json@93237c724f6453650ae9394077620c6e0fddb3d3;"
            "client_node_modes.json@b7e14a0dd7102ccd682235cf958a2d3d36378033";

        const auto Catalogue = DescriptorCatalogueBuilder::Build(
            SourceNamespace,
            SourceRevision,
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            Result.GetConcreteRecords()
        );
        MPP_CHECK(Catalogue.has_value());
        MPP_CHECK(Catalogue->IsValid());
        MPP_CHECK(Catalogue->GetEntryCount() == 2U);
        MPP_CHECK(
            Catalogue->GetEntries()[0U].GetDescriptorIdentifier() ==
            NodeDescriptorId(1U)
        );
        MPP_CHECK(
            Catalogue->GetEntries()[1U].GetDescriptorIdentifier() ==
            NodeDescriptorId(2U)
        );

        const auto RepeatedCatalogue = DescriptorCatalogueBuilder::Build(
            SourceNamespace,
            SourceRevision,
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            SpecializeAuthenticFixture().GetConcreteRecords()
        );
        MPP_CHECK(RepeatedCatalogue.has_value());
        MPP_CHECK(*Catalogue == *RepeatedCatalogue);
        MPP_CHECK(Result.GetFamilies().size() == 1U);
        MPP_CHECK(Result.GetFamilies()[0U].GetVariants().size() == 2U);
    }

    void TestDescriptorSpecializationFamilyScopeBoundaries()
    {
        using FamilyVector = std::vector<DescriptorSpecializationFamily>;
        using RecordVector = std::vector<NormalizedNodeDescriptorRecord>;
        using ExpectedAdaptation = std::expected<FamilyVector, DiagnosticCollection>;
        using ExpectedSpecialization =
            std::expected<DescriptorSpecializationResult, DiagnosticCollection>;

        static_assert(!std::is_constructible_v<
            DescriptorSpecializationResult,
            FamilyVector,
            RecordVector
        >);
        static_assert(std::is_same_v<
            decltype(
                GenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapter::
                    Adapt(std::string(), std::string())
            ),
            ExpectedAdaptation
        >);
        static_assert(std::is_same_v<
            decltype(DescriptorFamilySpecializer::Specialize(FamilyVector())),
            ExpectedSpecialization
        >);
        static_assert(!HasDescriptorIdentifierGetter<DescriptorSpecializationFamily>);
        static_assert(!HasDescriptorIdentifierGetter<DescriptorSpecializationResult>);
        static_assert(!HasGraphSurface<DescriptorFamilySpecializer>);
        static_assert(!HasGraphSurface<
            GenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapter
        >);

        const DescriptorSpecializationResult Result = SpecializeAuthenticFixture();
        MPP_CHECK(Result.IsValid());
    }
}

int main()
{
    TestGenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapterSuccess();
    TestDescriptorSpecializationFamilyFieldMapping();
    TestDescriptorSpecializationFamilyVariantIdentity();
    TestDescriptorSpecializationFamilyReflectionMapping();
    TestDescriptorSpecializationFamilyFixedPinAndTypeBoundaries();
    TestDescriptorSpecializationFamilyDefaultsAndProvenance();
    TestDescriptorSpecializationFamilyDeterminism();
    TestDescriptorSpecializationFamilyDiagnostics();
    TestDescriptorSpecializationFamilyFailureAtomicity();
    TestDescriptorSpecializationFamilyCatalogueIntegration();
    TestDescriptorSpecializationFamilyScopeBoundaries();
    return EXIT_SUCCESS;
}
