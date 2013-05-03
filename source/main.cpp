// ----------------------------------------------------------------------------
#include <program_options.h>
#include <cmdque.h>
#include <socket.h>
#include <inet_socket_address.h>
#include <json.h>
#include <jsonrpc.h>
#include <jsonrpc_spotify_handler.h>
#include <spotify.h>
#include <log.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <regex>

// ----------------------------------------------------------------------------
#include <signal.h>

// ----------------------------------------------------------------------------
class options : public program_options::container
{
public:
    options()
        :
        help(false),
        address("0.0.0.0"),
        port(8081),
        username(),
        password(),
        audio_device_name("default"),
        conf_filename("spotihifi.conf")
    {
        add('h', "help", "display this message", help);
        add('a', "address", "local interface ip address to bind to", address, "IP");
        add('p', "port", "port to listen on", port, "INT");
        add('u', "username", "spotify username", username, "STRING");
        add(-1, "password", "spotify password", password, "STRING");
        add(-1, "audio-device", "audio output device name eg \"plughw:0,0\"", audio_device_name, "STRING");
        add('c', "conf", "configuration filename, default spotihifi.conf", conf_filename, "STRING");
    }
public:
    bool        help;
    std::string address;
    int         port;
    std::string username;
    std::string password;
    std::string audio_device_name;
    std::string conf_filename;
};

// ----------------------------------------------------------------------------
void parse_conf_file(const std::string& filename, options& options)
{
    std::ifstream f(filename);

    if ( ! f.good() ) {
        return;
    }

    std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    json::value  doc;
    json::parser parser(doc);

    try
    {
        parser.parse(str.c_str(), str.length());

        if ( !doc.is_object() ) {
            throw std::runtime_error("configuration file must be a json object!");
        }

        json::object conf = doc.get<json::object>();

        if ( options.username.length() == 0 && conf.has("spotify_username") ) {
            options.username = conf.get("spotify_username").get<json::string>().str();
        }

        if ( options.password.length() == 0 && conf.has("spotify_password") ) {
            options.password = conf.get("spotify_password").get<json::string>().str();
        }

        if ( options.audio_device_name == "default" && conf.has("audio_device_name") ) {
            options.audio_device_name = conf.get("audio_device_name").get<json::string>().str();
        }
    }
    catch (const std::exception& e)
    {
        std::string err("configuration file parse error: ");
        err += std::string(e.what());
        throw std::runtime_error(err.c_str());
    }
}

