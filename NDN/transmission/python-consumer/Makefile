_client.so: client.cxx client.hpp client.i
	swig -c++ -python client.i
	g++ -O2 -fPIC -c client.cxx
	g++ -O2 -fPIC -c client_wrap.cxx -I/usr/include/python2.7
	g++ -shared client.o client_wrap.o -lndn-cxx -o _client.so

clean:
	rm _client.so client.o client.py client.pyc client_wrap.cxx client_wrap.o
