// © 2021 NVIDIA Corporation

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

/*
Versioning rules:
- give version to someone - increment version before and after for tracking
- BUILD - preserves binary compatibility
- MINOR - settings & descs change
- MAJOR - major feature update, API change
*/

#define VERSION_MAJOR                   1
#define VERSION_MINOR                   123
#define VERSION_BUILD                   0
#define VERSION_REVISION                0

#define VERSION_STRING STR(VERSION_MAJOR.VERSION_MINOR.VERSION_BUILD.VERSION_REVISION)
