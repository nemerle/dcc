#!/usr/bin/env ruby
require 'fileutils'
print("Regression tester 0.0.1\n")
def path_local(from)
    
    return from #from.gsub('/','//')
    from.gsub('/','\\\\')
end
TESTS_DIR="./tests"
def perform_test(exepath,filepath,outname)
	output_path=path_local(TESTS_DIR+"/outputs/"+outname)
	exepath=path_local(exepath)
	output_path=path_local(output_path)
	filepath=path_local(filepath)
	printf("calling:" + "#{exepath} -a1 -o#{output_path}.a1 #{filepath}\n")
	result = `#{exepath} -a1 -o#{output_path}.a1 #{filepath}`
	result = `#{exepath} -a2msc -o#{output_path}.a2 #{filepath}`
	puts result
	p $?
end
`rm -rf #{TESTS_DIR}/outputs/*.*`
#exit(1)
Dir.open(TESTS_DIR+"/inputs").each() {|f|
	next if f=="." or f==".."
	perform_test(".//"+ARGV[0],TESTS_DIR+"/inputs/"+f,f)
}
Dir.open(TESTS_DIR+"/inputs").each() {|f|
	next if f=="." or f==".."
	FileUtils.mv(TESTS_DIR+"/inputs/"+f,TESTS_DIR+"/outputs/"+f) if f.end_with?(".b")
}
"diff -rqbwB"