## **Text Editor**
some functions are pretty self explanatory

#### termios.h
`orig_termios` stores the original terminal data and stuff
`tcset`,`tcget` sets or gets the attributes of the terminal
`c_xflags`: here x is i,l,c which are input,local (bit related),control and output flags which are used to control those aspects 

#### esape sequences
`\x1b[2J` tells the terminal to clear the screen
`\x1b[H` puts the cursor on home or top left
`\x1b[6n` 4 byte and responds the (x,y ) coords of the cursor
`\x1b[999C\x1b[999B` basically moves the cursor to the extreme right and bottom

``` appendbuf ``` is the buffer used to append the data into the terminal or the stdout

``` editorscroll ``` works by having an offset valuesuch that it is added to the y posn to calculate the row on which the cursor is
<!-- todo complete the docs plsss -->


