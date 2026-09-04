# redis_2

 Version: 0.9.1

 date    : 2026/08/24
 
 update :

***

C++ Crow , redis example

* redis database
* LLVM CLang

***
### related

https://github.com/CrowCpp/Crow

https://crowcpp.org/master/

https://crowcpp.org/master/getting_started/setup/linux/

***
* LIB add
```
sudo apt update
sudo apt install libhiredis-dev
```

***
* build
```
clang++ -std=c++17 main.cpp -o server -lpthread -lhiredis

#start
./server

```

***
* Test-code
* add
```
curl -X POST -H "Content-Type: application/json" \
 -d '{"key": "k:1", "value":"hello" }' \
 http://localhost:8080/redis_add

```

***

