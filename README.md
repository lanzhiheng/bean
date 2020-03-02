# A simple programming language

1. A programming language without `class` keyword.
2. An OOP(Object-oriented programming) language base on prototype.
3. The syntax will be similar to JavaScript.
4. Just `nil` and `false` will be the falsy value.
5. The variable name or function name can contain `?`.
6. The basic data type include `Number`, `String`, `Bool`, `Array`, `Hash`.
7. Stack-based virtual Machine.



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
