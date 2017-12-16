//g++ segfault_test.cpp -o segfault_test
 
int main()
{
	int arr[5];
	arr[100000] = 33;	//Expected segfault

	return 0;
}
