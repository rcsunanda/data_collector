//g++ -std=c++11 exception_throw.cpp -o exception_throw

#include <string>

int main()
{
	std::stoi("abcd");	//exception thrown here
	return 0;
}
