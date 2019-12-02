#include <iostream>

#include "server.h"

int main(int argc, char* argv[])
{
	uv_tcp_t server;
	uv_tcp_init(uv_default_loop(), &server);

	sockaddr_in bind_addr;
	uv_ip4_addr("0.0.0.0", 7000, &bind_addr);
	uv_tcp_bind(&server, (const struct sockaddr*) & bind_addr, 0);

	int ret = uv_listen((uv_stream_t*)& server, backlog, on_new_connection);
	if (ret) {
		std::cerr << "error uv_listen" << std::endl;
		return 1;
	}

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}