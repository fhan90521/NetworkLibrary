#pragma once
#include <string>
#include <random>
class Encrptor
{
private:
	std::random_device _rd;
	std::mt19937 _rnGenerator;
	std::uniform_int_distribution<uint64_t> _dis;
public:
	Encrptor();
	std::string MakeRandomString();
	static std::string HashWithSalt(const std::string& str, const std::string& salt);
};