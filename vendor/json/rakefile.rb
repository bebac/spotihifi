
def cc(obj_filename, src_filename)
  file obj_filename => [ src_filename ] do |t|
    sh "g++ -c -std=c++11 -Wall -O2 -DNDEBUG -o#{t.name} -Iinclude #{t.prerequisites[0]}"
  end
end

#file "json_value.o" => [ "source/json/json_value.cpp", "include/json/json_value.h", "include/json/json_array.h", "include/json/json_object.h", "include/json/json_parser.h" ] do |t|
#  #sh "g++ -c -pg -std=c++11 -Wall -O2 -o#{t.name} -Iinclude #{t.prerequisites[0]}"
#  sh "g++ -c -std=c++11 -Wall -O2 -DNDEBUG -o#{t.name} -Iinclude #{t.prerequisites[0]3}"
#end
cc "json_value.o", "source/json/json_value.cpp"

#file "json_parser.o" => [ "source/json/json_parser.cpp", "include/json/json_parser.h" ] do |t|
#  #sh "g++ -c -pg -std=c++11 -Wall -O2 -o#{t.name} -Iinclude #{t.prerequisites[0]}"
#  sh "g++ -c -std=c++11 -Wall -O2 -DNDEBUG -o#{t.name} -Iinclude #{t.prerequisites[0]}"
#end
cc "json_parser.o", "source/json/json_parser.cpp"

file "libjson.a" => [ "json_value.o", "json_parser.o" ] do |t|
  sh "ar -cru #{t.name} #{t.prerequisites.join(" ")}"
end

file "main.o" => [ "source/main.cpp", "include/json/json_value.h", "include/json/json_array.h", "include/json/json_object.h", "include/json/json_parser.h" ] do |t|
  sh "g++ -c -std=c++11 -Wall -O2 -o#{t.name} -Iinclude #{t.prerequisites[0]}"
end

file "main" => [ "libjson.a", "main.o" ] do
  sh "g++ -Wall -O2 -omain main.o -L. -ljson"
end

file "json_test.o" => "test/json_test.cpp" do
  sh "g++ -c -std=c++11 -ojson_test.o -Itest/catch -Iinclude test/json_test.cpp"
end

file "test" => [ "libjson.a", "json_test.o" ] do
  sh "g++ -Wall -O2 -ojson_test json_test.o -L. -ljson"
end

file "benchmark.o" => [ "benchmark.cpp" ] do |t|
  #sh "g++ -c -pg -std=c++11 -Wall -O2 -o#{t.name} -Iinclude #{t.prerequisites[0]}"
  sh "g++ -c -std=c++11 -Wall -O2 -DNDEBUG -o#{t.name} -Iinclude #{t.prerequisites[0]}"
end

file "bench" => [ "libjson.a", "benchmark.o" ] do
  #sh "g++ -pg -obench benchmark.o -L. -ljson"
  sh "g++ -obench benchmark.o -L. -ljson"
end
