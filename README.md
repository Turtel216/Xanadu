# Xanadu

#### A simple interpreted esoteric programming language running on the JVM and with syntax based on Rush song lyrics and titles

> [!WARNING]
> The language is still under development and will probably undergo several changes before release. Keywords are most likely to change in the future

## Syntax

### Variables

The **yyz** keyword is used for declaring variables.

```ruby
yyz name = "Xanadu"
```

### Writting to the console

To write to the console use the **blabla** keyword 

```ruby
blabla "Piece"
```

### Control flow

**while** loops are declared using the **Workingmans_grind** keyword 

```ruby
Workingmans_grind(true) {
   blabla "Rush rocks!"
}
```

**for** loops are declared using the **circumstances** keyword 

```ruby
circumstances()
```

**if** statements are declared using the **Freewill** keyword

```ruby
Freewill(1 == 1) {
    blabla "I will choose free will"
}
```

**else** statements are declaed using the **Choose_not_to_decide** keyword 

```ruby
Freewill(1 == 2) {
   blabla "I will choose free will"
} Choose_not_to_decide {
   blabla "You still made choice"
}
```

### Functions

Functions are declared using the **subdivision** keyword 

```ruby
subdivision add(a, b) {
   limelight a + b
}
```

The **limelight** keyword in the above example functions just as a **return** keyword in other languages

### Classes

Classes in Xanadu are declared using the **overtune** keyword

```ruby
overtune Band
{

}
```

## The Xanadu Interpreter

The Tree-walk interpreter is implemented in the C++ programming language and produces byte code for the [Java virtual machine](https://docs.oracle.com/en/java/javase/22/vm/java-virtual-machine-technology-overview.html). Both the [lexer](https://en.wikipedia.org/wiki/Lexical_analysis) and the [parser](https://www.techopedia.com/definition/3854/parser) are implemented from scratch, no third party dependencies were used while building this project.
