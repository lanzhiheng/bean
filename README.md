# A simple programming language

1. A programming language without `class` keyword.
2. An OOP(Object-oriented programming) language base on prototype.
3. The syntax will be similar to JavaScript.
4. Just `nil` and `false` will be the falsy value.
5. The variable name or function name can contain `?`.
6. The basic data type include `Number`, `String`, `Bool`, `Array`, `Hash`.
7. Stack-based virtual Machine.

# Dependencies

1. [bdwgc](https://github.com/ivmai/bdwgc), providing Garbage Collector for Bean, because I don't have enough time to develop it by myself.
2. [libcurl](https://curl.haxx.se/libcurl/), providing some convenience function to send all kinds of HTTP request.

*I add the compiled version of them in my codebase, so now Bean just can run in MacOS.*

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
