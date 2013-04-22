// ----------------------------------------------------------------------------
//
//     Filename:  jsonrpc_spotify_handler.h
//
//  Description:
//
//       Author:  Benny Bach
//      Company:
//
// ----------------------------------------------------------------------------
#ifndef __jsonrpc_spotify_handler_h__
#define __jsonrpc_spotify_handler_h__

// ----------------------------------------------------------------------------
#include <json.h>
#include <spotify.h>
#include <log.h>

// ----------------------------------------------------------------------------
#include <fstream>

// ----------------------------------------------------------------------------
class jsonrpc_spotify_handler : public jsonrpc_handler
{
public:
    jsonrpc_spotify_handler(spotify_t& spotify)
        :
        spotify(spotify)
    {
    }
public:
    virtual void call_method(const std::string& method, json::value params, json::object& response)
    {
        LOG(INFO) << "method: '" << method << "', params:" << params;

        if ( method == "echo" )
        {
            json::array p = params.get<json::array>();
            auto v = p.at(0).get<json::string>().str();

            response.set("result", v);
        }
        else if ( method == "sync" )
        {
            std::ifstream f("songs.json");
            std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

            json::value  v;
            json::parser parser(v);

            try {
                parser.parse(str.c_str(), str.length());
            }
            catch (const std::exception& e) {
                LOG(ERROR) << "parse error:" << e.what();
            }

            response.set("result", v);
        }
        else if ( method == "play" ) {
            json::array p = params.get<json::array>();
            auto v = p.at(0).get<json::string>().str();

            spotify.player_play(v);
        }
        else
        {
            json::object e;

            e.set("code", -32601);
            e.set("message", "Method not found");

            response.set("error", e);
        }
    }
private:
    spotify_t& spotify;
};

// ----------------------------------------------------------------------------
#endif // __jsonrpc_spotify_handler_h__