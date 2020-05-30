votebm : votebm-schema.cc votebm.cc
	c++ -O -std=c++17 -o votebm votebm.cc

votebm-schema.cc : pre votebm-schema.txt
	./pre < votebm-schema.txt > votebm-schema.cc

pre : pre.cc
	c++ -g -std=c++17 -o pre pre.cc

clean :
	rm -f pre votebm votebm-schema.cc
