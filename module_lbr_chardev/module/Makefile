#Authors: Hans Liljestrand and Shohreh Hosseinzadeh
#Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
#This code is released under Apache 2.0 and GPL 2.0 licenses.
obj-m += lbr_dumper.o

all: module

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
