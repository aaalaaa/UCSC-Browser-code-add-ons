
all::   97way.commonNames.nh 97way.scientificNames.nh \
	96way.commonNames.nh 96way.scientificNames.nh

97way.commonNames.nh: 97way.nh
	./commonNames.sh $< | ./asciiTree.pl /dev/stdin > $@

96way.commonNames.nh: 96way.nh
	./commonNames.sh $< | ./asciiTree.pl /dev/stdin > $@

97way.scientificNames.nh: 97way.nh
	./scientificNames.sh $< | ./asciiTree.pl /dev/stdin > $@

96way.scientificNames.nh: 96way.nh
	./scientificNames.sh $< | ./asciiTree.pl /dev/stdin > $@
