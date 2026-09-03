# todo_11

 Version: 0.9.1

 date    : 2026/09/03
 
 update :

***

C++ Crow , TODO SQLite

***
### related

https://github.com/CrowCpp/Crow

https://crowcpp.org/master/

https://crowcpp.org/master/getting_started/setup/linux/

***
* LIB add
```
sudo apt-get install libsqlite3-dev
```

***
* build
```
g++ -std=c++17 main.cpp -o todo_app -lsqlite3 -lpthread

#start
./todo_app

```

***
* Test-code
* add
```
curl -X POST http://localhost:8080/api/todos \
  -H "Content-Type: application/json" \
  -d '{
    "title": "coffee",
    "content": "スーパーで食材を買う",
    "is_public": 1,
    "food_orange": 3,
    "food_apple": 5,
    "food_banana": 2,
    "pub_date": "2025-09-03",
    "qty1": 10,
    "qty2": 20,
    "qty3": 30
  }'

```

* list
```
curl http://localhost:8080/api/todos
```

* delete
```
curl -X DELETE http://localhost:8080/api/todos/1
```

***

