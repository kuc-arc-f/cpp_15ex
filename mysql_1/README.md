# mysql_1

 Version: 0.9.1

 date    : 2026/09/08
 
 update :

***

C++ Crow web , API MySQL example 

* MySQL 8.4.0
* LLVM CLang

***
### Related - client

https://github.com/kuc-arc-f/cpp_15ex/tree/main/express_cl_5

***
### related

https://github.com/CrowCpp/Crow

https://crowcpp.org/master/

https://crowcpp.org/master/getting_started/setup/linux/

***
* LIB add
```
sudo apt-get update
sudo apt-get install libmysqlclient-dev
sudo apt install nlohmann-json3-dev
```

***
* env
```
export TODO_DB_HOST=127.0.0.1
export TODO_DB_PORT=3306
export TODO_DB_USER=root
export TODO_DB_PASS=admin
export TODO_DB_NAME=mydb
```

***
* Table: schema.sql

***
* build
```
clang++ -std=c++17 $(mysql_config --cflags) main.cpp -o todo $(mysql_config --libs)
```

***
* Test-code
* add
```
curl -X POST http://localhost:8080/todos \
  -H "Content-Type: application/json" \
  -d '{"title": "TEST-DATA-01"}'

```

* list

```
curl http://localhost:8080/todos
```

* delete
```
curl -X DELETE http://localhost:8080/todos/1
```

***
### blog

https://zenn.dev/knaka0209/scraps/d56e76f40c6cdd

***