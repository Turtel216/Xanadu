# Xanadu

#### A simple interpreted esoteric programming language with syntax based on Rush song lyrics and titles

### Table of Contents
- [Language Overview](#overview)
    * [Comments](#comments)
    * [Declarations](#declarations)
    * [Control Flow](#control)
    * [Wirtting to the console](#console)
    * [Classes](#classes)
    * [Inheritance](#inheritance)
    * [Operators](#operators)
- [Tooling](#tooling)
- [Examples](#examples)

<a name="overview"/>

## Language Overview

Xanadu is a dynamically typed object oriented language with automatic memory management in the form of a garbage collector. Each statement has to end with a semicolon( ; ).

<a name="comments"/>

### Comments

Comments in Xanadu start with the double forward slash ( // ).

```c 
// this is a comment 
```

<a name="declarations"/>

### Declarations

The **yyz** keyword is used for declaring variables, similar to var, let, or const in Javascript.

```ruby
yyz name = "Xanadu";
```

Functions are declared using the **subdivision** keyword 

```ruby
subdivision add(a, b) {
   limelight a + b
}
```

The **limelight** keyword functions similarly to **return**in other languages, terminating the function and returning a value.

The function can later be called like this

```ruby 
yyz x = add(1, 2); // Assigns value of 3 to variable x
```

<a name="console"/>

### Writting to the console

To write to the console, use the **blabla** keyword, akin to print() or console.log() in other languages.

```ruby
blabla "Piece"; 
```

<a name="control"/>

### Control flow

**while** loops are declared using the **workingmans_grind** keyword 

```ruby
workingmans_grind(true) {
   blabla "Rush rocks!";
}
```

**for** loops are declared using the **circumstances** keyword 

```ruby 
circumstances(yyz i = 0; i < 10; i = i + 1) {
    blabla i;
} 
```

**freewill** acts like an if statement, executing the block if the condition evaluates to true.

```ruby
freewill(1 == 1) {
    blabla "I will choose free will";
}
```

Similar to else, **counterpoint** executes the block if the previous freewill condition was false.

```ruby
freewill(1 == 2) {
   blabla "I will choose free will";
} counterpoint {
   blabla "You still made a choice";
} 
```

<a name="classes"/>

### Classes

Classes in Xanadu are declared using the **overtune** keyword

```ruby
overtune Band
{
    printName(name) {
        blabla name;
    }
}
```

You can intialize the above class and use its method like so:

```ruby
yyz band = Band();
band.printName("Rush"); // Prints 'Rush' to the console
```

You can also use an initializer method (init) to set up class instances, similar to a constructor in other object-oriented languages like Python or Java.

```ruby
overtune Band
{
    init(name) {
        todays.name = name;
    }

    printName() {
        blabla todays.name;
    }
}

yyz band = Band("Rush");
band.printName(); // prints 'Rush' to the console
```

The keyword **todays** acts as a 'this' keyword in other languages and basically refers to the current class instance.

<a name="inheritance"/>

### Inheritance

A subclass in Xanadu can inherit from a parent class using the colon (:) symbol, similar to other object-oriented languages like Python. The syrinx keyword refers to the parent class's methods and properties, akin to super in Java or Python.

```ruby
overtune Band
{
    init() {}

    playSong(bandName, songName) {
        blabla bandName + " Plays " + songName;
    }
}

overtune Rush : Band 
{
    init(name) {
        todays.name = name;
    }

    play(song) {
        syrinx.playSong(todays.name, song);
    }
}

yyz rush = Rush("Rush");
rush.play("2112");
```

<a name="operators"/>

### Operators

#### Arithmetic Operators 

 | Symbol   | Operator  | Syntax |
 | :---:    |  :---:    | :---:  |
 |   +      |  Plus     | a + b  |
 |   -      |  Minus    | a - b  |
 |   *      | Multiply  | a * b  |
 |   /      |  Divide    | a / b  |

#### Relational Operators

 | Symbol  |  Operator  | Syntax |
 | :---:   |   :---:    | :---:  |
 |   <     |   Less than    |  a < b  |
 |   >     |   Greater than   |  a > b  |
 |   <=    | Less than or equal to |  a <= b  |
 |   >=    |   Greater than or equal to    | a >= b  |
 |   ==    |   Equal to   |  a == b  |
 |   !=    |   Not equal to    | a != b  |

#### Logical Operators

 | Symbol  |  Operator  | Syntax |
 | :---:   |   :---:    | :---:  |
 |   and      |  Logical AND     | a and b  |
 |   or      |  Logical OR   |  a or b |
 |   !      | Logical NOT |  !a  |

#### Assignment Operators

 | Symbol   | Operator |  Syntax |
 | :---:    |  :---:   |  :---:  |
 |   =      |  Simple Assignment  | a = b  |

<a name="tooling"/>

## Tooling

- For a better development experience, Xanadu supports the [Heaven's Door](https://github.com/Turtel216/Heavens-Door) editor, which provides syntax highlighting specifically for the language.

<a name="examples"/>

## Examples

You can find several examples covering the features of Xanadu in the [examples directory](https://github.com/Turtel216/Xanadu/tree/main/examples)
