# HTTP Request

> Because the compatibility and dependences will cause some issue, I decide to remove the HTTP module temporarily.

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
var putResult = HTTP.fetch("http://127.0.0.1:4000/contact-me/100", {
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
