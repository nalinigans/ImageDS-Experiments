clean:
	rm -f -r build/
	rm -f *.so
	rm -f test/*.so
	rm -f imageds.cpp

.PHONY: build
build:
	python3 setup.py build_ext --inplace

export DYLD_LIBRARY_PATH = /Library/Java/JavaVirtualMachines/jdk1.8.0_144.jdk/Contents/Home/jre/lib/server
export PYTHONPATH = $(shell pwd)

.PHONY: test
test: build
	python test.py

