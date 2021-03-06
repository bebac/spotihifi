// ----------------------------------------------------------------------------
//
//        Filename:  main.cpp
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <program_options.h>
#include <cmdque.h>
#include <socket.h>
#include <inet_socket_address.h>
#include <json/json.h>
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
    conf_filename("spotihifi.conf"),
    cache_dir("spotihifi_cache"),
    last_fm_username(),
    last_fm_password(),
    volume_normalization(false)
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
  std::string cache_dir;
  // Set from json configuration.
  std::string last_fm_username;
  std::string last_fm_password;
  std::string track_stat_filename;
  bool        volume_normalization;
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

    json::object conf = doc.as_object();

    if ( options.username.length() == 0 && conf["spotify_username"].is_string() ) {
      options.username = conf["spotify_username"].as_string();
    }

    if ( options.password.length() == 0 && conf["spotify_password"].is_string() ) {
      options.password = conf["spotify_password"].as_string();
    }

    if ( options.audio_device_name == "default" && conf["audio_device_name"].is_string() ) {
      options.audio_device_name = conf["audio_device_name"].as_string();
    }

    if ( conf["cache_dir"].is_string() ) {
      options.cache_dir = conf["cache_dir"].as_string();
    }

    if ( conf["last_fm_username"].is_string() && conf["last_fm_password"].is_string() )
    {
      options.last_fm_username = conf["last_fm_username"].as_string();
      options.last_fm_password = conf["last_fm_password"].as_string();
    }

    if ( conf["track_stat_filename"].is_string() ) {
      options.track_stat_filename = conf["track_stat_filename"].as_string();
    }

    if ( !conf["volume_normalization"].is_null() )
    {
      if ( conf["volume_normalization"].is_true() ) {
        options.volume_normalization = true;
      }
      else if ( conf["volume_normalization"].is_false() ) {
        options.volume_normalization = false;
      }
      else {
        throw std::runtime_error("configuration file error - volume_normalization must be true or false!");
      }
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
class client_connection : public notify_sender_t
{
public:
  client_connection(inet::tcp::socket socket, jsonrpc_spotify_handler* handler)
    :
    m_socket(std::move(socket)),
    m_handler(handler),
    m_cmdq(),
    m_running(false)
  {
    m_handler->player_observer_attach(m_handler);
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
public:
  ~client_connection()
  {
    if ( m_handler.get() ) {
      m_handler->player_observer_detach(m_handler);
    }
  }
private:
  client_connection(const client_connection&) = delete;
  client_connection& operator=(const client_connection&) = delete;
public:
  void send(json::value message)
  {
    m_cmdq.push([=]()
    {
      std::stringstream os;

      os << message;

      std::string body = os.str();

      std::vector<unsigned char> buf(4);

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
public:
  void send_notify(json::value message)
  {
    send(message);
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

    // Let the handler send notifies using this connection.
    m_handler->set_notify_sender(this);

    try
    {
      // Enter command loop.
      while ( m_running )
      {
        auto cmd = m_cmdq.pop(std::chrono::seconds(60), [this]{
          _log_(debug) << "client connection idle";
          send(json::object());
        });
        cmd();
      }
      // Join receive thread before terminating.
      _log_(debug) << "client connection joining receiver";
      receive_thr.join();
      _log_(info) << "client connection terminated";
    }
    catch(std::exception& e) {
      _log_(debug) << "client connection joining receiver";
      receive_thr.join();
      _log_(error) << "client connection terminated! " << e.what();
    }
    catch(...) {
      _log_(debug) << "client connection joining receiver";
      receive_thr.join();
      _log_(error) << "client connection terminated!";
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
      _log_(error) << "client connection receive error! " << e.what();
      disconnect();
    }
    catch(...) {
      _log_(error) << "client connection receive error!";
      disconnect();
    }
  }
private:
  void receive_and_process_request()
  {
    unsigned char hbuf[4];

    receive(hbuf, 4);

    _log_(debug) << "hbuf:"
        << static_cast<int>(hbuf[0]) << ", "
        << static_cast<int>(hbuf[1]) << ", "
        << static_cast<int>(hbuf[2]) << ", "
        << static_cast<int>(hbuf[3]);

    size_t hlen = 0;

    hlen += hbuf[3];
    hlen += hbuf[2]<<8;
    hlen += hbuf[1]<<16;
    hlen += hbuf[0]<<24;

    _log_(debug) << "hlen=" << hlen;

    std::vector<unsigned char> bbuf(hlen);

    receive(bbuf.data(), hlen);

    json::value  doc;
    json::parser parser(doc);

    size_t consumed = parser.parse(reinterpret_cast<char*>(bbuf.data()), hlen);

    _log_(debug) << "parser consumed=" << consumed << ", complete=" << parser.complete();
    //std::cout << "body: '" << doc << "'" << std::endl;

    auto request = jsonrpc_request::from_json(doc);

    if ( request.is_valid() )
    {
      json::object response{ { "jsonrpc", "2.0" }, { "id", request.id() } };

      _log_(debug) << "received request " << doc;

      m_handler->call_method(request.method(), request.params(), response);

      send(std::move(response));
    }
    else
    {
      _log_(info) << "invalid jsonrpc request: " << request.error();

      json::object response{
        { "jsonrpc", "2.0" },
        { "error", request.error() },
        { "id", request.id() }
      };

      send(std::move(response));
    }
  }
private:
  void receive(unsigned char* buf, size_t len)
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
  std::shared_ptr<jsonrpc_spotify_handler> m_handler;
  cmdque_t m_cmdq;
  bool m_running;
};

// ----------------------------------------------------------------------------
void sig_handler(int signum)
{
  std::cerr << std::endl;
  _log_(info) << "got signal " << signum;
  throw std::runtime_error("interrupted");
}

// ----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  options options;

  if ( signal(SIGPIPE, sig_handler) == SIG_ERR ) {
    std::cerr << "Error installing SIGPIPE handler!" << std::endl;
  }

  if ( signal(SIGINT, sig_handler) == SIG_ERR ) {
    std::cerr << "Error installing SIGINT handler!" << std::endl;
  }

  if ( signal(SIGTERM, sig_handler) == SIG_ERR ) {
    std::cerr << "Error installing SIGTERM handler!" << std::endl;
  }

  try
  {
    options.parse(argc, argv);

    if ( options.help )
    {
      std::cout << "Usage: spotihifid v0.1.6 [OPTION...]" << std::endl
                << std::endl
                << "spotihifi daemon" << std::endl
                << std::endl
                << "Available option:" << std::endl
                << options << std::endl
                << "This product uses SPOTIFY CORE but is not endorsed, certified or otherwise approved" << std::endl
                << "in any way by Spotify. Spotify is the registered trade mark of the Spotify Group." << std::endl
                << std::endl;
      return 0;
    }

    parse_conf_file(options.conf_filename, options);

    spotify_t spotify(
      options.audio_device_name,
      options.cache_dir,
      options.last_fm_username,
      options.last_fm_password,
      options.track_stat_filename,
      options.volume_normalization
    );

    spotify.login(options.username, options.password);

    inet::tcp::socket listener;

    _log_(info) << "starting server on " << options.address << ":" << options.port << " CTRL-C to stop";

    listener.reuseaddr(true);
    listener.bind(inet::socket_address(options.address.c_str(), options.port));
    listener.listen(5);

    inet::socket_address client_address;

    while ( true )
    {
      inet::tcp::socket client = listener.accept(client_address);

      _log_(info) << "client "
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
    _log_(fatal) << err.what();
  }
}