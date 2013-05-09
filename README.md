SpotiHifi
=========

SpotiHifi is a gui-less spotify client which is controlled by sending jsonrpc
requests through a tcp connection. On startup the spotihifi daemon imports
tracks from all your playlists. A spotihifi client can then connect to the
spotihifi daemon to get the full set of tracks and control playback. The
basic idea is to run the spotihifi daemon on a pc connected to your hifi
system and be able to control it from your tablet, phone and/or laptop.

> NOTE: Currently it is linux only



What do I need to get started
-----------------------------

It is written in c++ using c++11 features, so you'll need a fairly new gcc.
It is known to compile with gcc 4.6.3 and above.

You will need a spotify library that fits your platform. Get it from the
spotify development site and place it in the vendor directory:

    vendor/libspotify-12.1.51-Linux-x86_64-release

libasound is used to playback audio. On Ubuntu do:

    sudo apt-get install libasound2-dev

I have not included the spotify application key in the repository, so you
need to apply for one and place the appkey.c in the source folder.

ruby/rake is used to build. It needs some work on the configuration side, so
you will need to modify the rakefile depending on your platform. The included
version works for gcc 4.7 and x86_64.

If you are on 32 bit, gcc 4.6.3 (ubuntu 12.04) it should look something like
this:

```ruby
spec = Rake::ExecutableSpecification.new do |s|
    s.name = 'spotihifid'
    s.includes.add(
        'source',
        'vendor/program-options/include',
        'vendor/libspotify-12.1.51-Linux-i686-release/include'
    )
    s.libincludes.add(
        'vendor/libspotify-12.1.51-Linux-i686-release/lib'
    )
    s.sources.add(
        'vendor/program-options/source/*.cpp',
        'source/**/*.cpp',
        'source/appkey.c'
    )
    s.libraries += %w(asound spotify)
    s.compiler_options += %w(-g -Wall -std=c++0x)
end
```

With that in place you just need to invoke rake in the top directory to build

    rake

After build run it:

    LD_LIBRARY_PATH+=vendor/libspotify-12.1.51-Linux-x86_64-release/lib/ ./build/spotihifid



The jsonrpc API
---------------

THIS IS IN EARLY DEVELOPMENT AND WILL LIKELY CHANGE A LOT!

### Sync

When a client connects it should issue a sync command to get a list of all tracks
known to the spotihifi daemon. The sync response will include an incarnation token
and a transaction count which the client must include in subsequent sync requests
to only receive partial updates.

> NOTE: The transaction count is not currenly used.


    --> { "jsonrpc" : "2.0", "method" : "sync", "params": { "incarnation" : "-1", "transaction" : "-1" }, "id" : 1 }
    <-- { "jsonrpc" : "2.0", "result" : { "incarnation" : "-1080086476", "transaction" : "0", "tracks" :
            [
              {
                "album":"Sounds So Good",
                "playlists":["Albums"],
                "artist":"Ashton Shepherd",
                "track_number":5,
                "title":"Not Right Now",
                "track_id":"2fCQc7T8T5QAwq6rvNfw98"
              },
              {
                "album":"The Incredible Machine",
                "playlists":["Starred"],"artist":
                "Sugarland",
                "track_number":3,
                "title":"Stuck Like Glue",
                "track_id":"2d2GOl3u4xY0UjGvtuw5tn"
              },
              {
                "album":"The Foundation",
                "playlists":["Albums","Country","Zac Brown Band","Starred"],
                "artist":"Zac Brown Band",
                "track_number":12,
                "title":"Sic 'Em On A Chicken",
                "track_id":"6QBkPN3ARmAFVtVAk7oYN9"
              }
            ] },
          "id" : 1
        }

    # On a subsequent sync request the response will contain no tracks indicating that everything
    # is up to date.
    --> { "jsonrpc" : "2.0", "method" : "sync", "params": { "incarnation" : "-1080086476", "transaction" : "0" }, "id" : 1 }
    <-- { "jsonrpc" : "2.0", "result" : { "incarnation" : "-1080086476", "transaction" : "0" }, "id" : 1 }


It is up to the client to organize the tracks and present them to the user. One
possible solution is to put the tracks into an sqlite database. _TODO: insert reference to android spotihifi app_

### Queue

    --> { "jsonrpc" : "2.0", "method" : "queue", "params" : [ "spotify:track:2d2GOl3u4xY0UjGvtuw5tn" ], "id" : 2 }
    <-- { "jsonrpc" : "2.0", "result" : "ok", "id" : 2 }

### Play

To continue playback after pause:

    --> { "jsonrpc" : "2.0", "method" : "play", "params" : [], "id" : 3 }
    <-- { "jsonrpc" : "2.0", "result" : "ok", "id" : 3 }

To start playback of a playlist:

    --> { "jsonrpc" : "2.0", "method" : "play", "params" : { "playlist" : "Albums" }, "id" : 3 }
    <-- { "jsonrpc" : "2.0", "result" : "ok", "id" : 3 }

To start playback from all tracks:

    --> { "jsonrpc" : "2.0", "method" : "play", "params" : { "playlist" : "" }, "id" : 3 }
    <-- { "jsonrpc" : "2.0", "result" : "ok", "id" : 3 }

### Pause

    --> { "jsonrpc" : "2.0", "method" : "pause", "params" : [], "id" : 4 }
    <-- { "jsonrpc" : "2.0", "result" : "ok", "id" : 4 }

### Skip

    --> { "jsonrpc" : "2.0", "method" : "skip", "params" : [], "id" : 5 }
    <-- { "jsonrpc" : "2.0", "result" : "ok", "id" : 5 }

### Stop

    --> { "jsonrpc" : "2.0", "method" : "stop", "params" : [], "id" : 6 }
    <-- { "jsonrpc" : "2.0", "result" : "ok", "id" : 6 }
