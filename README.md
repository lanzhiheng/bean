# A simple programming language

1. A programming language without `class` keyword.
2. An OOP(Object-oriented programming) language base on prototype.
3. The syntax will be similar to JavaScript.
4. Just `nil` and `false` will be the falsy value.
5. The variable name or function name can contain `?`.
6. The basic data type include `Number`, `String`, `Bool`, `Array`, `Hash`.
7. Stack-based virtual Machine.

## Date

### 1. Basic

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

### 2. Parser

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

## Regular Expression

### Simple demo

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

### Syntax sugar for building

So simple, right? For convenience if you don't need to set the mode, you can build the regex expression by **``** syntax sugar.

```
> `aaa+`
=> /aaa+/
```

### Subexpression

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

### Shorthand Character Classes

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

## HTTP Request

### Get Request

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

### POST Request

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

### PUT Request

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

### DELETE Request

If you what to send the DELETE, you just need to set the request method to `DELETE`

```
> HTTP.fetch("http://www.lanzhiheng.com", { method: "delete" })
=> "<html>\r\n<head><title>301 Moved Permanently</title></head>\r\n<body bgcolor=\"white\">\r\n<center><h1>301 Moved Permanently</h1></center>\r\n<hr><center>nginx/1.14.0 (Ubuntu)</center>\r\n</body>\r\n</html>\r\n"
```
