#include "curl/curl.h"
#include <string>
#include <iostream>

static size_t write_data(char* szContents, size_t nSize, size_t nMemb, std::string *szStream)
{
	if (szStream == NULL) return 0;
	szStream->append(szContents, nSize * nMemb);
	return nSize * nMemb;
}


int main(int argc, char* argv[]) 
{
	std::string content;

	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = nullptr;
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:6060/test/");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
		CURLcode res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

	std::cout << content;
	std::cout << "\n\nPress any key to exit!";
	std::cin.get();

	return EXIT_SUCCESS;
}