std::string_view LuaGetStringView(lua_State* L, int idx, std::string_view fallback)
{
    if (!L) return fallback;
    const int top = lua_gettop(L);
    if (idx == 0 || idx > top || (idx < 0 && idx > LUA_REGISTRYINDEX && -idx > top)) return fallback;
    if (!lua_isstring(L, idx)) return fallback;
    size_t len = 0;
    const char* value = lua_tolstring(L, idx, &len);
    return value ? std::string_view(value, len) : fallback;
}

std::string LuaGetStringSafe(lua_State* L, std::string_view name)
{
    if (!L) return {};
    std::string key(name);
    lua_getglobal(L, key.c_str());
    const std::string_view value = LuaGetStringView(L, -1);
    std::string result(value);
    lua_pop(L, 1);
    return result;
}
void LuaGetString(lua_State* L, std::string_view name, char* destination, size_t capacity)
{
    if (!destination || capacity == 0) return;
    destination[0] = '\0';
    const std::string value = LuaGetStringSafe(L, name);
    const size_t count = (std::min)(value.size(), capacity - 1);
    if (count) memcpy(destination, value.data(), count);
    destination[count] = '\0';
}
std::string_view LuaCurrentMessageDescription() { return currentMessage.msgDesc; }
std::string_view LuaCurrentMessageString() { return currentMessage.msgString; }
int LuaCurrentMessageIndex() { return currentMessage.msgIndex; }
int LuaCurrentMessageInt() { return currentMessage.msgInt; }
float LuaCurrentMessageFloat() { return currentMessage.msgFloat; }
size_t LuaMessageQueueSize() { return luaMessages.size(); }
void LuaClearMessageQueue()
{
    luaMessages.clear();
    currentMessage = luaMessage();
}
