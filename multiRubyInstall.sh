#!/bin/bash

source /usr/local/opt/chruby/share/chruby/chruby.sh

for RUBY in 2.0.0-p353 2.0.0-p481 2.0.0-p576 2.1.3 #rbx unfortunately fails from now since no static or shared library
do
	chruby $RUBY
	path=$(ruby -e "puts File.join(ENV[\"HOME\"],\"dyndoc\",\"R\",\"library\",RUBY_ENGINE,RbConfig::CONFIG[\"exec_prefix\"].split(File::Separator)[-1])")
	mkdir -p $path
	R CMD INSTALL --preclean  ~/Github/rb4R -l $path
done

chruby system 
R CMD INSTALL --preclean  ~/Github/rb4R