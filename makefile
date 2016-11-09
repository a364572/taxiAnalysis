main: main.o common_function.o point.o
	g++ main.o common_function.o point.o -o main -std=c++11

main.o: main.cpp
	g++ -c main.cpp -o main.o -std=c++11

common_function.o: common_function.cpp common_function.h Station.h
	g++ -c common_function.cpp -o common_function.o -std=c++11

point.o: Point.cpp Point.h common_function.h
	g++ -c Point.cpp -o point.o -std=c++11

clean:
	rm main.o main common_function.o point.o

