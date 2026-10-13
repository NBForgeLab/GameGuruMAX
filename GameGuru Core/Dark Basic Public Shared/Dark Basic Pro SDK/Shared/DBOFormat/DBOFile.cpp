
//
// DBOFile Functions Implementation
//

//////////////////////////////////////////////////////////////////////////////////
// DBOFILE HEADER ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
#include "DBOFile.h"
#include "DBOBlock.h"
#include "direct.h"

DARKSDK_DLL int LoadDBOEx ( LPSTR pFilename, sObject** ppObject )
{
	DWORD dwBlockSize = 0;
	void* pDBOBlock = NULL; // was DWORD*

	if ( !DBOLoadBlockFile ( pFilename, (void**)&pDBOBlock, &dwBlockSize ) )
		return -1;

	const bool success = DBOConvertBlockToObject(pDBOBlock, dwBlockSize, ppObject);
	delete[] static_cast<char*>(pDBOBlock);
	return success ? 1 : -2;
}

DARKSDK_DLL int SaveDBOEx ( LPSTR pFilename, sObject* pObject )
{
	// DBOBlock ptr
	DWORD dwBlockSize = 0;
	void* pDBOBlock = NULL;

	// convert pObject to DBOBlock
	if ( !DBOConvertObjectToBlock ( pObject, (void**)&pDBOBlock, &dwBlockSize ) )
		return -2;
		
	// save DBOBlock to file
	const bool saved = DBOSaveBlockFile(pFilename, pDBOBlock, dwBlockSize);
	delete[] static_cast<char*>(pDBOBlock);
	return saved ? 1 : -3;
}
