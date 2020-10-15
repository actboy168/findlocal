## findlocal

获取非当前函数的局部变量，需要这个局部变量是某个(已经实例化的)函数的上值。

``` lua
    local findlocal = require "findlocal"
    print(findlocal("@test.lua", "a"))
```
