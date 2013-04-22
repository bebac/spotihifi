module Rake

    class ExecutableSpecification

        attr_accessor :name
        attr_accessor :sources
        attr_accessor :includes
        attr_accessor :libincludes
        attr_accessor :libraries
        attr_accessor :compiler_options

        def initialize(name=nil)
            init
            yield self if block_given?
        end

        def init()
            @sources = Rake::FileList.new
            @includes = Rake::FileList.new
            @libincludes = Rake::FileList.new
            @libraries = Array.new
            @compiler_options = Array.new
        end

    end

    class ExecutableTask < TaskLib

        attr_accessor :name
        attr_accessor :spec
        attr_accessor :intermediatedir
        attr_accessor :outputdir

        def initialize(name, spec)
            init(name, spec)
            yield self if block_given?
            define #unless self.name.nil?
        end

        def init(name, spec)
            @name = name
            @spec = spec
        end

        def define
            define_main_task
            define_intermediatedir_task
            define_compile_tasks
            define_link_task
        end

        def define_main_task
            desc "Build #{name}"
            task name => executable
        end

        def define_intermediatedir_task
            directory intermediatedir
        end

        def define_compile_tasks
            includes = spec.includes.collect { |x| "-I"+x }.join(" ")
            options = spec.compiler_options { |x| x }.join(" ")
            spec.sources.zip(objfiles, depfiles).each do |srcfile, objfile, depfile|
                desc "Compile #{srcfile}"
                file objfile do
                    Rake::Task[intermediatedir].invoke
                    sh "#{cc(srcfile)} #{options} -MD #{includes} -c #{srcfile} -o #{objfile}"
                end
                file objfile => [ srcfile ]
                file objfile => prerequisites(depfile) rescue Errno::ENOENT
            end
        end

        def define_link_task
            libincludes = spec.libincludes.collect { |x| "-L"+x }.join(" ")
            libraries = spec.libraries.collect { |x| "-l"+x }.join(" ")
            desc "Build #{executable}"
            file executable => objfiles do
                sh "g++ -g -pthread -o #{executable} #{libincludes} #{objfiles.join(" ")} #{libraries}"
            end
        end

        def executable
            #File.join(outputdir, spec.name + ".exe")
            File.join(outputdir, spec.name)
        end

        def objfiles
            spec.sources.pathmap("#{intermediatedir}/%n.o")
        end

        def depfiles
            spec.sources.pathmap("#{intermediatedir}/%n.d")
        end

        def cpp?(source)
            /\.[^.]*cpp$/.match(source)
        end

        def cc(source)
            cpp?(source) ? "g++" : "gcc"
        end

        def prerequisites(depfile)
            File.open(depfile, "r") do |file|
                prereqs = file.read().split(/:/, 2)[1].split(/\s/).delete_if { |x| x.empty? or x == "\\" }[1..-1]
                #prereqs.delete_if { |x| not File.exists?(x) }
            end
        end
    end
end
