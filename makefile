main: main.o common_function.o point.o taxiTask.o pthreadPool.o
	g++ main.o common_function.o point.o taxiTask.o pthreadPool.o -o main -std=c++11 -lpthread

main.o: main.cpp
	g++ -c main.cpp -o main.o -std=c++11

common_function.o: common_function.cpp common_function.h Station.h PthreadPool.h TaxiTask.h
#common_function.o: common_function.cpp
	g++ -c common_function.cpp -o common_function.o -std=c++11

point.o: Point.cpp Point.h
	g++ -c Point.cpp -o point.o -std=c++11

taxiTask.o: TaxiTask.cpp TaxiTask.h
	g++ -c TaxiTask.cpp -o taxiTask.o -std=c++11

pthreadPool.o: PthreadPool.cpp PthreadPool.h
	g++ -c PthreadPool.cpp -o pthreadPool.o -std=c++11

clean:
	rm main.o main common_function.o point.o pthreadPool.o taxiTask.o

