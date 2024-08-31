# Xanadu

#### A simple interpreted esoteric programming language with syntax based on Rush song lyrics and titles

> [!WARNING]
> The language is still under development and will probably undergo several changes before release. Keywords are most likely to change in the future

## Language Overview

Xanadu is a dynamically typed object oriented language with automatic memory management in the form of a garbage collector. Each statement has to end with a semicolone ( ; ).

### Comments

Comments in Xanadu start with the double forward slash ( // ).

```c 
// this is a comment 
```

### Declarations

The **yyz** keyword is used for declaring variables.

```ruby
yyz name = "Xanadu";
```

Functions are declared using the **subdivision** keyword 

```ruby
subdivision add(a, b) {
   limelight a + b
}
```

The **limelight** keyword in the above example functions just as a **return** keyword in other languages

The function can later be called like this

```ruby 
yyz x = add(1, 2); // Assigns value of 3 to variable x
```

### Writting to the console

To write to the console use the **blabla** keyword 

```ruby
blabla "Piece"; 
```

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

**if** statements are declared using the **freewill** keyword

```ruby
freewill(1 == 1) {
    blabla "I will choose free will";
}
```

**else** statements are declaed using the **counterpoint** keyword 

```ruby
freewill(1 == 2) {
   blabla "I will choose free will";
} counterpoint {
   blabla "You still made a choice";
} 
```

### Classes

Classes in Xanadu are declared using the **overtune** keyword

```ruby
overtune Band
{

}
```

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
 |   <     |   Less then    |  a < b  |
 |   >     |   Greater then   |  a > b  |
 |   <=    | Less then or equal to |  a <= b  |
 |   >=    |   Greater then or equal to    | a >= b  |
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

## Tooling

- Code editor with syntax highlighting: [Heaven's Door](https://github.com/Turtel216/Heavens-Door)

## Examples

You can find several examples covering the features of Xanadu in the [examples directory](https://github.com/Turtel216/Xanadu/tree/main/examples)
