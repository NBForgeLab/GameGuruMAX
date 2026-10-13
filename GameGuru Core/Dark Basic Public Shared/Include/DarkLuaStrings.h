#pragma once
#include <cstddef>
#include <string>
#include <string_view>
struct lua_State;
// The view borrows Lua storage; keep its stack value alive until copying it.
std::string_view LuaGetStringView(lua_State* L, int idx, std::string_view fallback = "");
std::string LuaGetStringSafe(lua_State* L, std::string_view name);
void LuaGetString(lua_State* L, std::string_view name, char* destination, size_t capacity);
int LuaSendMessage(lua_State* L);
int LuaSendMessageI(lua_State* L);
int LuaSendMessageF(lua_State* L);
int LuaSendMessageS(lua_State* L);
int LuaNext();
// Message views remain valid until the next LuaNext/Clear or queue mutation.
std::string_view LuaCurrentMessageDescription();
std::string_view LuaCurrentMessageString();
int LuaCurrentMessageIndex();
int LuaCurrentMessageInt();
float LuaCurrentMessageFloat();
size_t LuaMessageQueueSize();
void LuaClearMessageQueue();
