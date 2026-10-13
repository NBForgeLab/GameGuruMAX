//----------------------------------------------------
//--- GAMEGURU - M-Multiplayer Stub
//----------------------------------------------------

#include "gameguru.h"
#include "M-MP.h"

// Clean stub replacement for M-MPPhoton.cpp in GameGuru MAX
// All multiplayer functions are empty no-ops since MAX does not use legacy Photon multiplayer.

void mp_init ( void ) {}
void mp_fullinit ( void ) {}
void mp_fullclose ( void ) {}
void mp_loop ( void ) {}
void mp_free ( void ) {}
void mp_checkToCleanUpMasterHostList ( void ) {}
void mp_checkVoiceChat ( void ) {}
void mp_spawn_objects ( void ) {}
void mp_lua ( void ) {}
void mp_delete_entities ( void ) {}

void mp_gamecloud_getlist(void) {}
int mp_gamecloud_delete(LPSTR fileName) { return 0; }
void mp_gamecloud_deleteALLgamefiles(LPSTR siteName) {}
int mp_gamecloud_overwriteexisting(LPSTR fileName) { return 0; }
int mp_gamecloud_upload(bool bStartUpload, LPSTR fileName, LPSTR pFinalFilenameToUse) { return 0; }
float mp_gamecloud_getprogress(void) { return 0.0f; }
LPSTR mp_gamecloud_geterror(void) { return NULL; }

