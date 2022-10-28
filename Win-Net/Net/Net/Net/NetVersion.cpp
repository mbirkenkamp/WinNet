#include <Net/Net/NetVersion.h>

short Net::Version::Major()
{
	return NetMajorVersion;
}

short Net::Version::Minor()
{
	return NetMinorVersion;
}

short Net::Version::Revision()
{
	return NetRevision;
}

Net::String Net::Version::Key()
{
	/*
	* must be const char* 
	* otherwise it moves the pointer and tries to free it on destruction
	*/
	return Net::String(reinterpret_cast<const char*>(NetKey));
}

Net::String Net::Version::Version()
{
	/*
	* must be const char*
	* otherwise it moves the pointer and tries to free it on destruction
	*/
	return Net::String(reinterpret_cast<const char*>(CSTRING("%i.%i.%i-%s")), Major(), Minor(), Revision(), Key().data().data());
}