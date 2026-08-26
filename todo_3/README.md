# todo_3

 Version: 0.9.1

 date    : 2026/08/25
 
 update :

***

C++ Crow web , API PostgreSQL TODO example 

***
### related

https://github.com/CrowCpp/Crow

https://crowcpp.org/master/

https://crowcpp.org/master/getting_started/setup/linux/

***
* LIB add
```
sudo apt update
sudo apt install -y build-essential cmake libboost-system-dev \
  libpqxx-dev postgresql-client pkg-config
```

***
* DB set
```
export DATABASE_URL=postgresql://root:admin@localhost:5432/mydb
```

***
* Table: schema.sql

***
* build
```
g++ -std=c++17 -I/usr/include/postgresql main.cpp -o user_api -lpqxx -lpthread
```
***
* start
```
./user_api
```
***
* Test-code
* add
```
curl -X POST http://localhost:8080/todos \
  -H "Content-Type: application/json" \
  -d '{"title": "TEST-DATA-1"}'

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

