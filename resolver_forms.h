#pragma once
// Generated from documented native instruction forms; no DLL hashes or fixed addresses.
namespace RuntimeResolver {
enum class Id : unsigned {OnUpdate,Shutdown,IncRefs,DecRefs,LoadScript,SetTop,ClientInitialize,PushString,PushClosure,SetTable,CheckStack,Index,LifetimeDtor,AddMember,StaticFloatType,StaticBoolType,StaticUInt16Type,StaticEntityType,Alloc,Free,ValidateMap,SerializeRaw,UnserializeRaw,GetRawObject,PushInternalFunction,DispatchEvent,InheritProperties,ProcessDefinitions,RegisterClass,FixedStringCreate,FixedStringText,FixedStringRelease,ArgError,LuaError,TypeInitialize,ResizeStructs,LifetimeAllocate,LifetimePush,ContextAccessor,InputAccessor,CameraConstruct,ValidateObject,FinalizeClass,ProfilerCtor,ProfilerDtor,TLSIndex,StructRegistry,StaticStructs,Count};
struct Mask {uint32_t offset;uint8_t bytes;};
struct Relation {uint32_t offset,end;Id target;int32_t addend;};
struct Form {Id id;uint32_t bytes;char const* sha;Mask const* masks;size_t maskCount;Relation const* relations;size_t relationCount;};
struct Name {Id id;char const* text;};
inline constexpr Name Names[]={
{Id::OnUpdate,"bg3se::ecl::ExtensionState::OnUpdate"},
{Id::Shutdown,"bg3se::ecl::ExtensionState::ShutdownLuaState"},
{Id::IncRefs,"bg3se::ExtensionStateBase::IncLuaRefs"},
{Id::DecRefs,"bg3se::ExtensionStateBase::DecLuaRefs"},
{Id::LoadScript,"bg3se::lua::State::LoadScript"},
{Id::SetTop,"lua_settop"},
{Id::ClientInitialize,"bg3se::ecl::lua::ClientState::Initialize"},
{Id::PushString,"lua_pushstring"},
{Id::PushClosure,"lua_pushcclosure"},
{Id::SetTable,"lua_settable"},
{Id::CheckStack,"lua_checkstack"},
{Id::Index,"index2addr"},
{Id::LifetimeDtor,"bg3se::lua::LifetimeStackPin::~LifetimeStackPin"},
{Id::AddMember,"bg3se::TypeInformation::AddMember"},
{Id::StaticFloatType,"bg3se::GetStaticTypeInfo<float>"},
{Id::StaticBoolType,"bg3se::GetStaticTypeInfo<bool>"},
{Id::StaticUInt16Type,"bg3se::GetStaticTypeInfo<unsigned short>"},
{Id::StaticEntityType,"bg3se::GetStaticTypeInfo<bg3se::TypedHandle<bg3se::EntityHandleTag> >"},
{Id::Alloc,"bg3se::BG3Alloc"},
{Id::Free,"bg3se::BG3Free"},
{Id::ValidateMap,"bg3se::lua::GenericPropertyMap::ValidatePropertyMap"},
{Id::SerializeRaw,"bg3se::lua::SerializeRawObject"},
{Id::UnserializeRaw,"bg3se::lua::UnserializeRawObjectFromTable"},
{Id::GetRawObject,"bg3se::lua::ObjectProxy::GetRaw"},
{Id::PushInternalFunction,"bg3se::lua::PushInternalFunction"},
{Id::DispatchEvent,"bg3se::lua::State::DispatchEvent"},
{Id::InheritProperties,"bg3se::lua::InheritProperties"},
{Id::ProcessDefinitions,"bg3se::lua::ProcessPropertyMapDefinitions"},
{Id::RegisterClass,"bg3se::RegisterClassDefn"},
{Id::FixedStringCreate,"bg3se::FixedStringBase::CreateFixedString"},
{Id::FixedStringText,"bg3se::FixedStringBase::GetPooledStringPtr"},
{Id::FixedStringRelease,"bg3se::FixedStringBase::StaticDecRef"},
{Id::ArgError,"luaL_argerror"},
{Id::LuaError,"luaL_error"},
{Id::TypeInitialize,"bg3se::TypeInformationRepository::Initialize"},
{Id::ResizeStructs,"bg3se::Array<bg3se::StaticTypeInformation>::resize"},
{Id::LifetimeAllocate,"bg3se::lua::LifetimePool::Allocate"},
{Id::LifetimePush,"std::vector<bg3se::lua::LifetimeHandle,bg3se::GameAllocator<bg3se::lua::LifetimeHandle> >::push_back"},
{Id::ContextAccessor,"bg3se::ScriptExtender::GetCurrentExtensionState"},
{Id::InputAccessor,"bg3se::lua::GenericGetValue<bg3se::input::InputEvent>"},
{Id::CameraConstruct,"bg3se::lua::DefaultConstruct<bg3se::ecl::GameCameraBehavior>"},
{Id::ValidateObject,"bg3se::lua::GenericPropertyMap::ValidateObject"},
{Id::FinalizeClass,"bg3se::FinalizeClassDefn"},
{Id::ProfilerCtor,"bg3se::lua::ProfilerStackGuard::ProfilerStackGuard"},
{Id::ProfilerDtor,"bg3se::lua::ProfilerStackGuard::~ProfilerStackGuard"},
};
inline constexpr Mask Masks0[]={{15,4},{41,4},{66,4},{116,4},{158,4},{192,4},{209,4},{267,4},{279,4},{291,4},{304,4},{340,4},{351,4}};

inline constexpr Mask Masks1[]={{26,4},{70,4},{83,4}};

inline constexpr Mask Masks2[]={{17,4},{59,4},{91,4},{102,4}};

inline constexpr Mask Masks3[]={{60,4},{77,4}};

inline constexpr Mask Masks4[]={{31,4},{109,4},{122,4},{147,4},{161,4},{169,4},{178,4},{226,4},{245,4},{268,4},{285,2},{405,4},{435,4},{442,4},{483,4},{494,4},{504,4},{571,4},{588,4},{614,4},{639,4},{653,4},{661,4},{670,4},{718,4},{737,4},{760,4},{777,2},{896,4},{924,4},{931,4},{972,4},{983,4},{993,4},{1024,4},{1096,4},{1126,4}};
inline constexpr Relation Relations4[]={{571,575,Id::LifetimeAllocate,0},{588,592,Id::LifetimePush,0}};


inline constexpr Mask Masks6[]={{19,4},{42,4},{77,4},{87,4},{106,4},{125,4},{133,4},{166,4},{176,4},{195,4},{214,4},{222,4},{259,4},{272,4}};

inline constexpr Mask Masks7[]={{31,4},{76,4}};

inline constexpr Mask Masks8[]={{88,4},{200,4}};

inline constexpr Mask Masks9[]={{19,4},{58,4},{93,4}};
inline constexpr Relation Relations9[]={{19,23,Id::Index,0}};
inline constexpr Mask Masks10[]={{13,4},{105,4},{128,4},{133,4}};

inline constexpr Mask Masks11[]={{33,4}};

inline constexpr Mask Masks12[]={{27,4}};

inline constexpr Mask Masks13[]={{28,4},{50,4},{85,4},{95,4},{106,4}};
inline constexpr Relation Relations13[]={{50,54,Id::FixedStringCreate,0},{95,99,Id::FixedStringRelease,0},{106,110,Id::FixedStringRelease,0}};
inline constexpr Mask Masks14[]={{7,4},{21,4},{48,4},{57,4},{62,4},{68,4},{80,4},{87,4},{94,4},{99,4},{106,4},{119,4}};
inline constexpr Relation Relations14[]={{21,25,Id::TLSIndex,0}};
inline constexpr Mask Masks15[]={{7,4},{21,4},{48,4},{57,4},{62,4},{68,4},{80,4},{87,4},{94,4},{99,4},{106,4},{119,4}};
inline constexpr Relation Relations15[]={{21,25,Id::TLSIndex,0}};
inline constexpr Mask Masks16[]={{7,4},{21,4},{48,4},{57,4},{62,4},{68,4},{80,4},{87,4},{94,4},{99,4},{106,4},{119,4}};
inline constexpr Relation Relations16[]={{21,25,Id::TLSIndex,0}};
inline constexpr Mask Masks17[]={{7,4},{21,4},{48,4},{57,4},{62,4},{68,4},{80,4},{87,4},{94,4},{99,4},{106,4},{119,4}};
inline constexpr Relation Relations17[]={{21,25,Id::TLSIndex,0}};
inline constexpr Mask Masks18[]={{13,4},{104,4}};

inline constexpr Mask Masks19[]={{16,4},{90,4}};

inline constexpr Mask Masks20[]={{9,4},{37,4},{48,4},{76,4},{118,4},{128,4},{135,4},{149,4}};
inline constexpr Relation Relations20[]={{37,41,Id::ValidateObject,0},{48,52,Id::ValidateObject,0},{76,80,Id::StructRegistry,16},{118,122,Id::FixedStringText,0}};
inline constexpr Mask Masks21[]={{27,4},{38,4},{75,4},{94,4},{178,4},{189,4},{208,4}};
inline constexpr Relation Relations21[]={{27,31,Id::CheckStack,0},{178,182,Id::FixedStringText,0}};
inline constexpr Mask Masks22[]={{83,4},{93,4},{108,4},{122,4},{133,4}};
inline constexpr Relation Relations22[]={{83,87,Id::FixedStringText,0},{108,112,Id::Index,0}};
inline constexpr Mask Masks23[]={{37,4},{76,4},{231,4},{379,4},{389,4},{397,4},{407,4},{419,4},{437,4},{445,4},{455,4},{467,4},{485,4},{493,4},{504,4},{516,4},{534,4},{542,4},{553,4},{561,4}};
inline constexpr Relation Relations23[]={{231,235,Id::StructRegistry,0},{397,401,Id::LuaError,0},{445,449,Id::LuaError,0},{493,497,Id::LuaError,0},{542,546,Id::LuaError,0},{561,565,Id::LuaError,0}};
inline constexpr Mask Masks24[]={{30,4},{40,4},{48,4},{59,4},{71,4},{90,4},{117,4},{136,4}};

inline constexpr Mask Masks25[]={{18,4},{128,4},{150,4},{172,4},{186,4},{211,4},{256,4},{275,4},{297,4},{304,4},{314,4},{372,4}};
inline constexpr Relation Relations25[]={{150,154,Id::FixedStringCreate,0},{172,176,Id::FixedStringRelease,0},{186,190,Id::FixedStringRelease,0}};
inline constexpr Mask Masks26[]={{105,4},{169,4},{184,4},{196,4},{260,4},{336,4},{351,4},{369,4},{447,4},{486,4},{555,4},{615,4},{672,4},{698,4},{802,4}};
inline constexpr Relation Relations26[]={{105,109,Id::FixedStringText,0},{169,173,Id::FixedStringText,0},{336,340,Id::FixedStringText,0}};
inline constexpr Mask Masks27[]={{23,4},{59,4},{66,4},{137,4},{247,4},{254,4},{332,4},{468,4},{495,4},{505,4},{512,4},{586,4},{609,4},{633,4},{649,4},{971,4},{993,4},{1012,4},{1026,4},{1149,4},{1184,4}};
inline constexpr Relation Relations27[]={{247,251,Id::FixedStringText,0},{586,590,Id::StructRegistry,0},{993,997,Id::FixedStringCreate,0},{1012,1016,Id::FixedStringRelease,0},{1026,1030,Id::FixedStringRelease,0},{1149,1153,Id::StructRegistry,0}};
inline constexpr Mask Masks28[]={{26,4},{37,4},{59,4},{84,4},{94,4},{110,4},{132,4},{160,4},{171,4},{192,4},{214,4},{240,4},{251,4},{273,4},{295,4},{321,4},{334,4}};
inline constexpr Relation Relations28[]={{59,63,Id::FixedStringCreate,0},{94,98,Id::FixedStringRelease,0},{132,136,Id::FixedStringCreate,0},{160,164,Id::FixedStringRelease,0},{171,175,Id::FixedStringRelease,0},{214,218,Id::FixedStringCreate,0},{240,244,Id::FixedStringRelease,0},{251,255,Id::FixedStringRelease,0},{295,299,Id::FixedStringCreate,0},{321,325,Id::FixedStringRelease,0},{334,338,Id::FixedStringRelease,0}};
inline constexpr Mask Masks29[]={{7,4},{41,4},{54,4},{72,4},{89,4},{119,4},{161,4},{173,4},{210,4}};

inline constexpr Mask Masks30[]={{7,4},{27,4},{69,4},{89,4}};

inline constexpr Mask Masks31[]={{12,4},{27,4},{43,4},{74,4},{86,4},{106,4}};

inline constexpr Mask Masks32[]={{14,4},{73,4},{78,4},{90,4},{129,4},{140,4},{164,4},{184,4},{193,4},{216,4},{224,4},{235,4},{246,4}};
inline constexpr Relation Relations32[]={{140,144,Id::LuaError,0},{224,228,Id::LuaError,0},{246,250,Id::LuaError,0}};
inline constexpr Mask Masks33[]={{35,4},{51,4},{64,4},{72,4}};

inline constexpr Mask Masks34[]={{28,4},{45,4},{66,4},{83,4},{93,4},{113,4},{123,4},{144,4},{161,4},{171,4},{191,4},{201,4},{222,4},{239,4},{249,4},{269,4},{279,4},{300,4},{317,4},{327,4},{347,4},{357,4},{378,4},{395,4},{405,4},{425,4},{435,4},{456,4},{473,4},{483,4},{503,4},{513,4},{534,4},{551,4},{561,4},{581,4},{591,4},{612,4},{629,4},{639,4},{659,4},{669,4},{690,4},{707,4},{717,4},{737,4},{747,4},{768,4},{785,4},{795,4},{815,4},{825,4},{846,4},{863,4},{873,4},{893,4},{903,4},{924,4},{941,4},{951,4},{971,4},{981,4},{1002,4},{1019,4},{1029,4},{1049,4},{1059,4},{1080,4},{1097,4},{1107,4},{1134,4},{1160,4},{1169,4},{1174,4},{1180,4},{1190,4},{1197,4},{1204,4},{1209,4},{1216,4},{1223,4},{1244,4},{1261,4},{1271,4},{1291,4},{1301,4},{1322,4},{1339,4},{1349,4},{1374,4},{1383,4},{1388,4},{1394,4},{1404,4},{1411,4},{1418,4},{1423,4},{1430,4},{1437,4},{1458,4},{1475,4},{1485,4},{1505,4},{1515,4},{1536,4},{1553,4},{1563,4},{1583,4},{1593,4},{1614,4},{1631,4},{1641,4},{1661,4},{1671,4},{1692,4},{1709,4},{1719,4},{1739,4},{1749,4},{1770,4},{1787,4},{1797,4},{1822,4},{1831,4},{1836,4},{1842,4},{1852,4},{1859,4},{1866,4},{1871,4},{1878,4},{1885,4},{1906,4},{1923,4},{1933,4},{1953,4},{1963,4},{1984,4},{2001,4},{2011,4},{2031,4},{2041,4},{2062,4},{2079,4},{2089,4},{2109,4},{2119,4},{2140,4},{2157,4},{2167,4},{2187,4},{2197,4},{2218,4},{2235,4},{2245,4},{2265,4},{2275,4},{2296,4},{2313,4},{2323,4},{2343,4},{2353,4},{2374,4},{2391,4},{2401,4},{2421,4},{2431,4},{2452,4},{2469,4},{2479,4},{2499,4},{2509,4},{2530,4},{2547,4},{2557,4},{2577,4},{2587,4},{2608,4},{2625,4},{2635,4},{2655,4},{2665,4},{2686,4},{2703,4},{2713,4},{2738,4},{2747,4},{2752,4},{2758,4},{2768,4},{2775,4},{2782,4},{2787,4},{2794,4},{2801,4},{2822,4},{2839,4},{2849,4},{2869,4},{2879,4},{2900,4},{2917,4},{2927,4},{2947,4},{2957,4},{2978,4},{2995,4},{3005,4},{3025,4},{3035,4},{3056,4},{3073,4},{3083,4},{3103,4},{3113,4},{3134,4},{3151,4},{3161,4},{3181,4},{3191,4},{3212,4},{3229,4},{3239,4},{3259,4},{3267,4},{3278,4},{3299,4},{3316,4},{3326,4},{3346,4},{3354,4},{3365,4},{3386,4},{3403,4},{3413,4},{3433,4},{3441,4},{3452,4},{3473,4},{3490,4},{3500,4},{3525,4},{3534,4},{3539,4},{3545,4},{3555,4},{3562,4},{3569,4},{3574,4},{3581,4},{3586,4},{3597,4},{3618,4},{3635,4},{3645,4},{3665,4},{3673,4},{3684,4},{3705,4},{3722,4},{3732,4},{3752,4},{3760,4},{3771,4},{3792,4},{3809,4},{3819,4},{3839,4},{3847,4},{3858,4},{3879,4},{3896,4},{3906,4},{3926,4},{3934,4},{3945,4},{3966,4},{3983,4},{3993,4},{4013,4},{4021,4},{4032,4},{4053,4},{4070,4},{4080,4},{4100,4},{4108,4},{4119,4},{4140,4},{4157,4},{4167,4},{4187,4},{4195,4},{4206,4},{4227,4},{4244,4},{4254,4},{4274,4},{4282,4},{4293,4},{4314,4},{4331,4},{4341,4},{4361,4},{4369,4},{4380,4},{4401,4},{4418,4},{4428,4},{4448,4},{4456,4},{4467,4},{4488,4},{4505,4},{4515,4},{4535,4},{4545,4},{4566,4},{4583,4},{4593,4},{4613,4},{4623,4},{4644,4},{4661,4},{4671,4},{4691,4},{4699,4},{4706,4},{4727,4},{4744,4},{4754,4},{4774,4},{4784,4},{4802,4},{4823,4},{4840,4},{4850,4},{4870,4},{4880,4},{4903,4}};
inline constexpr Relation Relations34[]={{66,70,Id::FixedStringCreate,0},{93,97,Id::FixedStringRelease,0},{113,117,Id::StaticBoolType,0},{144,148,Id::FixedStringCreate,0},{171,175,Id::FixedStringRelease,0},{222,226,Id::FixedStringCreate,0},{249,253,Id::FixedStringRelease,0},{300,304,Id::FixedStringCreate,0},{327,331,Id::FixedStringRelease,0},{378,382,Id::FixedStringCreate,0},{405,409,Id::FixedStringRelease,0},{425,429,Id::StaticUInt16Type,0},{456,460,Id::FixedStringCreate,0},{483,487,Id::FixedStringRelease,0},{534,538,Id::FixedStringCreate,0},{561,565,Id::FixedStringRelease,0},{612,616,Id::FixedStringCreate,0},{639,643,Id::FixedStringRelease,0},{690,694,Id::FixedStringCreate,0},{717,721,Id::FixedStringRelease,0},{768,772,Id::FixedStringCreate,0},{795,799,Id::FixedStringRelease,0},{815,819,Id::StaticFloatType,0},{846,850,Id::FixedStringCreate,0},{873,877,Id::FixedStringRelease,0},{924,928,Id::FixedStringCreate,0},{951,955,Id::FixedStringRelease,0},{1002,1006,Id::FixedStringCreate,0},{1029,1033,Id::FixedStringRelease,0},{1080,1084,Id::FixedStringCreate,0},{1107,1111,Id::FixedStringRelease,0},{1134,1138,Id::TLSIndex,0},{1244,1248,Id::FixedStringCreate,0},{1271,1275,Id::FixedStringRelease,0},{1322,1326,Id::FixedStringCreate,0},{1349,1353,Id::FixedStringRelease,0},{1458,1462,Id::FixedStringCreate,0},{1485,1489,Id::FixedStringRelease,0},{1536,1540,Id::FixedStringCreate,0},{1563,1567,Id::FixedStringRelease,0},{1614,1618,Id::FixedStringCreate,0},{1641,1645,Id::FixedStringRelease,0},{1692,1696,Id::FixedStringCreate,0},{1719,1723,Id::FixedStringRelease,0},{1770,1774,Id::FixedStringCreate,0},{1797,1801,Id::FixedStringRelease,0},{1906,1910,Id::FixedStringCreate,0},{1933,1937,Id::FixedStringRelease,0},{1984,1988,Id::FixedStringCreate,0},{2011,2015,Id::FixedStringRelease,0},{2062,2066,Id::FixedStringCreate,0},{2089,2093,Id::FixedStringRelease,0},{2140,2144,Id::FixedStringCreate,0},{2167,2171,Id::FixedStringRelease,0},{2218,2222,Id::FixedStringCreate,0},{2245,2249,Id::FixedStringRelease,0},{2265,2269,Id::StaticEntityType,0},{2296,2300,Id::FixedStringCreate,0},{2323,2327,Id::FixedStringRelease,0},{2374,2378,Id::FixedStringCreate,0},{2401,2405,Id::FixedStringRelease,0},{2452,2456,Id::FixedStringCreate,0},{2479,2483,Id::FixedStringRelease,0},{2530,2534,Id::FixedStringCreate,0},{2557,2561,Id::FixedStringRelease,0},{2608,2612,Id::FixedStringCreate,0},{2635,2639,Id::FixedStringRelease,0},{2686,2690,Id::FixedStringCreate,0},{2713,2717,Id::FixedStringRelease,0},{2822,2826,Id::FixedStringCreate,0},{2849,2853,Id::FixedStringRelease,0},{2900,2904,Id::FixedStringCreate,0},{2927,2931,Id::FixedStringRelease,0},{2978,2982,Id::FixedStringCreate,0},{3005,3009,Id::FixedStringRelease,0},{3056,3060,Id::FixedStringCreate,0},{3083,3087,Id::FixedStringRelease,0},{3134,3138,Id::FixedStringCreate,0},{3161,3165,Id::FixedStringRelease,0},{3212,3216,Id::FixedStringCreate,0},{3239,3243,Id::FixedStringRelease,0},{3299,3303,Id::FixedStringCreate,0},{3326,3330,Id::FixedStringRelease,0},{3386,3390,Id::FixedStringCreate,0},{3413,3417,Id::FixedStringRelease,0},{3473,3477,Id::FixedStringCreate,0},{3500,3504,Id::FixedStringRelease,0},{3618,3622,Id::FixedStringCreate,0},{3645,3649,Id::FixedStringRelease,0},{3673,3677,Id::StaticFloatType,0},{3705,3709,Id::FixedStringCreate,0},{3732,3736,Id::FixedStringRelease,0},{3760,3764,Id::StaticFloatType,0},{3792,3796,Id::FixedStringCreate,0},{3819,3823,Id::FixedStringRelease,0},{3847,3851,Id::StaticFloatType,0},{3879,3883,Id::FixedStringCreate,0},{3906,3910,Id::FixedStringRelease,0},{3934,3938,Id::StaticFloatType,0},{3966,3970,Id::FixedStringCreate,0},{3993,3997,Id::FixedStringRelease,0},{4021,4025,Id::StaticFloatType,0},{4053,4057,Id::FixedStringCreate,0},{4080,4084,Id::FixedStringRelease,0},{4108,4112,Id::StaticFloatType,0},{4140,4144,Id::FixedStringCreate,0},{4167,4171,Id::FixedStringRelease,0},{4195,4199,Id::StaticFloatType,0},{4227,4231,Id::FixedStringCreate,0},{4254,4258,Id::FixedStringRelease,0},{4282,4286,Id::StaticFloatType,0},{4314,4318,Id::FixedStringCreate,0},{4341,4345,Id::FixedStringRelease,0},{4369,4373,Id::StaticFloatType,0},{4401,4405,Id::FixedStringCreate,0},{4428,4432,Id::FixedStringRelease,0},{4488,4492,Id::FixedStringCreate,0},{4515,4519,Id::FixedStringRelease,0},{4566,4570,Id::FixedStringCreate,0},{4593,4597,Id::FixedStringRelease,0},{4644,4648,Id::FixedStringCreate,0},{4671,4675,Id::FixedStringRelease,0},{4727,4731,Id::FixedStringCreate,0},{4754,4758,Id::FixedStringRelease,0},{4784,4788,Id::StaticStructs,0},{4823,4827,Id::FixedStringCreate,0},{4850,4854,Id::FixedStringRelease,0},{4880,4884,Id::StaticStructs,0}};
inline constexpr Mask Masks35[]={{30,4},{123,4}};

inline constexpr Mask Masks36[]={{36,4},{60,4}};

inline constexpr Mask Masks37[]={{122,4},{182,4},{204,4},{218,4},{266,4}};

inline constexpr Mask Masks38[]={{8,4},{78,4},{85,4},{98,4},{112,4},{119,4},{132,4}};
inline constexpr Relation Relations38[]={{8,12,Id::TLSIndex,0}};
inline constexpr Mask Masks39[]={{63,4},{148,4},{184,4}};
inline constexpr Relation Relations39[]={{63,67,Id::StructRegistry,16},{148,152,Id::StructRegistry,0},{184,188,Id::ValidateMap,0}};
inline constexpr Mask Masks40[]={{18,4}};

inline constexpr Mask Masks41[]={{95,4},{106,4},{113,4},{128,4},{152,4}};
inline constexpr Relation Relations41[]={{95,99,Id::FixedStringText,0},{106,110,Id::FixedStringText,0}};
inline constexpr Mask Masks42[]={{24,4},{34,4},{71,4},{78,4},{83,4},{90,4},{98,4},{114,4},{125,4},{139,4}};

inline constexpr Mask Masks43[]={{32,4},{53,4},{97,4},{110,4},{128,4},{133,4},{141,4},{146,4}};

inline constexpr Mask Masks44[]={{17,4},{68,4},{92,4},{105,4},{110,4},{118,4},{123,4},{134,4},{155,4}};

inline constexpr Mask Masks45[]={{28,4},{95,4},{117,4},{124,4},{129,4},{137,4},{142,4}};

inline constexpr Mask Masks46[]={{31,4},{114,4},{127,4},{152,4},{166,4},{174,4},{183,4},{231,4},{250,4},{273,4},{290,2},{410,4},{440,4},{447,4},{488,4},{499,4},{509,4},{609,4},{626,4},{694,4},{742,4},{767,4},{779,4},{787,4},{796,4},{844,4},{863,4},{886,4},{902,2},{1019,4},{1045,4},{1052,4},{1093,4},{1103,4},{1113,4},{1141,4},{1179,4},{1249,4},{1285,4},{1315,4},{1342,4},{1347,4},{1355,4},{1360,4},{1368,4},{1373,4},{1381,4},{1386,4},{1394,4},{1399,4}};
inline constexpr Relation Relations46[]={{609,613,Id::LifetimeAllocate,0},{626,630,Id::LifetimePush,0},{1141,1145,Id::ProfilerDtor,0},{1249,1253,Id::ProfilerDtor,0}};
inline constexpr Mask Masks47[]={{19,4},{42,4},{86,4},{96,4},{115,4},{134,4},{142,4},{184,4},{194,4},{213,4},{232,4},{240,4},{277,4},{290,4},{313,4},{318,4},{326,4},{331,4}};

inline constexpr Mask Masks48[]={{43,4},{77,4},{82,4},{90,4},{95,4}};

inline constexpr Mask Masks49[]={{9,4},{37,4},{48,4},{75,4},{84,4},{131,4},{141,4},{148,4},{162,4},{197,4},{202,4},{210,4},{215,4}};
inline constexpr Relation Relations49[]={{37,41,Id::ValidateObject,0},{48,52,Id::ValidateObject,0},{84,88,Id::StructRegistry,16},{131,135,Id::FixedStringText,0}};
inline constexpr Mask Masks50[]={{27,4},{38,4},{75,4},{94,4},{194,4},{205,4},{224,4},{252,4},{257,4}};
inline constexpr Relation Relations50[]={{27,31,Id::CheckStack,0},{194,198,Id::FixedStringText,0}};
inline constexpr Mask Masks51[]={{107,4},{117,4},{132,4},{146,4},{157,4},{359,4},{364,4},{372,4},{377,4},{385,4},{390,4}};
inline constexpr Relation Relations51[]={{107,111,Id::FixedStringText,0},{132,136,Id::Index,0}};
inline constexpr Mask Masks52[]={{37,4},{75,4},{83,4},{236,4},{384,4},{394,4},{402,4},{412,4},{424,4},{442,4},{450,4},{460,4},{472,4},{490,4},{498,4},{509,4},{521,4},{539,4},{547,4},{558,4},{566,4}};
inline constexpr Relation Relations52[]={{236,240,Id::StructRegistry,0},{402,406,Id::LuaError,0},{450,454,Id::LuaError,0},{498,502,Id::LuaError,0},{547,551,Id::LuaError,0},{566,570,Id::LuaError,0}};
inline constexpr Mask Masks53[]={{22,4},{75,4},{110,4},{118,4},{126,4},{208,4},{230,4},{254,4},{269,4},{294,4},{343,4},{362,4},{384,4},{391,4},{401,4},{423,4},{454,4},{474,4},{496,4},{517,4}};
inline constexpr Relation Relations53[]={{230,234,Id::FixedStringCreate,0},{254,258,Id::FixedStringRelease,0},{269,273,Id::FixedStringRelease,0}};
inline constexpr Mask Masks54[]={{198,4},{269,4},{284,4},{312,4},{357,4},{432,4},{447,4},{465,4},{543,4},{582,4},{635,4},{679,4},{720,4},{746,4},{898,4},{905,4},{910,4},{918,4},{923,4},{931,4},{936,4},{944,4},{949,4},{957,4},{962,4},{970,4},{975,4},{983,4},{988,4}};
inline constexpr Relation Relations54[]={{198,202,Id::FixedStringText,0},{269,273,Id::FixedStringText,0},{432,436,Id::FixedStringText,0}};
inline constexpr Mask Masks55[]={{23,4},{59,4},{66,4},{149,4},{279,4},{286,4},{364,4},{502,4},{529,4},{539,4},{546,4},{611,4},{717,4},{743,4},{767,4},{783,4},{1134,4},{1156,4},{1175,4},{1189,4},{1313,4},{1326,4},{1349,4},{1358,4},{1374,4},{1379,4},{1387,4},{1392,4},{1400,4},{1405,4},{1413,4},{1418,4},{1426,4},{1431,4},{1439,4},{1444,4},{1452,4},{1457,4},{1465,4},{1470,4},{1478,4},{1483,4},{1491,4},{1496,4},{1504,4},{1509,4}};
inline constexpr Relation Relations55[]={{279,283,Id::FixedStringText,0},{717,721,Id::StructRegistry,0},{1156,1160,Id::FixedStringCreate,0},{1175,1179,Id::FixedStringRelease,0},{1189,1193,Id::FixedStringRelease,0},{1313,1317,Id::StructRegistry,12},{1326,1330,Id::StructRegistry,0},{1349,1353,Id::StructRegistry,12},{1358,1362,Id::StructRegistry,0}};
inline constexpr Mask Masks56[]={{35,4},{46,4},{69,4},{88,4},{98,4},{114,4},{136,4},{164,4},{175,4},{196,4},{218,4},{244,4},{255,4},{277,4},{299,4},{325,4},{338,4},{361,4},{383,4},{402,4},{443,4},{448,4}};
inline constexpr Relation Relations56[]={{69,73,Id::FixedStringCreate,0},{98,102,Id::FixedStringRelease,0},{136,140,Id::FixedStringCreate,0},{164,168,Id::FixedStringRelease,0},{175,179,Id::FixedStringRelease,0},{218,222,Id::FixedStringCreate,0},{244,248,Id::FixedStringRelease,0},{255,259,Id::FixedStringRelease,0},{299,303,Id::FixedStringCreate,0},{325,329,Id::FixedStringRelease,0},{338,342,Id::FixedStringRelease,0},{383,387,Id::FixedStringCreate,0},{402,406,Id::FixedStringRelease,0}};
inline constexpr Mask Masks57[]={{54,4},{78,4},{327,4},{332,4},{340,4},{345,4},{353,4},{358,4}};

inline constexpr Mask Masks58[]={{8,4},{78,4},{85,4},{98,4},{112,4},{119,4},{132,4}};
inline constexpr Relation Relations58[]={{8,12,Id::TLSIndex,0}};
inline constexpr Mask Masks59[]={{62,4},{75,4},{160,4},{195,4},{200,4},{209,4}};
inline constexpr Relation Relations59[]={{75,79,Id::StructRegistry,16},{160,164,Id::StructRegistry,0},{209,213,Id::ValidateMap,0}};
inline constexpr Mask Masks60[]={{6,4},{18,4},{46,4},{51,4},{59,4},{64,4}};
inline constexpr Relation Relations60[]={{6,10,Id::StaticStructs,12},{18,22,Id::StaticStructs,0}};
inline constexpr Mask Masks61[]={{59,4}};

inline constexpr Mask Masks62[]={{65,4},{75,4},{103,4},{113,4},{139,4},{167,4},{172,4}};

inline constexpr Form Forms[]={
{Id::OnUpdate,356,"e158e5016b4b7bfb34c4d61d04b90963f52c31f696bf0de179bae13593550d0d",Masks0,std::size(Masks0),nullptr,0},
{Id::Shutdown,98,"fb912c92a370dc294272b9cbc65507dfa1249f591ce0f853dd628a3da6fc1cac",Masks1,std::size(Masks1),nullptr,0},
{Id::IncRefs,107,"2ba8c34b49a9e781bdb0cf46cd329fcaa63905ef3328580e147b0ef48c61a45a",Masks2,std::size(Masks2),nullptr,0},
{Id::DecRefs,81,"860408964786b155cedf7c8e464f8c59dd8f2014a272f78f10e6895dd23c6193",Masks3,std::size(Masks3),nullptr,0},
{Id::LoadScript,1150,"504055b1b15ebf8ca942c692e3da74502ad5050ef1f621ee626ef74ffbba1e47",Masks4,std::size(Masks4),Relations4,std::size(Relations4)},
{Id::SetTop,87,"7978a38f2bd856f8a24c30050c7ae72925ee43c8e79862612afe95ae3f896b6b",nullptr,0,nullptr,0},
{Id::ClientInitialize,292,"a8cd15a1cf97bc050560fe8ba867ccb843758660a35455f78bd75a87d77fc201",Masks6,std::size(Masks6),nullptr,0},
{Id::PushString,94,"a928dce908b76c7eaf03eb0f4a04f83e137cee51dc5399080a30e99bb698a683",Masks7,std::size(Masks7),nullptr,0},
{Id::PushClosure,220,"b12b5c193bde8070b887555ad1513464b3253ad869db848821a2b1a12abf6dfa",Masks8,std::size(Masks8),nullptr,0},
{Id::SetTable,187,"4fa58d9b4fd1e86c018497f974514278cf86df5c144740363b54214ae50d4c1d",Masks9,std::size(Masks9),Relations9,std::size(Relations9)},
{Id::CheckStack,178,"743f0a4499ef086e0e8cc37e579de365a5b93c938a6ce3fdeeed782187e5ed1b",Masks10,std::size(Masks10),nullptr,0},
{Id::Index,114,"7f4d3c3eaf23d03563f4eba1b2ab4db6253b87a27e5085a3196dd1fcfd3ca45b",Masks11,std::size(Masks11),nullptr,0},
{Id::LifetimeDtor,58,"47d3f0449ff83ed7914cc56ab11a2961d8160da4f397452d8230efa9dcaf8158",Masks12,std::size(Masks12),nullptr,0},
{Id::AddMember,127,"554ecbbe7152f7fdb29dea2bc994e012305254a90a51a44ab6cba929404664a7",Masks13,std::size(Masks13),Relations13,std::size(Relations13)},
{Id::StaticFloatType,128,"2875c2c68cd7f4b6236eeb32c1b323bcfe5c644ea85494fb72623cb5baaa75fe",Masks14,std::size(Masks14),Relations14,std::size(Relations14)},
{Id::StaticBoolType,128,"2875c2c68cd7f4b6236eeb32c1b323bcfe5c644ea85494fb72623cb5baaa75fe",Masks15,std::size(Masks15),Relations15,std::size(Relations15)},
{Id::StaticUInt16Type,128,"2875c2c68cd7f4b6236eeb32c1b323bcfe5c644ea85494fb72623cb5baaa75fe",Masks16,std::size(Masks16),Relations16,std::size(Relations16)},
{Id::StaticEntityType,128,"2875c2c68cd7f4b6236eeb32c1b323bcfe5c644ea85494fb72623cb5baaa75fe",Masks17,std::size(Masks17),Relations17,std::size(Relations17)},
{Id::Alloc,108,"c9200f746c9420010ab2c12f095f3d6d908d9c560045b7f0a4bf716af237a919",Masks18,std::size(Masks18),nullptr,0},
{Id::Free,94,"ff3fd5ead10ca24de28e3e0f3fe3620f7b1973923fe1671d2d2158b0dd73953b",Masks19,std::size(Masks19),nullptr,0},
{Id::ValidateMap,181,"a86a1b7b59751d015d0493708412e5b7323ec39529bd6a79c61a59d063e06050",Masks20,std::size(Masks20),Relations20,std::size(Relations20)},
{Id::SerializeRaw,233,"c29e3eafc201deac2ac118064a6c1e33bfaf81ff5a53ee85f4afe646600d5709",Masks21,std::size(Masks21),Relations21,std::size(Relations21)},
{Id::UnserializeRaw,299,"0e119be09836ee16e26f640b9849c96879e06fd280b69f4369badf59d541abdf",Masks22,std::size(Masks22),Relations22,std::size(Relations22)},
{Id::GetRawObject,566,"9c277acbb22c6ade7bb180daffe7911312cd16a2fff69ed2799d4a859f1ce3ca",Masks23,std::size(Masks23),Relations23,std::size(Relations23)},
{Id::PushInternalFunction,156,"fbf09e36948a38b52a65541a290e62f68aaeba6611553f696d25ca0b7ef47f48",Masks24,std::size(Masks24),nullptr,0},
{Id::DispatchEvent,392,"8fcebf63dc051e7299f0219f417a1c7ae3481b1c2e34027c6bd8c98b844702f4",Masks25,std::size(Masks25),Relations25,std::size(Relations25)},
{Id::InheritProperties,806,"2defa993a3983aa99097e76aad22d9c8248ee3b8c8aa753c2754a2662d357adb",Masks26,std::size(Masks26),Relations26,std::size(Relations26)},
{Id::ProcessDefinitions,1208,"6af1dcc922087816b25bcc84129ef4345c31c6716b3a90c863e599d261fa0358",Masks27,std::size(Masks27),Relations27,std::size(Relations27)},
{Id::RegisterClass,369,"44580dd8e7bd60564d40d42fb65cd725f6c15eb6f40c6b15df23420b91e74a83",Masks28,std::size(Masks28),Relations28,std::size(Relations28)},
{Id::FixedStringCreate,219,"e5058e3f5bb45dca97c9b2b8153a7f6a3ce5772d69ec8e60d2409e1011daa3c9",Masks29,std::size(Masks29),nullptr,0},
{Id::FixedStringText,98,"8600429b7a10d6e15a7bf1be3372e4ea27df84b6c3d35440def56da8a8a33f12",Masks30,std::size(Masks30),nullptr,0},
{Id::FixedStringRelease,115,"f2052301381d90e2d04dcce2593fae09d57fb781bc2c8185a329b8a4e6767ddb",Masks31,std::size(Masks31),nullptr,0},
{Id::ArgError,251,"b63f358704f3ab8b8ca06432ac80f89392e526548832b40dc1ece8521f74581b",Masks32,std::size(Masks32),Relations32,std::size(Relations32)},
{Id::LuaError,77,"d282861e64677d48c402fabcbe9cfd9fbcb6ce3a5e3921af381e4fe0111fbed1",Masks33,std::size(Masks33),nullptr,0},
{Id::TypeInitialize,4933,"0f50e9fe555b41f2e29fd9e7a26aaafd1ebe41b308b07bc0e3ee1eec78e9504e",Masks34,std::size(Masks34),Relations34,std::size(Relations34)},
{Id::ResizeStructs,186,"158ae77983e73bdc585fbb8cd7e4ce56788aa4dd5c9d1dd07e0e050fe686482c",Masks35,std::size(Masks35),nullptr,0},
{Id::LifetimeAllocate,308,"f4e0791496fdc86c088253a42b44e2b5cbc22def92b0d844dae4976740353d59",Masks36,std::size(Masks36),nullptr,0},
{Id::LifetimePush,271,"2c9d2d937d8b26ae69c6a84b442e19b2c73cad6b14d70b549fc47661063550cf",Masks37,std::size(Masks37),nullptr,0},
{Id::ContextAccessor,158,"dcf9b486716fa31f7896892df459f661aa38d09cea4080d3224fd78897a06972",Masks38,std::size(Masks38),Relations38,std::size(Relations38)},
{Id::InputAccessor,190,"00fdf33bdd4e48b238d7137efc8ead64ad5a92a5d10ff15bae90cceb1d86246f",Masks39,std::size(Masks39),Relations39,std::size(Relations39)},
{Id::CameraConstruct,59,"ca3b48300d2e42edfc681215efa6e4cef8031250a70259330560beb51305d870",Masks40,std::size(Masks40),nullptr,0},
{Id::ValidateObject,171,"c6e55f29f7c5f619edb0753c0e0ebd13654dff89af58aad1d70fcddab4c7830c",Masks41,std::size(Masks41),Relations41,std::size(Relations41)},
{Id::OnUpdate,159,"2eedb08d3da0ec19a59f688969b2082ad29cb084214f006e5db121fb0b37d56c",Masks42,std::size(Masks42),nullptr,0},
{Id::Shutdown,151,"139ef3d9faeb76d32734155db740919beed827c31ad9f02471cce915e1b600e4",Masks43,std::size(Masks43),nullptr,0},
{Id::IncRefs,160,"6054aa773ffa3d08b5ad11256d582eaf252c155e235cad201fd283de4ca4ef55",Masks44,std::size(Masks44),nullptr,0},
{Id::DecRefs,147,"9b2dc8ab19ce37c63f93cd82e83936308ab91a43e95ba04db393a7958e32cbfd",Masks45,std::size(Masks45),nullptr,0},
{Id::LoadScript,1404,"33273e2a043a368eda9bedc7b79d3e4e6e2c28b008a6ec404924789958bfdff2",Masks46,std::size(Masks46),Relations46,std::size(Relations46)},
{Id::ClientInitialize,336,"db0afd13421fc79e98a5080f70650b4747ec7a3735cb8d462c91e21f6904daba",Masks47,std::size(Masks47),nullptr,0},
{Id::LifetimeDtor,100,"b499e6e0cb67985e48db06b8ab87b0d9855bc9673ff674ebdb01b91a96efa5cc",Masks48,std::size(Masks48),nullptr,0},
{Id::ValidateMap,220,"69c8f3d84ff3f4f44d65b0ff2379aa08a5beb5feeba760e81985a3b2f9dbc092",Masks49,std::size(Masks49),Relations49,std::size(Relations49)},
{Id::SerializeRaw,262,"5357613cf79f99089c20ccb74cae215bda6210db8250e0f8f073e74343dfbd90",Masks50,std::size(Masks50),Relations50,std::size(Relations50)},
{Id::UnserializeRaw,395,"e7a254a3a90fb506e17013e1627d9b6634b609339681e221d2bf30afa359bb90",Masks51,std::size(Masks51),Relations51,std::size(Relations51)},
{Id::GetRawObject,571,"ceeac8cb47b858b2260c62e92854e9a81cc5b5e4c9d828b920d483effabcbbe0",Masks52,std::size(Masks52),Relations52,std::size(Relations52)},
{Id::DispatchEvent,540,"d7fc99bd50ea330fd603737ec84e835f119937560f83f8c5fbc66cf0add7755e",Masks53,std::size(Masks53),Relations53,std::size(Relations53)},
{Id::InheritProperties,993,"0cd57903cb0345862d524be4994b57ff47fce6c8077ddb657284ac9e3393f696",Masks54,std::size(Masks54),Relations54,std::size(Relations54)},
{Id::ProcessDefinitions,1514,"fda5a4bbb2082c0d9b82f4902bd1769fe809b028bd90d003f08b4b079db8dadc",Masks55,std::size(Masks55),Relations55,std::size(Relations55)},
{Id::RegisterClass,453,"e927bacf4c7c5ca9483771420d890ba2818048afa1643658764e18296e41d019",Masks56,std::size(Masks56),Relations56,std::size(Relations56)},
{Id::LifetimeAllocate,363,"6d4ef4a64a340dda7f870dbbcefecd0f1334751c3aff9af15da75ba52dceadf3",Masks57,std::size(Masks57),nullptr,0},
{Id::ContextAccessor,158,"53afd335f96ef98e5353831bcf2b63bbd03ad6c250ef69bfac78a1d071f4635c",Masks58,std::size(Masks58),Relations58,std::size(Relations58)},
{Id::InputAccessor,215,"df1c51479396552029a3e1d2ca00606116fbfba6b3ec1d36a3637458a481a83d",Masks59,std::size(Masks59),Relations59,std::size(Relations59)},
{Id::FinalizeClass,69,"01717c24e5a2fd34cc0bd1268413ad8bada895537da1dc2200bd7715b9a0b63b",Masks60,std::size(Masks60),Relations60,std::size(Relations60)},
{Id::ProfilerCtor,100,"b6677db63aa3c74e30bc7a6e2203f23dbe72fe715c567e00d89c2bcf327b39e6",Masks61,std::size(Masks61),nullptr,0},
{Id::ProfilerDtor,177,"9b1b299131ed2437af4e03dfa31e4945c5090d9f9945fa58b47b08823d707c5b",Masks62,std::size(Masks62),nullptr,0},
};
}