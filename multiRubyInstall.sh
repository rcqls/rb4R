#!/bin/bash

source /usr/local/opt/chruby/share/chruby/chruby.sh

for RUBY in 2.0 2.1 #rbx unfortunately fails from now since no static or shared library
do
	chruby $RUBY
	path=$(ruby -e "puts File.join(ENV[\"HOME\"],\"dyndoc\",\"R\",\"library\",RUBY_ENGINE,RUBY_VERSION)")
	mkdir -p $path
	R CMD INSTALL --preclean  ~/Github/rb4R -l $path
done

chruby system 
R CMD INSTALL --preclean  ~/Github/rb4R