# WTOP (kvadraOS команда 2)

вебграфический top с поддержкой графиков

# Быстрый тест без компиляции
    http://202.71.13.178:8080

# Изображения
![alt text](image.png)
![alt text](image-1.png)

вывод сразу всей доступной информации даже при перезаходе в браузер (за ограниченный промежуток пока)

# dependences
## динамические для самого сервера
    linux-vdso.so.1
    libstdc++.so.6
    libm.so.6
    libgcc_s.so.1
    libc.so.6
## для компиляции ts
    typescript > 4.9.5

# build

```
cd kvadraOS_2
// npm install typescript (если ещё нет)
make webui
```

