require 'readline'
require 'socket'
require 'thread'
require 'json'

module JsonRpc

  class Server

    def initialize(ip, port)
      @res_q = Queue.new
      @socket = TCPSocket.new(ip, port)
      @receiver = receiver
    end

    def execute(json)
      buf = json.unpack("C*")
      len = [buf.length].pack("N").unpack("C*")
      buf.unshift(*len)
      @socket.write(buf.pack("C*"))
      @res_q.pop
    end

    def close
      @socket.close
    end

    def receiver
      Thread.new do
        begin
          v = String.new
          begin
            r = @socket.recv(4)
            if r.length == 0
              raise RuntimeError.new("disconnect")
            else
              v += r
            end
          end while v.length < 4

          l = v.unpack("N")[0]

          v = String.new
          begin
            r = @socket.recv(l)
            if r.length == 0
              raise RuntimeError.new("disconnect")
            else
              v += r
            end
          end while v.length < l

          result = JSON::parse(v)

          #puts result.inspect

          if result.has_key?("jsonrpc") and result.has_key?("id")
            @res_q << result
          else
            puts "event:#{result.inspect}"
            #@evt_q << result
          end
        end while true
      end
    end

  end

end

module SpotifyClientConsole

  extend self

  def run
    # Connect to server.
    server = JsonRpc::Server.new("127.0.0.1", 8081)
    # Enter command loop.
    while line = Readline.readline('spc$ ', true)
      if line.length > 0
        command, _, arg = line.partition(/\s+/)
        ret = Commands.send command.to_sym, server, arg
        ret = nil
      end
    end
  end

  module Commands

    extend self

    def echo(server, s)
      msg = { :jsonrpc => "2.0", :method => "echo", :params => [ "#{s}" ], :id => 1 }.to_json
      result = server.execute(msg)
      puts result.fetch("result") { fail(result["error"].inspect) }
      #puts s
    end

    def sync(server, s)
      msg = { :jsonrpc => "2.0", :method => "sync", :params => [], :id => 2 }.to_json
      result = server.execute(msg)
      songs = result.fetch("result") { fail(result["error"].inspect) }
      puts songs.length
      #puts s
    end

    def exit(server, s)
      server.close
      Kernel.exit(0)
    end

  end

end

Thread.abort_on_exception = true

SpotifyClientConsole.run
