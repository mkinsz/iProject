#include <string>
#include <sys/stat.h>

#include "utils.h"
#include "cedid.h"

size_t GetFileSize(const std::string& file) {
	struct _stat info;
	_stat(file.c_str(), &info);
	size_t size = info.st_size;
	return size; //µ¥Î»ÊÇ£ºbyte
}

int main(int argc, char* argv[])
{
	//size_t size = GetFileSize("E:/Work/4K.bin");
	//OutputLog("Size: %d \n", size);

	//"E:/Work/4K.bin"

	CEdid ced("E:/Work/DEL.bin");
	ced.parse_edid();

	system("pause");

	return EXIT_SUCCESS;
}