void mp_pre_game_file_sync ( void ) {}
void mp_pre_game_file_sync_server ( int iOnlySendMapToSpecificPlayer ) {}
void mp_pre_game_file_sync_client ( void ) {}
void mp_sendAvatarInfo ( void ) {}
void mp_animation ( void ) {}
void mp_update_player ( void ) {}
void mp_updatePlayerPositions ( void ) {}
void mp_server_message ( void ) {}
void mp_updatePlayerNamePlates ( void ) {}
void mp_updatePlayerAnimations ( void ) {}
void mp_switchDirectAnim ( int iAnimIndex ) {}
void mp_switchAnim ( void ) {}
void mp_update_waist_rotation ( void ) {}
void mp_showdeath ( void ) {}
void mp_respawn ( void ) {}
void mp_getPlaceToSpawn ( void ) {}
void mp_getInitialPlayerCount ( void ) {}
void mp_nukeTestmap ( void ) {}
void mp_respawnEntities ( void ) {}
void mp_addDestroyedObject ( void ) {}
void mp_add_respawn_timed ( void ) {}
void mp_setLuaPlayerNames ( void ) {}
void mp_setLuaResetStats ( void ) {}
void mp_updatePlayerInput ( void ) {}
void mp_load_guns ( void ) {}
void mp_check_for_attachments ( void ) {}
void mp_addJetpackParticles ( void ) {}
void mp_NearOtherPlayers ( void ) {}
void mp_check_respawn_objects ( void ) {}
void mp_checkForEveryoneLeft ( void ) {}
void mp_lostConnection ( void ) {}
void mp_gameLoop ( void ) {}
void mp_dontShowOtherPlayers ( void ) {}
void mp_ending_game ( void ) {}
void mp_free_game ( void ) {}
void mp_subbedToItem ( void ) {}
void mp_checkItemSubbed ( void ) {}
void mp_resetGameStats ( void ) {}
void mp_update_all_projectiles ( void ) {}
void mp_destroyentity ( void ) {}
void mp_refresh ( void ) {}
int mp_closeconnection ( void ) { return 0; }
void mp_setMessage ( void ) {}
void mp_setMessageDots ( void ) {}
void mp_message ( void ) {}
void mp_messageDots ( void ) {}
void mp_update_projectile ( void ) {}
void mp_serverSetLuaGameMode ( void ) {}
void mp_setServerTimer ( void ) {}
void mp_serverRespawnAll ( void ) {}
void mp_restoreEntities ( void ) {}
void mp_serverEndPlay ( void ) {}
void mp_setServerKillsToWin ( void ) {}
void mp_networkkill ( void ) {}
void mp_lobbyListBox ( void ) {}
void mp_createLobby ( void ) {}
void mp_searchForLobbies ( void ) {}
void mp_searchForFpms ( void ) {}
void mp_launchGame ( void ) {}
void mp_restartMultiplayerSystem ( void ) {}
void mp_backToStart ( void ) {}
void mp_selectedALevel ( void ) {}
void mp_checkIfLevelHasCustomContent ( void ) {}
void mp_buildWorkShopItem ( void ) {}
void mp_buildingWorkshopItemFailed ( void ) {}
void mp_joinALobby ( void ) {}
void mp_canIJoinThisLobby ( void ) {}
void mp_leaveALobby ( void ) {}
void mp_SubscribeToWorkShopItem ( void ) {}
void mp_save_workshop_files_needed ( void ) {}
void mp_grabWorkshopChangedFlagAndVersion ( void ) {}
int mp_check_if_entity_is_from_install ( char* name_s ) { return 0; }
void mp_resetSteam ( void ) {}
void mp_shoot ( void ) {}
void mp_chat ( void ) {}
void mp_chatNew ( void ) {}
void mp_quitGame ( void ) {}
void mp_freefadesprite ( void ) {}
void mp_backToEditor ( void ) {}
void mp_cleanupGame ( void ) {}
void mp_sendSteamIDToEditor ( void ) {}
void mp_checkIfLobbiesAvailable ( void ) {}
void mp_flashLightOff ( void ) {}
void mp_setupCoopTeam ( void ) {}
void mp_COOP_aiMoveTo ( void ) {}
void mp_entity_lua_lookatplayer ( void ) {}
void mp_entity_lua_fireweaponEffectOnly ( void ) {}
void mp_updateAIForCOOP ( void ) {}
void mp_coop_rotatetoplayer ( void ) {}
void mp_storeOldEntityPositions ( void ) {}
void mp_howManyEnemiesLeftToKill ( void ) {}
void mp_IKilledAnAI ( void ) {}
void mp_text ( int x, int y, int size, char* txt_s ) {}
void mp_textDots ( int x, int y, int size, char* txt_s ) {}
void mp_textColor ( int x, int y, int size, char* txt_s, int r, int g, int b ) {}
void mp_panel ( int x, int y, int x2, int y2 ) {}
void mp_deleteFile ( char* tempFileToDelete_s ) {}
int mp_check_if_lua_entity_exists ( int tentitytocheck ) { return 0; }
void mp_sendlua ( int code, int e, int v ) {}
void mp_sendluaToPlayer ( int iRealPhotonPlayerID, int code, int e, int v ) {}

