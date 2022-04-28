#pragma once
namespace Utils
{
	bool ShouldWriteFile(string Path);
	string GetTimestamp();
	string GetDate();
};


#define ASSERT_SIZE(Type, Size) static_assert(sizeof(Type) == Size, "Invalid type size for " #Type)