// ----------------------------------------------------------------------------
class client_connection
{
public:
    client_connection(inet::tcp::socket socket, jsonrpc_handler* handler)
        :
        m_socket(std::move(socket)),
        m_handler(handler),
        m_cmdq(),
        m_running(false)
    {
    }
public:
    client_connection(client_connection&& other)
        :
        m_socket(std::move(other.m_socket)),
        m_handler(std::move(other.m_handler)),
        //m_cmdq(std::move(other.m_cmdq)),
        m_cmdq(),
        m_running(false)
    {
    }
private:
    client_connection(const client_connection&) = delete;
    client_connection& operator=(const client_connection&) = delete;
public:
    void send(json::value message)
    {
        m_cmdq.push([=]()
        {
            std::string body = to_string(message);
            std::vector<char> buf(4);

            size_t len = body.length();
            buf[0] = len>>24;
            buf[1] = len>>16;
            buf[2] = len>>8;
            buf[3] = len;

            buf.insert(begin(buf)+4, begin(body), end(body));

            size_t sent = 0;
            do {
                sent += m_socket.send(buf.data(), buf.size(), 0);
            } while ( sent < buf.size() );
        });
    }
private:
    void disconnect()
    {
        m_cmdq.push([this]() {
            m_running = false;
        });
    }
public:
    void operator()()
    {
        m_running = true;

        // Start receive thread.
        std::thread receive_thr(&client_connection::receive_loop, this);

        try
        {
            // Enter command loop.
            while ( m_running )
            {
                auto cmd = m_cmdq.pop(std::chrono::seconds(60), [this]{
                    LOG(DEBUG) << "client connection idle";
                    send(json::object());
                });
                cmd();
            }
            // Join receive thread before terminating.
            LOG(DEBUG) << "client connection joining receiver";
            receive_thr.join();
            LOG(INFO) << "client connection terminated";
        }
        catch(std::exception& e) {
            LOG(DEBUG) << "client connection joining receiver";
            receive_thr.join();
            LOG(ERROR) << "client connection terminated! " << e.what();
        }
        catch(...) {
            LOG(DEBUG) << "client connection joining receiver";
            receive_thr.join();
            LOG(ERROR) << "client connection terminated!";
        }
    }
private:
    void receive_loop()
    {
        try {
            while ( true ) {
                receive_and_process_request();
            }
        }
        catch(const std::exception& e) {
            LOG(ERROR) << "client connection receive error! " << e.what();
            disconnect();
        }
        catch(...) {
            LOG(ERROR) << "client connection receive error!";
            disconnect();
        }
    }
private:
    void receive_and_process_request()
    {
        char hbuf[4];

        receive(hbuf, 4);

        LOG(DEBUG) << "hbuf:"
            << static_cast<int>(hbuf[0]) << ", "
            << static_cast<int>(hbuf[1]) << ", "
            << static_cast<int>(hbuf[2]) << ", "
            << static_cast<int>(hbuf[3]);

        size_t hlen = 0;

        hlen += hbuf[3];
        hlen += hbuf[2]<<8;
        hlen += hbuf[1]<<16;
        hlen += hbuf[0]<<24;

        LOG(DEBUG) << "hlen=" << hlen;

        std::vector<char> bbuf(hlen);

        receive(bbuf.data(), hlen);

        //std::cout << "body: '" << std::string(begin(bbuf), end(bbuf)) << "'" << std::endl;

        json::value  doc;
        json::parser parser(doc);

        size_t consumed = parser.parse(bbuf.data(), hlen);

        LOG(DEBUG) << "parser consumed=" << consumed << ", complete=" << parser.complete();
        //std::cout << "body: '" << doc << "'" << std::endl;

        auto request = jsonrpc_request::from_json(doc);

        if ( request.is_valid() )
        {
            json::object response;

            LOG(DEBUG) << "received request " << doc;

            response.set("jsonrpc", "2.0");
            response.set("id", request.id());

            m_handler->call_method(request.method(), request.params(), response);

            LOG(DEBUG) << "sending response length=" << to_string(response).length();

            send(std::move(response));
        }
        else
        {
            LOG(INFO) << "invalid jsonrpc request: " << request.error();

            json::object response;

            response.set("jsonrpc", "2.0");
            response.set("error", request.error());
            response.set("id", request.id());

            send(std::move(response));
        }
    }
private:
    void receive(char* buf, size_t len)
    {
        size_t i = 0;
        do
        {
            size_t received = m_socket.recv(buf+i, len-i, 0);
            if ( received == 0 ) {
                throw std::runtime_error("disconnect");
            }
            i += received;
        } while ( i < len );
    }
private:
    inet::tcp::socket m_socket;
    std::unique_ptr<jsonrpc_handler> m_handler;
    cmdque_t m_cmdq;
    bool m_running;
};

// ----------------------------------------------------------------------------
void sig_handler(int signum)
{
    std::cerr << std::endl;
    LOG(INFO) << "got signal " << signum;
    throw std::runtime_error("interrupted");
}

// ----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    options options;

    if ( signal(SIGPIPE, sig_handler) == SIG_ERR ) {
        std::cerr << "Error installing signal handler!" << std::endl;
    }

    if ( signal(SIGINT, sig_handler) == SIG_ERR ) {
        std::cerr << "Error installing signal handler!" << std::endl;
    }

    try
    {
        options.parse(argc, argv);

        if ( options.help )
        {
            std::cout << "Usage: main [OPTION...]" << std::endl
                      << std::endl
                      << "spotihifi server" << std::endl
                      << std::endl
                      << "Available option:" << std::endl
                      << options << std::endl;
            return 0;
        }

        parse_conf_file(options.conf_filename, options);

        spotify_t spotify(options.audio_device_name);

        spotify.login(options.username, options.password);

        inet::tcp::socket listener;

        LOG(INFO) << "starting server on " << options.address << ":" << options.port << " CTRL-C to stop";

        listener.bind(inet::socket_address(options.address.c_str(), options.port));
        listener.listen(5);

        inet::socket_address client_address;

        while ( true )
        {
            inet::tcp::socket client = listener.accept(client_address);

            LOG(INFO) << "client "
                      << client_address.ip() << ":" << client_address.port()
                      << " connected";

            std::thread c1(client_connection(std::move(client), new jsonrpc_spotify_handler(spotify)));
            c1.detach();
        }
    }
    catch(const program_options::error& err) {
        std::cerr << err.what() << std::endl
                  << "try: example --help" << std::endl;
    }
    catch(const std::exception& err) {
        LOG(FATAL) << err.what();
    }
}