// Legacy Steam multiplayer stubs
int SteamInit() { return 0; }
void SteamFree() {}
bool SteamOwned ( void ) { return false; }
LPSTR SteamGetPlayerName(void) { return (LPSTR)""; }
LPSTR SteamGetOtherPlayerName( int index ) { return (LPSTR)""; }
LPSTR SteamGetPlayerID(void) { return (LPSTR)""; }
void SteamLoop(void) {}
int SteamIsOnline(void) { return 0; }
void SteamAddAch(void) {}
void ParseCommandLine( const char *pchCmdLine, const char **ppchServerAddress, const char **ppchLobbyID, bool *pbUseVR ) {}
void SteamInitClient() {}
void SteamCreateLobby() {}
int SteamIsLobbyCreated() { return 0; }
void SteamGetLobbyList() {}
int SteamIsLobbyListCreated() { return 0; }
int SteamGetLobbyListSize() { return 0; }
LPSTR SteamGetLobbyListName( int index ) { return (LPSTR)""; }
void SteamJoinLobby( int index ) {}
int SteamGetLobbyUserCount() { return 0; }
int SteamHasJoinedLobby() { return 0; }
void SteamStartServer() {}
int SteamIsServerRunning() { return 0; }
int SteamIsGameRunning() { return 0; }
int SteamGetMyPlayerIndex() { return 0; }
void SteamSetPlayerPositionX( float _x) {}
void SteamSetPlayerPositionY( float _y ) {}
void SteamSetPlayerPositionZ( float _z ) {}
void SteamSetPlayerAngle( float _angle ) {}
void SteamSetPlayerScore( int index, int score ) {}
float SteamGetPlayerPositionX ( int index ) { return 0.0f; }
float SteamGetPlayerPositionY ( int index ) { return 0.0f; }
float SteamGetPlayerPositionZ ( int index ) { return 0.0f; }
float SteamGetPlayerAngle ( int index ) { return 0.0f; }
int SteamGetPlayerScore ( int index ) { return 0; }
void SteamSetBullet ( int index , float x , float y , float z, float anglex, float angley, float anglez, int type, int on ) {}
int SteamGetBulletOn ( int index ) { return 0; }
int SteamGetBulletType ( int index ) { return 0; }
float SteamGetBulletX ( int index ) { return 0.0f; }
float SteamGetBulletY ( int index ) { return 0.0f; }
float SteamGetBulletZ ( int index ) { return 0.0f; }
float SteamGetBulletAngleX ( int index ) { return 0.0f; }
float SteamGetBulletAngleY ( int index ) { return 0.0f; }
float SteamGetBulletAngleZ ( int index ) { return 0.0f; }
void SteamSetKeyState ( int key , int state ) {}
int SteamGetKeyState ( int index, int key ) { return 0; }
void SteamApplyPlayerDamage ( int index, int damage, int x, int y, int z, int force, int limb ) {}
int SteamGetPlayerDamageSource() { return 0; }
int SteamGetPlayerDamageX() { return 0; }
int SteamGetPlayerDamageY() { return 0; }
int SteamGetPlayerDamageZ() { return 0; }
int SteamGetPlayerDamageForce() { return 0; }
int SteamGetPlayerDamageLimb() { return 0; }
int SteamGetPlayerKilledSource( int index ) { return 0; }
int SteamGetPlayerKilledX( int index ) { return 0; }
int SteamGetPlayerKilledY( int index ) { return 0; }
int SteamGetPlayerKilledZ( int index ) { return 0; }
int SteamGetPlayerKilledForce( int index ) { return 0; }
int SteamGetPlayerKilledLimb( int index ) { return 0; }
void SteamKilledBy ( int killedBy, int x, int y, int z, int force, int limb ) {}
void SteamKilledSelf() {}
LPSTR SteamGetServerMessage( void ) { return (LPSTR)""; }
int SteamGetPlayerDamageAmount () { return 0; }
int SteamGetClientServerConnectionStatus() { return 0; }
void SteamResetClient() {}
void SteamCleanupClient() {}
void ResetGameStats() {}
void SteamSetPlayerAlive ( int state ) {}
int SteamGetPlayerAlive ( int index ) { return 0; }
void SteamSpawnObject ( int obj, int source, float x, float y, float z ) {}
void SteamDeleteObject ( int obj ) {}
void SteamDestroy ( int obj ) {}
void SteamSendLua ( int code, int e, int v ) {}
void SteamSendLuaString ( int code, int e, LPSTR s ) {}
int SteamGetLuaList() { return 0; }
void SteamGetNextLua() {}
int SteamGetLuaCommand() { return 0; }
int SteamGetLuaE() { return 0; }
int SteamGetLuaV() { return 0; }
LPSTR SteamGetLuaS(void) { return (LPSTR)""; }
int SteamGetSpawnList() { return 0; }
void SteamGetNextSpawn() {}
int SteamGetSpawnObjectNumber() { return 0; }
int SteamGetSpawnObjectSource() { return 0; }
float SteamGetSpawnX() { return 0.0f; }
float SteamGetSpawnY() { return 0.0f; }
float SteamGetSpawnZ() { return 0.0f; }
int SteamGetDeleteList() { return 0; }
void SteamGetNextDelete() {}
int SteamGetDeleteObjectNumber() { return 0; }
int SteamGetDeleteSource() { return 0; }
int SteamGetDestroyList() { return 0; }
void SteamGetNextDestroy() {}
int SteamGetDestroyObjectNumber() { return 0; }
int SteamReadyToSpawn() { return 0; }
void SteamSetSendFileCount(int count) {}
void SteamSendFileBegin ( int index , LPSTR pString ) {}
int SteamSendFileDone() { return 0; }
int SteamAmIFileSynced() { return 0; }
int SteamIsEveryoneFileSynced() { return 0; }
void SteamSendIAmLoadedAndReady() {}
int SteamIsEveryoneLoadedAndReady() { return 0; }
void SteamSendIAmReadyToPlay() {}
int SteamIsEveryoneReadyToPlay() { return 0; }
int SteamGetFileProgress() { return 0; }
void SteamSetVoiceChat( int on ) {}
void SteamSetPlayerAppearance( int a ) {}
int SteamGetPlayerAppearance( int index ) { return 0; }
void SteamSetCollision ( int index, int state ) {}
int SteamGetCollisionList() { return 0; }
void SteamGetNextCollision() {}
int SteamGetCollisionIndex() { return 0; }
int SteamGetCollisionState() { return 0; }
void SteamShoot () {}
int SteamGetShoot ( int index ) { return 0; }
void SteamPlayAnimation ( int index, int start, int end, int speed ) {}
int SteamGetAnimationList() { return 0; }
void SteamGetNextAnimation() {}
int SteamGetAnimationIndex() { return 0; }
int SteamGetAnimationStart() { return 0; }
int SteamGetAnimationEnd() { return 0; }
int SteamGetAnimationSpeed() { return 0; }
void SteamSetTweening(int index , int flag) {}
LPSTR SteamGetLobbyUserName( int index) { return (LPSTR)""; }
void SteamLeaveLobby() {}
void SteamCreateWorkshopItem() {}
void SteamCreateWorkshopItem( LPSTR pString ) {}
void SteamDownloadWorkshopItem( LPSTR pString ) {}
int SteamIsWorkshopItemDownloaded() { return 0; }
int SteamIsWorkshopItemUploaded() { return 0; }
LPSTR SteamGetWorkshopID(void) { return (LPSTR)""; }
LPSTR SteamGetWorkshopItemPath(void) { return (LPSTR)""; }
void SteamGetWorkshopItemPathDLL(LPSTR string ) {}
int SteamIsWorkshopLoadingOnDLL() { return 0; }
void SteamSetRoot(LPSTR string ) {}
void SteamSetLobbyName(LPSTR name ) {}
int SteamIsWorkshopItemInstalled(LPSTR idString ) { return 0; }
int SteamHasSubscriptionWorkshopItemFinished() { return 0; }
void SteamSendMyName() {}
int SteamIsOverlayActive() { return 0; }
void SteamWorkshopModeOff() {}
void SteamShowAgreement() {}
void SteamEndGame() {}
void SteamSendChat( LPSTR msg ) {}
void SteamSendLobbyChat( LPSTR msg ) {}
LPSTR SteamGetChat( void ) { return (LPSTR)""; }
int SteamInkey( int scancode ) { return 0; }
int SteamCheckSyncedAvatarTexturesWithServer() { return 0; }
void SteamSetMyAvatarHeadTextureName(LPSTR sAvatarTexture) {}
void SteamAvatarClient() {}
void SteamAvatarServer() {}
int SteamStrCmp( LPSTR s1, LPSTR s2 ) { return 0; }

// Legacy DarkAI stub
char* AIGetEntityState ( int iObjID ) { static char szEmpty[1] = { 0 }; return szEmpty; }

