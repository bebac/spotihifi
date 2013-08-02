require 'yaml'
require 'pathname'

module Rake

    ###
    # LibrarySpecification.
    #

    class LibrarySpecification

        attr_accessor :name
        attr_accessor :sources
        attr_accessor :includes

        def initialize
            yield self if block_given?
        end

        def static_lib_name
            "lib#{name}.a"
        end

    end

    ###
    # ExecutableSpecification.
    #

    class ExecutableSpecification

        attr_accessor :name
        attr_accessor :sources
        attr_accessor :includes
        attr_accessor :libincludes
        attr_accessor :libraries

        def initialize(name=nil)
            init
            yield self if block_given?
        end

        def init()
            @sources = Rake::FileList.new
            @includes = Rake::FileList.new
            @libincludes = Rake::FileList.new
            @libraries = Array.new
        end

    end

    ###
    # CProjectTaskLib. Base class for c/c++ projects.
    #

    class CProjectTaskLib < TaskLib

        def define
            define_main_task
            define_intermediatedir_task
            define_compile_tasks
            define_link_task
        end

        def objfiles
            spec.sources.pathmap("#{intermediatedir}/%n.o")
        end

        def depfiles
            spec.sources.pathmap("#{intermediatedir}/%n.d")
        end

        def intermediatedir=(dirname)
            @intermediatedir = dirname
        end

        def intermediatedir
            @intermediatedir || File.join(outputdir, name.to_s + ".d")
        end

        def outputdir=(dirname)
            @outputdir = dirname
        end

        def outputdir
            @outputdir || "build"
        end

        def define_intermediatedir_task
            directory intermediatedir
        end

        def define_compile_tasks
            includes = spec.includes.collect { |x| "-I"+x }.join(" ")
            spec.sources.zip(objfiles, depfiles).each do |srcfile, objfile, depfile|
                desc "Compile #{srcfile}"
                file objfile do
                    Rake::Task[intermediatedir].invoke
                    case srcfile
                    when /\.cpp$/
                        sh "g++ #{ENV["CFLAGS"]} #{ENV["CPPFLAGS"]} #{includes} -c #{srcfile} -o #{objfile}"
                    when /\.c$/
                        sh "gcc #{ENV["CFLAGS"]} #{includes} -c #{srcfile} -o #{objfile}"
                    else
                        fail("cannot compile #{src}")
                    end
                end
                file objfile => [ srcfile ]
                file objfile => prerequisites(depfile) rescue Errno::ENOENT
            end
        end

        def prerequisites(depfile)
            File.open(depfile, "r") do |file|
                prereqs = file.read().split(/:/, 2)[1].split(/\s/).delete_if { |x| x.empty? or x == "\\" }[1..-1]
            end
        end

    end

    ###
    # StaticLibraryTask. Define task to create a static library.
    #

    class StaticLibraryTask < CProjectTaskLib

        attr_accessor :name
        attr_accessor :spec

        def initialize(spec)
            if spec.class == String
                ###
                # TODO: Find a better solution than this ugly global prefix hack
                #       to make paths relative to the top level rakefile.
                $prefix = File.dirname(spec)
                ###
                spec = YAML.load_file(spec)
            end
            init(spec.name, spec)
            yield self if block_given?
            define #unless self.name.nil?
        end

        def init(name, spec)
            @name = name
            @spec = spec
        end

        def define_main_task
            desc "Build #{name}"
            task name => filename
        end

        def define_link_task
            desc "Build #{filename}"
            file filename => objfiles do
                sh "ar -r #{filename} #{objfiles.join(" ")}"
            end
        end

        def filename
            "#{outputdir}/#{spec.static_lib_name}"
        end

    end

    ###
    # ExecutableTask. Define task to create an executable console application.
    #

    class ExecutableTask < CProjectTaskLib

        attr_accessor :name
        attr_accessor :spec

        def initialize(name, spec)
            init(name, spec)
            yield self if block_given?
            define #unless self.name.nil?
        end

        def init(name, spec)
            @name = name
            @spec = spec
        end

        def define_main_task
            desc "Build #{name}"
            task name => executable
        end

        def define_link_task
            # Create library includes option string.
            libincludes = spec.libincludes.collect { |x|
                "-L"+x
            }.join(" ")
            # Create library link options + Add file dependencies to static library tasks
            libraries = spec.libraries.collect { |x|
                case x
                when String
                    "-l"+x
                when StaticLibraryTask
                    file executable => [ x.filename ]
                    "-l"+x.name
                end
            }.join(" ")
            # Define link task.
            desc "Build #{executable}"
            file executable => objfiles do
                sh "g++ #{ENV["LDFLAGS"]} -o #{executable} #{libincludes} #{objfiles.join(" ")} #{libraries}"
            end
        end

        def executable
            File.join(outputdir, spec.name)
        end

    end

end

###
# Add library specification yaml tag.
#

YAML::add_domain_type("bebac.com,2013", "library") do |type, val|
    Rake::LibrarySpecification.new do |lib|
        lib.name = val["name"]
        lib.sources = FileList.new(val["sources"].collect { |s| File.join($prefix, s) })
        lib.includes = val['includes'].collect { |i| File.join($prefix, i) }
    end
end
