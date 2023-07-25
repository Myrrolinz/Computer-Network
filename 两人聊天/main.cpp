#include"client.h"
#include"server.h"
#include <iostream>

int main() {
	cout << "Please select which you want to be:\n0. Server\t1. Client\n";
	int choose_server_or_client;
	cin >> choose_server_or_client;
	cin.get();
	if (choose_server_or_client == 0)
		serverMain();
	else if (choose_server_or_client == 1)
		clientMain();
	else
		cout << "Error! You must be Server or Client!";
	return 0;
}

