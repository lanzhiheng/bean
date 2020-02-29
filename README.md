# A simple programming language

1. A programming language without `class` keyword.
2. An OOP(Object-oriented programming) language base on prototype.
3. The syntax will be similar to JavaScript.
4. Just `nil` and `false` will be the falsy value.
5. The variable name or function name can contain `?`.
6. The basic data type include `Number`, `String`, `Bool`, `Array`, `Hash`.
7. Stack-based virtual Machine.



## Regex Expression

For using the regex expression feature in bean, you should build the regex instance like this

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
