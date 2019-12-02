#pragma once
#include <iostream>
#include <string>
#include <uv.h>

const size_t backlog = 128;
const size_t buffer_size = 1024;

uv_fs_t open_req;
uv_fs_t read_req;
uv_tcp_t* client;

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void on_client_read(uv_stream_t* _client, ssize_t nread, const uv_buf_t* buf);
void on_file_open(uv_fs_t* req);
void on_file_read(uv_fs_t* req);

void on_new_connection(uv_stream_t* server, int status)
{
	if (status == -1) {
		std::cerr << "error on_new_connection" << std::endl;
		uv_close((uv_handle_t*)client, NULL);
		return;
	}

	client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(uv_default_loop(), client);

	int result = uv_accept(server, (uv_stream_t*)client);

	if (result == 0) 
		uv_read_start((uv_stream_t*)client, alloc_buffer, on_client_read);
	else
		uv_close((uv_handle_t*)client, NULL);
}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	buf->base = (char*)malloc(suggested_size);
	buf->len = suggested_size;
}

void on_client_read(uv_stream_t* _client, ssize_t nread, const uv_buf_t* buf)
{
	if (nread == -1) {
		std::cerr << "error on_client_read" << std::endl;
		uv_close((uv_handle_t*)_client, NULL);
		return;
	}

	char* filename = buf->base;
	int mode = 0;

	uv_fs_open(uv_default_loop(), &open_req, filename, O_RDONLY, mode, on_file_open);
}

void on_client_write(uv_write_t* req, int status)
{
	if (status == -1) {
		std::cerr << "error on_client_write" << std::endl;
		uv_close((uv_handle_t*)client, NULL);
		return;
	}

	char* buffer = (char*)req->data;
	free(buffer);
	free(req);

	uv_close((uv_handle_t*)client, NULL);
}

void on_file_open(uv_fs_t* req)
{
	if (req->result == -1) {
		std::cerr << "error on_file_read" << std::endl;
		uv_close((uv_handle_t*)client, NULL);
		return;
	}

	uv_buf_t buffer = uv_buf_init((char*)malloc(sizeof(char) * buffer_size), buffer_size);

	int offset = -1;
	read_req.data = (void*)buffer.base;
	uv_fs_read(uv_default_loop(), &read_req, req->result, &buffer, sizeof(char) * buffer_size, offset, on_file_read);
	uv_fs_req_cleanup(req);
}

void on_file_read(uv_fs_t* req)
{
	if (req->result < 0) {
		std::cerr << "error on_file_open" << std::endl;
		uv_close((uv_handle_t*)client, NULL);
	}
	else if (req->result == 0) {
		uv_fs_t close_req;
		uv_fs_close(uv_default_loop(), &close_req, open_req.result, NULL);
		uv_close((uv_handle_t*)client, NULL);
	}
	else {
		uv_write_t* write_req = (uv_write_t*)malloc(sizeof(uv_write_t));

		char* message = (char*)req->data;

		uv_buf_t buf = uv_buf_init(message, sizeof(message));
		buf.len = req->result;
		buf.base = message;

		int buf_count = 1;
	
		write_req->data = (void*)message;

		uv_write(write_req, (uv_stream_t*)client, &buf, buf_count, on_client_write);
	}

	uv_fs_req_cleanup(req);
}