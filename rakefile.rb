require 'rake/clean'
require 'rake/tasklib'

# -----------------------------------------------------------------------------
require_relative 'rakelib/executable'

# -----------------------------------------------------------------------------
spec = Rake::ExecutableSpecification.new do |s|
    s.name = 'spotihifid'
    s.includes.add(
        'source',
        'vendor/program-options/include',
        'vendor/libspotify-12.1.51-Linux-x86_64-release/include'
    )
    s.libincludes.add(
        'vendor/libspotify-12.1.51-Linux-x86_64-release/lib'
    )
    s.sources.add(
        'vendor/program-options/source/*.cpp',
        'source/**/*.cpp',
        'source/appkey.c'
    )
    s.libraries += %w(asound spotify)
    #s.compiler_options += %w(-g -Wall -std=c++11)
    s.compiler_options += %w(-O2 -Wall -std=c++11)
end
# -----------------------------------------------------------------------------
Rake::ExecutableTask.new(:spotihifid, spec) do |exe|
    exe.outputdir = 'build'
    exe.intermediatedir = File.join(exe.outputdir, 'spotihifid_obj')
end
# -----------------------------------------------------------------------------
CLEAN.include('build')
# -----------------------------------------------------------------------------
task :default => [ :spotihifid ]
task :all => [ :default ]
