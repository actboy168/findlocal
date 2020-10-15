## findlocal

获取非当前函数的局部变量，需要这个局部变量是某个(已经实例化的)函数的上值。如果这个局部变量有多个实例，会将所有实例都返回。

``` lua
    local findlocal = require "findlocal"
    
    --文件test.lua中第一个名字是a的局部变量
    print(findlocal("@test.lua", "a"))
 
    --文件test.lua中第二个名字是a的局部变量
    print(findlocal("@test.lua", "a#2"))
  
    --文件test.lua中18行第一个局部变量
    print(findlocal("@test.lua", "18"))
    
    --文件test.lua中18行第二个局部变量
    print(findlocal("@test.lua", "18#2"))
   
    --文件test.lua中从18行查找名字为a的局部变量（作用域包含18行且定义在最后的局部变量）
    print(findlocal("@test.lua", "a", 18))
```
