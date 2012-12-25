all:
	#g++ -DDEBUG -g generate_index_new.cpp -I/usr/include/mysql -lmysqlclient -lmpkgsupport -o generate_index_new
	g++ -O3 -ffast-math -fomit-frame-pointer -march=native -fPIC generate_index_new.cpp -I/usr/include/mysql -lmysqlclient -lmpkgsupport -o generate_index_new
	strip generate_index_new

