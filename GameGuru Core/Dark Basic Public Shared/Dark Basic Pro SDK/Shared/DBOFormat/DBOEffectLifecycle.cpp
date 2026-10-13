#include "DBOFormat.h"
cSpecialEffect::~cSpecialEffect ( )
{
	if ( m_dwRTTexCount>0 )
	{
		for ( DWORD t=0; t<m_dwRTTexCount; t++ )
		{
			SAFE_RELEASE ( m_pRTTex[t] );
			SAFE_RELEASE ( m_pRTTexView[t] );
		}
	}

	// free default zfile mesh
	SAFE_DELETE ( m_pXFileMesh );

	// free effect
	SAFE_RELEASE ( m_pEffect );
	SAFE_DELETE ( m_pDefaultXFile );
}

