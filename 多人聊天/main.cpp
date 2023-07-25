#include"client.h"
#include"server.h"
#include <iostream>

int main() {
	cout << "Please select which you want to be:\n0. Server\t1. Client\n";
	int choose_identity;
	cin >> choose_identity;
	cin.get();
	if (choose_identity == 0)
		serverMain();
	else
		clientMain();
	return 0;
}

