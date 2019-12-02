#include "ringbuffer.h"

#include <iostream>
#include <future>
#include <thread>
#include <chrono>
#include <string>
#include <random>

#include "ThreadPool.h"

void func_future() {
	std::packaged_task<int()> task([]() { return 7; }); //wrapped function
	std::future<int> f1 = task.get_future();
	std::thread(std::move(task)).detach();

	//
	std::future<int> f2 = std::async(std::launch::async, []() { return 8; });

	//
	std::promise<int> p;
	std::future<int> f3 = p.get_future();
	std::thread([&p] { p.set_value_at_thread_exit(9); }).detach();

	//
	std::cout << "Waiting..." << std::flush;
	f1.wait(); std::cout << "Waiting 1" << std::flush;
	f2.wait(); std::cout << "Waiting 2" << std::flush;
	f3.wait(); std::cout << "Waiting 3" << std::flush;

	std::cout << "Done! \nResults are: "
		<< f1.get() << " " << f2.get() << " " << f3.get() << '\n';

	auto start = std::chrono::high_resolution_clock::now();
	std::future<std::string> f4 = std::async(std::launch::async, []() { return std::string("ready200"); });
	f4.wait();
	auto end = std::chrono::high_resolution_clock::now();
	
	std::cout << f4.get() << '\n';

	std::chrono::duration<double, std::milli> elapsed = end - start;
	std::cout << "Waited " << " " << elapsed.count() << " ms\n";
}

struct A { double x; };
const A* a;

decltype(a->x) y;
decltype((a->x)) z = y;

template<typename T, typename U>
auto add(T t, U u) -> decltype(t + u) { return t + u; }

void func_decltype()
{
	int i = 33;
	decltype(i) j = i * 2;

	std::cout << "i= " << i << ", "
		<< "j= " << j << '\n';

	auto f = [](int a, int b) -> int {
		return a * b;
	};

	decltype(f) g = f;
	i = f(2, 2);
	j = g(3, 3);

	std::cout << "i = " << i << ", "
			  << "j = " << j << '\n';
}

std::random_device g_rd;
std::mt19937 g_mt(g_rd());
std::uniform_int_distribution<int> g_dist(-1000, 1000);
auto rnd = std::bind(g_dist, g_mt);

void simulate_hard_computation() {
	std::this_thread::sleep_for(std::chrono::milliseconds(2000 + rnd()));
}

//simple function that adds multiplies two numbers and prints the result
void multiply(const int a, const int b) {
	simulate_hard_computation();
	const int res = a * b;
	std::cout << a << " * " << b << " = " << res << std::endl;
}

//
void multiply_output(int& out, const int a, const int b) {
	simulate_hard_computation();
	out = a * b;
	std::cout << a << " * " << b << " = " << out << std::endl;
}

//
int multiply_return(const int a, const int b) {
	simulate_hard_computation();
	const int res = a * b;
	std::cout << a << " * " << b << " = " << res << std::endl;
	return res;
}

void poolExample() {
	ThreadPool pool(3);
	pool.init();

	for (int i = 1; i < 3; ++i) {
		for (int j = 1; j < 10; ++j) {
			pool.submit(multiply, i, j);
		}
	}

	int output_ref;
	auto future1 = pool.submit(multiply_output, std::ref(output_ref), 5, 6);

	future1.get();
	std::cout << "Last operation result is equals to " << output_ref << std::endl;

	auto future2 = pool.submit(multiply_return, 5, 3);

	int res = future2.get();
	std::cout << "Last operation result is equals to " << res << std::endl;

	pool.shutdown();
}

#include "define.h"

struct test2 {
	char x1;
	short x2;
	float x3;
	char x4;
};

struct test3 {
	int x1;
	char x2;
};

struct test4 {
	char x2;
	int x1;
};

int main(int argc, char* argv[])
{
	//func_future();
	//func_decltype();

	std::cout << sizeof(test) << " " << sizeof(test1) << " " << sizeof(test2) << " " << sizeof(test3) << " " << sizeof(test4) << std::endl;

	//poolExample();

	int a = 2;
	int b = 3;

	switch (a) {
	case 0: {

	}break;
	case 2: {
		if (b == 3) {
			std::cout << "break inside..." << std::endl;
			break;
		}

		std::cout << "break outside..." << std::endl;
	} break;

	default:;
	}


	return EXIT_SUCCESS;
}