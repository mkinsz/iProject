#pragma once

#ifdef _WIN32
#pragma pack( push, 2 )
#else
#pragma pack(1)
#endif

struct test
{
	char x1;
	float x3;
	short x2;
	char x4;
};

struct test1
{
	char x1;
	char x4;
	short x2;
	float x3;
};

#ifdef _WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif