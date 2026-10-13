//----------------------------------------------------
//--- GAMEGURU - M-MapFile
//----------------------------------------------------
#pragma once

#include "cstr.h"
#include <string>
#include <string_view>

void mapfile_emptyebesfromtestmapfolder ( bool bIgnoreValidTextureFiles );
void mapfile_emptyterrainfilesfromtestmapfolder(void);
void mapfile_saveproject_fpm ( void );
void mapfile_loadproject_fpm ( void );
void mapfile_newmap ( void );
void mapfile_loadmap ( void );
void mapfile_savemap ( void );
void mapfile_loadplayerconfig ( void );
void mapfile_saveplayerconfig ( void );
void mapfile_savestandalone_start ( void );
int mapfile_savestandalone_continue ( void );
float mapfile_savestandalone_getprogress ( void );
void mapfile_savestandalone_finish ( void );
void mapfile_savestandalone_restoreandclose ( void );

void scanscriptfileandaddtocollection ( const char* tfile_s , const char *pPath = NULL);
inline void scanscriptfileandaddtocollection ( char* tfile_s , char *pPath = NULL) { scanscriptfileandaddtocollection((const char*)tfile_s, (const char*)pPath); }
inline void scanscriptfileandaddtocollection ( const std::string& tfile_s , const char *pPath = NULL) { scanscriptfileandaddtocollection(tfile_s.c_str(), pPath); }
inline void scanscriptfileandaddtocollection ( const cstr& tfile_s , const char *pPath = NULL) { scanscriptfileandaddtocollection(tfile_s.Get(), pPath); }

bool addtocollection ( const char* file_s );
inline bool addtocollection ( char* file_s ) { return addtocollection((const char*)file_s); }
inline bool addtocollection ( const std::string& file_s ) { return addtocollection(file_s.c_str()); }
inline bool addtocollection ( const cstr& file_s ) { return addtocollection(file_s.Get()); }

void removefromcollection ( const char* file_s );
inline void removefromcollection ( char* file_s ) { removefromcollection((const char*)file_s); }
inline void removefromcollection ( const std::string& file_s ) { removefromcollection(file_s.c_str()); }
inline void removefromcollection ( const cstr& file_s ) { removefromcollection(file_s.Get()); }

void removeanymatchingfromcollection ( const char* folderorfile_s );
inline void removeanymatchingfromcollection ( char* folderorfile_s ) { removeanymatchingfromcollection((const char*)folderorfile_s); }
inline void removeanymatchingfromcollection ( const std::string& folderorfile_s ) { removeanymatchingfromcollection(folderorfile_s.c_str()); }
inline void removeanymatchingfromcollection ( const cstr& folderorfile_s ) { removeanymatchingfromcollection(folderorfile_s.Get()); }

void addfoldertocollection ( const char* path_s );
inline void addfoldertocollection ( char* path_s ) { addfoldertocollection((const char*)path_s); }
inline void addfoldertocollection ( const std::string& path_s ) { addfoldertocollection(path_s.c_str()); }
inline void addfoldertocollection ( const cstr& path_s ) { addfoldertocollection(path_s.Get()); }

void addallinfoldertocollection ( const char* subThisFolder_s, const char* subFolder_s );
inline void addallinfoldertocollection ( const std::string& subThisFolder_s, const std::string& subFolder_s ) { addallinfoldertocollection(subThisFolder_s.c_str(), subFolder_s.c_str()); }
inline void addallinfoldertocollection ( const cstr& subThisFolder_s, const cstr& subFolder_s ) { addallinfoldertocollection(subThisFolder_s.Get(), subFolder_s.Get()); }

void createallfoldersincollection ( void );

void findalltexturesinmodelfile ( const char* file_s, const char* folder_s, const char* texpath_s );
inline void findalltexturesinmodelfile ( char* file_s, char* folder_s, char* texpath_s ) { findalltexturesinmodelfile((const char*)file_s, (const char*)folder_s, (const char*)texpath_s); }
inline void findalltexturesinmodelfile ( const std::string& file_s, const std::string& folder_s, const std::string& texpath_s ) { findalltexturesinmodelfile(file_s.c_str(), folder_s.c_str(), texpath_s.c_str()); }
inline void findalltexturesinmodelfile ( const cstr& file_s, const cstr& folder_s, const cstr& texpath_s ) { findalltexturesinmodelfile(file_s.Get(), folder_s.Get(), texpath_s.Get()); }

void CreateItineraryFile ( void );

void scanallfolder ( const char* subThisFolder_s, const char* subFolder_s );
inline void scanallfolder ( const std::string& subThisFolder_s, const std::string& subFolder_s ) { scanallfolder(subThisFolder_s.c_str(), subFolder_s.c_str()); }
inline void scanallfolder ( const cstr& subThisFolder_s, const cstr& subFolder_s ) { scanallfolder(subThisFolder_s.Get(), subFolder_s.Get()); }

bool IsFileAStockAsset ( const char* pCheckThisFile );
inline bool IsFileAStockAsset ( LPSTR pCheckThisFile ) { return IsFileAStockAsset((const char*)pCheckThisFile); }
inline bool IsFileAStockAsset ( const std::string& pCheckThisFile ) { return IsFileAStockAsset(pCheckThisFile.c_str()); }
inline bool IsFileAStockAsset ( const cstr& pCheckThisFile ) { return IsFileAStockAsset(pCheckThisFile.Get()); }

void mapfile_convertCLASSICtoMAX(LPSTR pFPMLoaded);
