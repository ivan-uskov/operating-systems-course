#include "MeasureTime.h"
#include <algorithm>
#include <functional>
#include <iterator>
#include <random>
#include <thread>
#include <vector>

std::vector<int> GenerateArray(size_t size)
{
	std::vector<int> numbers;
	numbers.reserve(size);

	std::mt19937 gen;
	std::uniform_int_distribution<int> dist(1000);

	std::generate_n(std::back_inserter(numbers), size, std::bind_front(dist, gen));

	return numbers;
}



using namespace std::literals;

int main()
{
	auto numbers = MeasureTime("numbers generation", GenerateArray, 1'000'000);

	MeasureTime("sleep", [] { std::this_thread::sleep_for(1s); });
}
