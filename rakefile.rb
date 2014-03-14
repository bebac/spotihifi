# -----------------------------------------------------------------------------
require 'rake/clean'
require 'rake/tasklib'

# -----------------------------------------------------------------------------
require './rakelib/lib/ctasklib'

# -----------------------------------------------------------------------------
case ENV["variant"]
when "release"
    ENV["CFLAGS"]  = %q(-O2 -Wall -MMD)
    ENV["LDFLAGS"] = %q(-pthread)
else
    ENV["CFLAGS"]  = %q(-g -Wall -MMD)
    ENV["LDFLAGS"] = %q(-g -pthread)
end

ENV["CPPFLAGS"] = %q(-std=c++11)

# -----------------------------------------------------------------------------
popt = Rake::StaticLibraryTask.new("vendor/program-options/program-options.yml")
json = Rake::StaticLibraryTask.new("vendor/json/json.yml")

# -----------------------------------------------------------------------------
spec = Rake::ExecutableSpecification.new do |s|
    s.name = 'spotihifid'
    s.includes.add %w(
        source
        vendor/program-options/include
        vendor/json/include
        vendor/libspotify-12.1.51-Linux-x86_64-release/include
    )
    s.libincludes.add %w(
        build
        vendor/libspotify-12.1.51-Linux-x86_64-release/lib
    )
    s.sources.add %w(
        source/**/*.cpp
        source/appkey.c
    )
    s.libraries += [ popt, json ] + %w(asound b64 spotify)
end

# -----------------------------------------------------------------------------
Rake::ExecutableTask.new(:spotihifid, spec)

# -----------------------------------------------------------------------------
CLEAN.include('build')
# -----------------------------------------------------------------------------
task :default => [ :spotihifid ]
task :all => [ :default ]
