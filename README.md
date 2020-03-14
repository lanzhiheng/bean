# A simple programming language

## Introduction

Bean is a Dynamic Programming Language which developed by myself at work off hours. The reason of why it called **Bean** is that the company which I am working for named [Beansmile](https://www.beansmile.com). That is the most primary reason. May be it will not be a good programming language in the world but it must be a meaningful language to me. Because it is the first programming language which I make.

I don't know too much about the principle of compiling, So I try my best to make it simple. Even Though it isn't means that Bean is a DSL--Domain Specific Language, no, it is a GPL--General Purpose Language, like Ruby, Python, JavaScript, Java, Go, etc.

There is a simple preview for Bean.

1. A programming language without `class` keyword.
2. An OOP(Object-oriented programming) language base on prototype.
3. The syntax will be similar to JavaScript.
4. Just `nil` and `false` will be the falsy value.
5. The variable name or function name can contain `?`.
6. The basic data type include `Number`, `String`, `Bool`, `Array`, `Hash`.
7. Stack-based virtual Machine.

And for display the inheritance relationship, I make a diagram.

![Image](https://via.placeholder.com/1000x600)

## Dependencies

- [libcurl](https://curl.haxx.se/libcurl/), providing some convenience function to send all kinds of HTTP request.

## Install Guide

Thanks for the help from [GUN](https://www.gnu.org/), we can easily install bean in our computer. Unfortunately I just test it in my Mac book and some Linux distribution, like Ubuntu.

1. Clone from github

```
git clone git@github.com:lanzhiheng/bean.git && cd ~/bean
```

2. Configure the project to generate makefile

```
./configure
```

3. Make it to start compiling

```
make
```

4. Run the bean

After that you will get the Bean binary file named `bean` in current directory. Just run it. I have design two mode for it. *Script-Mode* and *REPL*

Let's check *REPL* mode first

```
➜ ./bean
> a = "hello"
=> "hello"
> print(a)
hello
=> nil
> 1 + 22
=> 23
> (10 - 20) * 100
=> -1000
```

it is very convenience for checking the basic statement of Bean. But if you want to write multiline source code you should use *Script-Mode*.

Your first Bean script.

```
// first-script.bn
fn firstScript() {
  a = 100 + 90;
  print(a)
  "Hello World"
}
b = firstScript()

print("The value of variable b is " + b)
```

Run the script

```
➜ ./bean first-script.bn
190
The value of variable b is Hello World
```

# Number

## Number value

Not only in Bean, almost every programming language contains the number type. Similar to JavaScript, Bean store all the number value as float, even it has the integer schema. Let me so you some cases.

```
> a = 10
=> 10
> a = 1e3
1e3
=> 1000
> a = 11.1
=> 11.100000
> a = 11.1e2
=> 1110
```

Bean doesn't support the octal but hex. So you can define the number like below.

```
> 022
=> 22
> 018
=> 18
> 0x01
=> 1
> 0x10
=> 16
```

## Number formatting

We provide a few method to format the number. For example if you want to make a string which has fixed bits after the point. You can use `toFixed/1` method.

```
> a = 10.33333333
=> 10.333333
> a.toFixed(1)
=> "10.3"
>  a.toFixed(2)
=> "10.33"
> a.toFixed(100)
=> "10.3333333300000003163177098031155765056610107421875000000000000000000000000000000000000000000000000000"
```

If you don't pass paramater, number `2` will be the default value.

```
> a = 100
=> 100
> a.toFixed()
=> "100.00"
> a.toFixed('100')
The paramter which you passed must be a number.
> a.toFixed(-10)
=> "100.0000000000"
> a.toFixed(2.3)
=> "100.00"
```

Otherwse, As you can see in the example, above, The parameter which you are passing must be a number value. If you passed the positive float as the parameter we will convert it to integer as well as if you passed the negative one we will get the `abs` value of it.

You also can get the string which formatted by Scientific Notation, by using `toExponential/0`. it doesn't accept any parameters.

```
> a.toExponential(100)
=> "1.000000e+02"
> a.toExponential()
=> "1.000000e+02"
```

## Number operators

I think if you have learned JavaScript or any other programming language you will be familiar to `+`, `-`, `*`, `/`, etc. I think for explaining them, code will be better than my words.

```
> a = 10
=> 10
> b = 100
=> 100
> a + b
=> 110
> a - b
=> -90
> a * b
=> 1000
> a / b
=> 0.100000
```

Also support some bit operators.

```
> a = 10
=> 10
> b = 2
=> 2
> a | b
=> 10
> a & b
=> 2
> a % b
=> 0
> a ^ b
=> 0
```

Similar to javascript we also support `++`, `--` and something like `(*)=` which between two operands.

```
> a = 10
=> 10
> a++
=> 10
> a
=> 11
> b = 11
=> 11
> --b
=> 10
> b
=> 10
```

```
# (*)= schema
> a = 100
=> 100
> a += 10
=> 110
> a -= 10
=> 100
> a *= 10
=> 1000
> a /= 10
=> 100

> a = 2
=> 2
> a |= 10
=> 10
> a = 2
=> 2
> a &= 10
=> 2
> a = 2
=> 2
> a %= 10
=> 2
> a = 2
=> 2
> a ^= 10
=> 8
```

Sorry about that I don't have enough time to talk about them one by one.

# nil, true or false

Different to JavaScript we don't have too mush falsy. In Bean, We just have `nil` and `false` will be seen as false value. Others are truly.

```
> if (0) { print("I am true value") }
I am truly
=> nil
> if ("") { print("I am true value") }
I am truly
> if (true) { print("I am true value") }
I am true value
=> nil

=> nil
> if (false) { print("I am false value") }
=> nil
> if (nil) { print("I am false value") }
=> nil
```

Similar to Ruby, Just the `false` and `nil` are false value, the print branch with them will never run.

# String

## Basic

String must be a common data structure in modern programming language. When using Bean you can easy build a string literally.

```
> a = "I am a string"
=> "I am a string"
> b = "I am a string"
=> "I am a string"
> typeof a
=> "string"
> a == b
=> true
```

Easily, You can compare two strings by `==` operator. Otherwise you also can use the special method `equal/1` to compare two strings.

```
> a = "hello"
=> "hello"
> a.equal("hi")
=> false
> a.equal("hello")
=> true
```

The performance of `==` operator and `equal/1` method is equivalent.

Bean also provide some common method to handle the string instance. If you want to transform the case of string, `upcase/0`, `downcase/0`, `capitalize/0` are your choices.

```
> a = "ruby is a Good programming language"
=> "ruby is a Good programming language"
> a.upcase()
=> "RUBY IS A GOOD PROGRAMMING LANGUAGE"
> a.downcase()
=> "ruby is a good programming language"
> a.capitalize()
=> "Ruby is a Good programming language"
```

What if you want to concat two string? You can use `concat/1` method to do it.

```
> a = "Bean"
=> "Bean"
> a.concat(" Ruby")
=> "Bean Ruby"
> a.concat(1)
Please pass a valid string instance as parameter.
```

As you can see. `contact/1` just accepts string instance as parameter. If you want to concat others data type with a string, `+` operator may be a good choice.

```
> a = "Bean"
=> "Bean"
> a + " 1"
=> "Bean 1"
> 1 + 1
=> 2
> "1 " + a
=> "1 Bean"
```

You can see that if one side of the `+` operator is a string instance, other side's instance will auto convert to string, and they will be contacted later.

In addition, We can use `trim/0` method to clear the whitespace of the string both right side and left side.

```
>  "     Bean Language   ".trim()
=> "Bean Language"
```

## slice and split

What should you do if you want to get slice of a sting? `slice/2` can help you. That is a flexible method. It accepts 0~2 parameters.

if you don't pass any parameters, it will return the same string as target string.

```
> a = "hello"
=> "hello"
> a.slice() == a
=> true
```

if you just pass one parameter, it will set the other one equals to  the length of the string.

```
> a = "hello world"
=> "hello world"
> a.slice(0)
=> "hello world"
> a.slice(1)
=> "ello world"
> a.slice(2)
=> "llo world"
```

It is obvious that what will happen if you pass two parameters.

```
> a = "hello"
=> "hello"
> a.slice(0, 2)
=> "he"
> a.slice(3, 4)
=> "l"
```

Some tricks in `slice/2` is that you can pass negative number as the index of the string. I will handle them by plus it with the length of the sting. You also use `length` attribute to get the string's length.

```
> a = "hello world"
=> "hello world"
> a.slice(-3, -1)
=> "rld"
> a.slice(a.length - 3, a.length - 1)
=> "rl"
> a.slice(10, 4)
End index must greater than start index.
```

The last example will raise the exception, the end index must greater that start index.

The method similar to `slice/2` is `split/2`. I will you some code, I think you will know what it can do.

```
> a = "lan, zhi, heng"
=> "lan, zhi, heng"
> a.split(", ")
=> ["lan", "zhi", "heng"]
> a = "蓝，智，恒"
=> "蓝，智，恒"
> a.split("，")
=> ["蓝", "智", "恒"]
```

Split the string and store the result to an array.

## UTF-8 Support

Bean not only supports the ASCII characters, also can handle the string which contain UTF-8 Characters. I will make some string contain Chinese, they will tell you the work of `charAt/1`, `IndexOf/1`, `includes/1` and `codePoint/0`.

### chartAt

```
> a = "hello world"
=> "hello world"
> a.charAt(3)
=> "l"
> a = "你好，世界"
=> "你好，世界"
> a.charAt(2)
=> "，"
```

### indexOf

```
> a = "hello world"
=> "hello world"
>  a.indexOf(" ")
=> 5
> a = "你好，世界"
=> "你好，世界"
> a.indexOf("，")
=> 2
```

### includes

```
> a = "hello world"
=> "hello world"
>  a.includes(" ")
=> true
> a = "你好，世界"
=> "你好，世界"
> a.includes("ha")
=> false
```

### codePoint

```
> a = 'hello'
=> "hello"
> a.codePoint()
=> 104
> a = 'h'
=> "h"
> a.codePoint()
=> 104
> a = "你好"
=> "你好"
> a.codePoint()
=> 20320
> a = "你"
=> "你"
> a.codePoint()
=> 20320
```

At last, I will show you the performance of `length` attribute and `toNum` method.

```
> a  = "hello world"
=> "hello world"
> a.length
=> 11
> a = "蓝智恒"
=> "蓝智恒"
> a.length
=> 3
```

As you can see, the `length` attribute will return the count of UTF-8 characters to you not just the count of bytes.

```
> a = "hello"
=> "hello"
> a.toNum()
=> 0
> a = "212.333"
=> "212.333"
> a.toNum()
=> 212.333000
```

Finally, the `toNum` method is similar to Ruby's `Number#to_i` of `Number#to_f` methods. if the string can not convert to number they will return `0` to the caller.

# Array

## Basic methods

Let's talk about array data structure in Bean. As begining I will show you some basic method of array. They are `pop/0`, `push/1`, `unshift/1`, `shift/0`, `join/1`, `reverse/0` and `includes/1`, respectively.

When you see the name of the method, you can guess what their work. 

```
> a = [1, "a", "b", true]
=> [1, "a", "b", true]
> a.pop()
=> true
> a
=> [1, "a", "b"]
> a.pop()
=> "b"
> a
=> [1, "a"]
```

As you can see, `pop/0` method will pop the last item from the array. The fist one be poped is `true` value, the second is string `"b"`.

`push/1` method is opposite to `pop/0`, you can push items back like this.

```
> a
=> [1, "a"]
> a.push("b")
=> 3
> a.push(true)
=> 4
> a
=> [1, "a", "b", true]
```

Another thing you need to notice is when push successfully, it will return the size of current array.

`shift/0` and `unshift/1` are similar to `pop/0` and `push/1`. The difference is that they handle the item at the head of the array, not tail.

```
> a = [1, "a", "b", true]
=> [1, "a", "b", true]
> a.shift()
=> 1
> a.shift()
=> "a"
```

OK, at this case, the first item be "pop" is number `1`, the second is string `"a"`. Let's use `unshift/1` to "push" back them.

```
> a
=> [1, "a"]
> a.unshift("a")
=> 3
> a.unshift(1)
=> 4
> a
=> [1, "a", "b", true]
```

Otherwise, you can reverse the array by using `reverse/1` method.

```
> a = [1, 2, 3, 4, 6]
=> [1, 2, 3, 4, 6]
> a.reverse()
=> [6, 4, 3, 2, 1]
> a
=> [6, 4, 3, 2, 1]
```

It will change the original order of the array, and return it back. And if you want to check if array contains the expect item, `includes/1` is your choice.

```
> a = [1, "lan", "ruby"]
=> [1, "lan", "ruby"]
> a.includes(false)
=> false
> a.includes("lan")
=> true
```

Almost, there are all the basic of array. Oh, leak one, the `length` attribute.

```
> a
=> [1, "a", "b", true]
> a.length
=> 4
> b = [1,2]
=> [1, 2]
> b.length
=> 2
```

## Methods with callback

Bean also provides some functional methods which can accept callback. They are `map/1`, ``reduce/1`, `each/1`, `filter/1` and `find/1`.

The `map/1` method accept a callback function as parameter, and then use this call back method to handle all items in array. Finally base of each return value from function's calling it will build a new array.

```
> a= [1,2,3,4,5,6]
=> [1, 2, 3, 4, 5, 6]
> a.map(fn(x) { x * 2 })
=> [2, 4, 6, 8, 10, 12]
> a.map(fn(x, y) { x * y })
=> [0, 2, 6, 12, 20, 30]
```

You may notice two things. The first is that the keyword to define a function in Bean is `fn`. The second is that the first argument for the callback method is the item in array, the second is the index of the relevent item. This mechanism similar to JavaScript.

Next, Let's check `each/1` method. it is similar to `map/1`, the difference is that it will not build a new array. it will return the original one. You can use it to finish some complex logic if you do not want to build a new array.

```
> a= [1,2,3,4,5,6].reverse()
> a.each(fn(x, y) { print(y) })
0
1
2
3
4
5
=> [6, 5, 4, 3, 2, 1]
> a.each(fn(x, y) { print(x) })
6
5
4
3
2
1
=> [6, 5, 4, 3, 2, 1]
```

Next, Let's try to find item by `find/1` with condition from the result of the callback. If you want to find item which greater than number `3` in the array, you can use this code.

```
> a = [1,2,3,4,5,6]
=> [1, 2, 3, 4, 5, 6]
> a.find(fn (x, y) { x > 3 })
=> 4
```

But, `find/1` method just can return only one item which match the condition. If you want to get all of them, please use `filter/1`.

```
> a
=> [1, 2, 3, 4, 5, 6]
> a.filter(fn (x, y) { x > 3 })
=> [4, 5, 6]
```

That's all? No. we need to talk about the last one, `reduce/1` method. it will accept two parameter, first is callback, second is the initial value of the flow. what flow? if you want to add from 1 to 10, you can do it by `reduce/1`.

```
> a = [1,2,3,4,5,6,7,8,9,10]
=> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
> a.reduce(fn(a,b) { a + b }, 0)
=> 55
> a.reduce(fn(a,b) { print(b); a + b }, 0)
0
1
3
6
10
15
21
28
36
45
=> 55
```

You can see that the second parameter in callback will be the result of previous result of the callback. In the first round it will use the initial value.

No, You can simpily multiply from 1 to 10 by seting number `1` as the initial value.

```
> a = [1,2,3,4,5,6,7,8,9,10]
=> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
> a.reduce(fn(a,b) { a * b }, 1)
=> 3628800
```

# Math

Math is a special hash object, of cause the simplest one. It just contains some method, have not ability to create some new instances.

There are the method list for Math, after that I will show you some demo.

- `Math.ceil/1`
- `Math.floor/1`
- `Math.round/1`
- `Math.sin/1`
- `Math.cos/1`
- `Math.abs/1`
- `Math.sqrt/1`
- `Math.log/1`
- `Math.exp/1`
- `Math.random/1`
- `Math.min/1`
- `Math.max/1`
- `Math.pow/1`

and also `Math.PI` equal to `3.14159265358979323846`

There are some example for them.

```
> a = 11.1
=> 11.100000
> Math.ceil(a)
=> 12

> a = 11.9
=> 11.900000
> Math.floor(a)
=> 11

> a = 1000.3333
=> 1000.333300
> Math.round(a)
=> 1000

> a = 30
=> 30
> Math.sin(a * Math.PI / 180)
=> 0.500000
> Math.cos(a * 2 * Math.PI / 180)
=> 0.500000

> a = -10000
=> -10000
> Math.abs(a)
=> 10000

> a = 400
=> 400
> Math.sqrt(a)
=> 20

> a = Math.exp(1)
=> 2.718282
> Math.log(a)
=> 1

> a = Math.random()
=> 0.473279
> Math.round(Math.random() * 10 + 3)
=> 11

> a = 100
=> 100
> b = -100
=> -100
> Math.min(a,b)
=> -100
> Math.max(a,b)
=> 100
> Math.pow(a, 2)
=> 10000
```

OK, that all for Math library, so easy right?

# Hash

Hash data structure in Bean, is similar to the Object in JavaScript. For simplicity, it is the only data structure can be inherited.

Hash is a key-value system. so sometime you want to get all the key of it. We can use `keys/0` to get the result, the keys will be included in an array.

```
> a = {name: "lanzhiheng", age: "xx"}
=> {name: "lanzhiheng", age: "xx"}
> a.keys()
=> ["name", "age"]
```

If you what to create a new Hash which can call the methods from current Hash, just using the `clone/0` method.

```
> a = { hi: fn() { print(self) }, sayHello: fn() { print("Hello") }}
=> {sayHello: [Function sayHello], hi: [Function hi]}
> b = a.clone()
=> {}
> b.sayHello()
Hello
=> nil
> b.hi()
{}
=> nil
```

# Date

## 1. Basic

In general, most of programming language will include the date-handling library. So I decide to add some simple feature to Bean.

If we want to get current timestamp we can use `Date.now` like this.

```
> Date.now()
=> 1583666189
```

Otherwise, we can make an instance by using `Date.build`, then we will have a batch of methods for this instance.

```
> c = Date.build()
=> Sun Mar  8 19:26:59 2020
> c.getYear()
=> 2020
> c.getMonth()
=> 2
> c.getDate()
=> 8
> c.getHours()
=> 19
> c.getMinutes()
=> 26
> c.getSeconds()
=> 59
> c.getWeekDay()
=> 0
```

## 2. Parser

I provide a method named `Date.parse` to parse the time from time string. By default the formatter for parsing is `%Y-%m-%d %H:%M:%S %z`. So you can got the date instance like this

```
> c = Date.parse("2020-3-30 10:30:00 +0800")
=> Mon Mar 30 10:30:00 2020
```

The time of printed string is equal to the params which in `Date.parse` method. Because I run this script in china, the timezone for my computer is `Asia/Shanghai`. If you wat to parse an UTC time. you can pass the parameter like this

```
> c = Date.parse("2020-3-30 10:30:00 +0000")
=> Mon Mar 30 18:30:00 2020
```

Now, the parser think the string as UTC time. If you want to display it in Chinese timezone, it will become `18:30:00`.

Also, you can define the formatter by your self. You can customize it by passing second parameter to `Date.parse`. Let't see the example, below.

```
> c = Date.parse("10:30:00++++2020-3-30 +0000", "%H:%M:%S++++%Y-%m-%d %z")
=> Mon Mar 30 18:30:00 2020
```

OK, as you can see the result is right with the new formatter and new string which matched to it.

## 3. Stringify

You can use instance method `format` to construct the Date string by the existing date instance. for example

```
> f =  Date.parse("2020-3-30 10:30:00 +0000", "%Y-%m-%d %H:%M:%S %z")
=> Mon Mar 30 18:30:00 2020
> f.format()
=> "2020-03-30 18:30:00 +0800"
```

It is obvious that we parse the date string as the UTC time. If you format it later, you will get the result which tranformed by your current Timezone(Asia/Shanghai for me).

You also can specify the format string by passing a parameter to `format` method.

```
> f.format("%Y-%m-%d")
=> "2020-03-30"
> f.format("%H:%M:%S")
=> "18:30:00"
```

But the result of them will depended on your current Timezone. What should I do if I want to specify the timezone information when formatting the date instance? Just pass the timezone info as second parameter to `format` method.

```
> h = Date.parse("2020-3-30 10:30:00 +0800", "%Y-%m-%d %H:%M:%S %z")
=> Mon Mar 30 10:30:00 2020
> h.format("%Y-%m-%d %H:%M:%S %z", "Asia/Shanghai")
=> "2020-03-30 10:30:00 +0800"
>  h.format("%Y-%m-%d %H:%M:%S %z", "UTC")
=> "2020-03-30 02:30:00 +0000"
> h.format("%Y-%m-%d %H:%M:%S %z", "America/Toronto")
=> "2020-03-29 22:30:00 -0400"
```

The timezone which you pass to the method must match the [tzfile](https://linux.die.net/man/5/tzfile) names. If you pass an invalid timezone info, system will treat it as UTC.

```
>  h.format("%Y-%m-%d %H:%M:%S %z", "UTC")
=> "2020-03-30 02:30:00 +0000"
>  h.format("%Y-%m-%d %H:%M:%S %z", "Invalid")
=> "2020-03-30 02:30:00 +0000"
```

OK, I think that all about the date library for bean language. If I have some new idea, I will extend it.

# Regular Expression

## Simple demo

For using the regular expression feature in bean, you should build the regex instance like this

```
> regex = Regex.build('aa+')
=> /aa+/
> regex
=> /aa+/
> regex.test('aaa')
=> true
> regex.test('AAA')
=> false
```

You also can choose your favor mode. For example, you can specify the `ignore case` mode like this, the result of test with the string **"AAA"** will become `true`

```
> regex = Regex.build('aa+', 'i')
=> /aa+/
> regex.test('aaa')
=> true
> regex.test('AAA')
=> true
```

## Syntax sugar for building

So simple, right? For convenience if you don't need to set the mode, you can build the regex expression by **``** syntax sugar.

```
> `aaa+`
=> /aaa+/
```

## Subexpression

Some time we want to handle subexpression in regular expression. we just need to involve the `exec` method, it will return an array to us.

```
> c = Regex.build('(Hello) (Ruby)')
=> /(Hello) (Ruby)/
> c.exec("Hello Ruby, You are the best.")
=> ["Hello Ruby", "Hello", "Ruby"]
```

Otherwise, If the regular expression can not match any part of the string, we will get an empty array.

```
> c = Regex.build('(Hello) (Ruby)')
=> /(Hello) (Ruby)/
> c.exec("Hello Python, You are the best.")
=> []
```

## Shorthand Character Classes

Similar to javascript we can support the Shorthand Character Classes now. for example, `\s` for space characters, `\w` for a-z, A-Z, 0-9, _ characters and `\d` for 0-9 characters. You can define a regex expression like this. Also supported the opposite of them, `\W`, `\D` and `\S`.

```
> c = Regex.build('\w+\s\d+\sRuby')
=> /\w+\s\d+\sRuby/
> c.test('Hello 1024 Ruby')
=> true

> c = Regex.build('\D+\W\S+')
=> /\D+\W\S+/
> c.test('Hello Ruby')
=> true
```

# HTTP Request

## Get Request

You also can use bean to send requests to servers. Let's begin with the simply example. I you want to download the page content from my website, you just need to send the `GET` request like this

```
> HTTP.fetch("http://www.lanzhiheng.com", { method: "get" })
=> "<!DOCTYPE html>\n<html>\n  <head>\n    <title>Step By Step</title>\n    <meta name=\"csrf-param\" content=\"authenticity_token\" />\n<meta name=\"csrf-token\" content=\"m0yy3sD7cWur0E3SmV0ZpufoGGhCInyrfoRED3m74v3B6YbEhXKWWOTeNWyBQjxECS514Oy1Lx35uczz0rIo2Q==\" />\n......
```

The default method of `fetch` method is `GET`, so you can omit it.

```
> HTTP.fetch("http://www.lanzhiheng.com")
=> "<!DOCTYPE html>\n<html>\n  <head>\n    <title>Step By Step</title>\n    <meta name=\"csrf-param\" content=\"authenticity_token\" />\n<meta name=\"csrf-token\" content=\"m0yy3sD7cWur0E3SmV0ZpufoGGhCInyrfoRED3m74v3B6YbEhXKWWOTeNWyBQjxECS514Oy1Lx35uczz0rIo2Q==\" />\n......
```

## POST Request

Basically, there are two format for post data. `application/x-www-form-urlencoded`and`multipart/form-data`。 You can special the `Content-Type` header to config them.

If you want to send a POST request with `application/x-www-form-urlencoded` data, you can do like this

```
HTTP.fetch("http://127.0.0.1:4000/hello", {
  method: "POST",
  data: {
    name: "lan",
    email: "lanzhihengrj@gmail.com",
    content: "Hello World",
    params: {
     age: 1,
     age: '100'
    }
  },
  header: {
    'Content-Type': 'application/x-www-form-urlencoded'
  }
})
```

You also can omit the header configuration in this case, because the `application/x-www-form-urlencoded` is the default value of `Content-Type` when sending the POST request.

In this case the data will transform to string like *content=Hello World&params={"age": "100"}&name=lan&email=lanzhihengrj@gmail.com*. If you don't like this mode you can use another `Content-Type` value, it is `multipart/form-data`.Just simply set it like the code below.

```
HTTP.fetch("http://127.0.0.1:4000/hello", {
  method: "POST",
  data: {
    name: "lan",
    email: "lanzhihengrj@gmail.com",
    content: "Hello World",
    params: {
     age: 1,
     age: '100'
    }
  },
  header: {
    'Content-Type': 'multipart/form-data'
  }
})
```

That all for POST request, not very diffcult, right?

*PS: For simplicity the HTTP request for Bean doesn't support the file uploading now, Because I am so busy these time. Sorry about that if I have enough time I will add the file uploading feature later. Not Only for POST request but also PUT request.*

## PUT Request

The PUT Request is very similar to POST Request. But for better handle the PUT data, we better set the `Content-Type` to `application/json` explicitly. So you need to need to send the request like this

```
putResult = HTTP.fetch("http://127.0.0.1:4000/contact-me/100", {
  method: "PUT",
  data: {
    name: "lan",
    email: "lanzhihengrj@gmail.com",
    content: "Hello World",
    params: {
      p1: 1,
      p2: "hello"
    }
  },
  header: {
    'content-type': 'application/json'
  }
})

print(putResult) // => The result of PUT request
```

## DELETE Request

If you what to send the DELETE, you just need to set the request method to `DELETE`

```
> HTTP.fetch("http://www.lanzhiheng.com", { method: "delete" })
=> "<html>\r\n<head><title>301 Moved Permanently</title></head>\r\n<body bgcolor=\"white\">\r\n<center><h1>301 Moved Permanently</h1></center>\r\n<hr><center>nginx/1.14.0 (Ubuntu)</center>\r\n</body>\r\n</html>\r\n"
```
