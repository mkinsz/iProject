#include <iostream>
#include <vector>

#include "glog/logging.h"


int main(int argc, char* argv[])
{
	//initialize google logging
	google::InitGoogleLogging(argv[0]);
	google::SetLogDestination(google::GLOG_INFO, ".//");
	google::SetStderrLogging(google::GLOG_INFO);
	google::SetLogFilenameExtension("log_");
	//google::InstallFailureSignalHandler();
	//google::InstallFailureWriter(&FatalMessageDump);

	FLAGS_colorlogtostderr = true;
	FLAGS_logbufsecs = 0;
	FLAGS_max_log_size = 1024;
	FLAGS_stop_logging_if_full_disk = true;

	LOG(INFO) << "This is an info  message";
	LOG(WARNING) << "This is a warning message";
	LOG(ERROR) << "This is an error message";
	//LOG(FATAL) << "This is a fatal message";

	system("pause");

	return EXIT_SUCCESS;
}