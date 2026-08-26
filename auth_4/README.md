# auth_4

 Version: 0.9.1

 date    : 2026/08/25
 
 update :

***

C++ Crow web , API auth example PostgreSQL Signup , Login

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

sudo apt-get install libsodium-dev

```
***
* DB set
```
export DATABASE_URL=postgresql://root:admin@localhost:5432/mydb
```

***
* Table: table.sql
***
* build
```
g++ -std=c++17 -I/usr/include/postgresql main.cpp -o user_api -lpqxx -lpthread -lsodium
```
***
* start
```
./user_api
```
***
* Test-code
* signup
```
curl -X POST http://localhost:8080/signup \
  -H "Content-Type: application/json" \
  -d '{"email":"you3@example.com","password":"1111"}'

```

* login
```
curl -X POST http://localhost:8080/login \
  -H "Content-Type: application/json" \
  -d '{"email":"you@example.com","password":"1111"}'
```

***

