#include <iostream>
#include <random>
#include <map>
#include <thread>
#include <mutex>
#include <vector>
#include <concepts>
#include <time.h>
#include <chrono>
#include <functional>
#include <fstream>
#include <iomanip>

std::mutex mtx;
std::random_device rd;
std::mt19937 mt(rd());
std::uniform_int_distribution<int> dist(0, 99);

// rand, modulus 연산
template <typename K, typename T> requires std::integral<T>&& std::integral<K>
void fn1(const int start, const int end, std::map<K, T>& mp) {
	std::map<K, T> st;
	for (int i = start; i < end; i++) {
		const K randNum = rand() % 100;

		st[randNum] += 1;
	}

	std::lock_guard<std::mutex> lock(mtx);
	for (const auto& s : st) {
		mp[s.first] += s.second;
	}
}

// random ���� ���
template <typename K, typename T> requires std::integral<T>&& std::integral<K>
void fn2(const int start, const int end, std::map<K, T>& mp) {
	std::map<K, T> st;
	for (int i = start; i < end; i++) {
		const K randNum = dist(mt);
		st[randNum] += 1;
	}

	std::lock_guard<std::mutex> lock(mtx);
	for (const auto& s : st) {
		mp[s.first] += s.second;
	}
}

// K -> key, T -> value
template <typename K, typename T> requires std::integral<T>&& std::integral<K>
double getStdev(const T last_num, std::function<void(int, int, std::map<K, T>&)> fn, const int thread_num = 8) {
	std::map<K, T> mp;
	std::vector<std::jthread> v;
	const auto step = last_num / thread_num;

	v.reserve(thread_num);

	for (int i = 1; i <= thread_num; i++) {
		v.emplace_back(fn, 0, static_cast<int>(step), std::ref(mp));
	}

	for (auto& j : v) {
		j.join();
	}

	T sum = 0;
	for (const auto& m : mp) {
		sum += m.second;
	}

	// get the standard deviation
	double mean = sum / mp.size();
	double accum = 0.0;
	for (const auto& m : mp) {
		accum += (m.second - mean) * (m.second - mean);
	}
	double stdev = sqrt(accum / (mp.size() - 1));

	return stdev;
}

auto printOutResult(const std::string& trial, const std::string& option, const double stdev, const double time, std::ostream& ofs) {
	ofs << std::left << std::setw(10) << trial << " : " << std::setw(15) << option << "�� ǥ������ = " << std::setw(17) << stdev << " | �ҿ�ð� = " << time << "��" << std::endl;
}


int main(void)
{
	srand(time(NULL));

	constexpr auto trial = 3;
	std::string fileName = "result.txt";
	auto thread_num = std::thread::hardware_concurrency() * 2 + 1;
	std::cout << "thread number = " << thread_num << std::endl;
	std::ofstream ofs(fileName, std::ios::out | std::ios::out);
	ofs << "thread number = " << thread_num << std::endl;
	ofs.close();
	for (auto last_num = 100; last_num <= 1'000'000'000; last_num *= 10) {
		std::cout << "num = " << last_num << std::endl;
		ofs.open(fileName, std::ios::app | std::ios::out);
		ofs << "num = " << last_num << std::endl;
		ofs.close();

		auto sum_stdev_modulus = 0.0;
		auto sum_stdev_randomEngine = 0.0;
		auto sum_time_modulus = 0.0;
		auto sum_time_randomEngine = 0.0;
		for (int i = 0; i < trial; i++) {
			const auto start1 = std::chrono::system_clock::now();
			auto stdev_modulus = getStdev<int, decltype(last_num)>(last_num, fn1<int, decltype(last_num)>, thread_num);
			const auto end1 = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds1 = end1 - start1;
			sum_time_modulus += elapsed_seconds1.count();
			sum_stdev_modulus += stdev_modulus;
			std::string trial1 = "trial " + std::to_string(i);
			printOutResult(trial1, "modulus", stdev_modulus, elapsed_seconds1.count(), std::cout);

			const auto start2 = std::chrono::system_clock::now();
			auto stdev_randomEngine = getStdev<int, decltype(last_num)>(last_num, fn2<int, decltype(last_num)>, thread_num);
			const auto end2 = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds2 = end2 - start2;
			sum_time_randomEngine += elapsed_seconds2.count();
			sum_stdev_randomEngine += stdev_randomEngine;
			std::string trial2 = "trial " + std::to_string(i);
			printOutResult(trial2, "randomEngine", stdev_randomEngine, elapsed_seconds2.count(), std::cout);

			ofs.open(fileName, std::ios::app | std::ios::out);
			printOutResult(trial1, "modulus", stdev_modulus, elapsed_seconds1.count(), ofs);
			printOutResult(trial2, "randomEngine", stdev_randomEngine, elapsed_seconds2.count(), ofs);

			ofs.close();
		}
		const auto avg_stdev_modulus = sum_stdev_modulus / trial;
		const auto avg_stdev_randomEngine = sum_stdev_randomEngine / trial;
		const auto avg_time_modulus = sum_time_modulus / trial;
		const auto avg_time_randomEngine = sum_time_randomEngine / trial;
		printOutResult("���", "modulus", avg_stdev_modulus, avg_time_modulus, std::cout);
		printOutResult("���", "randomEngine", avg_stdev_randomEngine, avg_time_randomEngine, std::cout);

		ofs.open(fileName, std::ios::app | std::ios::out);
		printOutResult("���", "modulus", avg_stdev_modulus, avg_time_modulus, ofs);
		printOutResult("���", "randomEngine", avg_stdev_randomEngine, avg_time_randomEngine, ofs);
		ofs.close();
	}

	auto big_num = 10'000'000'000;
	for (int i = 0; i < 2; i++) {
		std::cout << "num = " << big_num << std::endl;
		std::ofstream ofs(fileName, std::ios::app | std::ios::out);
		ofs << "num = " << big_num << std::endl;
		ofs.close();

		const auto start1 = std::chrono::system_clock::now();
		auto stdev_modulus = getStdev<int, decltype(big_num)>(big_num, fn1<int, decltype(big_num)>, thread_num);
		const auto end1 = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds1 = end1 - start1;
		printOutResult("trial 0", "modulus", stdev_modulus, elapsed_seconds1.count(), std::cout);

		const auto start2 = std::chrono::system_clock::now();
		auto stdev_randomEngine = getStdev<int, decltype(big_num)>(big_num, fn2<int, decltype(big_num)>, thread_num);
		const auto end2 = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds2 = end2 - start2;
		printOutResult("trial 0", "randomEngine", stdev_randomEngine, elapsed_seconds2.count(), std::cout);

		std::ofstream ofs2(fileName, std::ios::app | std::ios::out);
		printOutResult("trial 0", "modulus", stdev_modulus, elapsed_seconds1.count(), ofs2);
		printOutResult("trial 0", "randomEngine", stdev_randomEngine, elapsed_seconds2.count(), ofs2);
		ofs2.close();

		big_num *= 10;
	}

	return 0;
}