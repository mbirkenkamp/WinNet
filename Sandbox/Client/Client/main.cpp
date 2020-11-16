#include "config.h"
#include "Client/Client.h"
#include <Net/assets/web/http.h>

#pragma comment(lib, "NetClient.lib")

int main()
{
	// test http parsing
	Net::Web::HTTPS https("https://google.com", Net::ssl::NET_SSL_METHOD_TLSv1_2_CLIENT);
	if(https.Get())
	{
		
		LOG_ERROR("%s", https.GetHeaderContent().data());
		LOG("%s", https.GetBodyContent().data());
	}
	else
		LOG_ERROR("%s", https.GetRawData().data());
	
	system("pause");

	Client client;
	client.SetCryptPackage(true);
	if (!client.Connect(SANDBOX_SERVERIP, SANBOX_PORT))
		MessageBoxA(nullptr, CSTRING("Connection timeout"), CSTRING("Network failure"), MB_OK | MB_ICONERROR);
	else
	{
		while(client.IsConnected())
		{
			Sleep(1000);
		}
	}

	return 0;
}