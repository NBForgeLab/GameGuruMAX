#include <deque>
#include <utility>

namespace
{
    struct luaMessage
    {
        std::string msgDesc;
        int msgIndex = 0;
        int msgInt = 0;
        float msgFloat = 0.0f;
        std::string msgString;
    };

    luaMessage currentMessage;
    std::deque<luaMessage> luaMessages;

    luaMessage ReadLuaMessageHeader(lua_State* L, int count)
    {
        luaMessage message;
        message.msgDesc = std::string(LuaGetStringView(L, 1));
        if (count == 3) message.msgIndex = static_cast<int>(lua_tonumber(L, 2));
        return message;
    }
}

// Retain the diagnostic state used by other DarkLUA bindings.
thread_local lua_State* g_CurrentLuaState = nullptr;
static int LUA_GETTOP(lua_State* L)
{
    g_CurrentLuaState = L;
    return lua_gettop(L);
}

int LuaSendMessage(lua_State* L)
{
    lua = L;
    const int count = LUA_GETTOP(L);
    for (int i = 1; i <= count; ++i)
    {
        luaMessage message;
        message.msgDesc = std::string(LuaGetStringView(L, i));
        luaMessages.push_back(std::move(message));
    }
    return 0;
}

int LuaSendMessageI(lua_State* L)
{
    lua = L;
    const int count = LUA_GETTOP(L);
    if (count != 2 && count != 3) return 0;
    luaMessage message = ReadLuaMessageHeader(L, count);
    message.msgInt = static_cast<int>(lua_tonumber(L, count));
    luaMessages.push_back(std::move(message));
    return 0;
}

int LuaSendMessageF(lua_State* L)
{
    lua = L;
    const int count = LUA_GETTOP(L);
    if (count != 2 && count != 3) return 0;
    luaMessage message = ReadLuaMessageHeader(L, count);
    message.msgFloat = static_cast<float>(lua_tonumber(L, count));
    luaMessages.push_back(std::move(message));
    return 0;
}

int LuaSendMessageS(lua_State* L)
{
    lua = L;
    const int count = LUA_GETTOP(L);
    if (count != 2 && count != 3) return 0;
    luaMessage message = ReadLuaMessageHeader(L, count);
    message.msgString = std::string(LuaGetStringView(L, count));
    luaMessages.push_back(std::move(message));
    return 0;
}

int LuaNext()
{
    if (luaMessages.empty())
    {
        currentMessage = luaMessage();
        return 0;
    }
    currentMessage = std::move(luaMessages.front());
    luaMessages.pop_front();
    return 1;
}
