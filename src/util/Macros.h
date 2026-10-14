#pragma once

#include <rhatt/Macros.h>

#ifndef CW_TESTING
#ifdef CW_WEB_BASEURL
#undef CW_WEB_BASEURL
#endif

#define CW_WEB_BASEURL "https://ferry.cheeseworks.gay"
#endif