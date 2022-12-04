### comments handling

We use variable "comment_level" to refer to the level of comments. When the scanner encounters "/*", the start condition will be switched to COMMENT and add "comment_level". 

In COMMENT condition, each "/*" will add the value of comment_level and each "\*/" will decrease the value of comment_level. If the value of it reaches 1 which is the initial value, the start condition will be switched to INITIAL and this comment pattern ends.

### string handling

We use variable string_buffer_ to restore the string being reading. When the scanner encounters "\"", the start condition will be switched to STR.

In STR, when another "\"" is recognized, this string ends and we call member function setMatched(string_buffer_) to output the string and the start condition will be switched to INITIAL.

In STR we have to  transfer some escape sequences into the formal format in a string:

- "\n" "\t" "\\\"" "\\\\"	 kept the same format in a string
- "\^c" 					remain the letter
-  "\ddd"                  call stoi() to transfer it to an integer
- \\[ \n\t\v]+\             all of the content in this will be ignored 

### error handling

When a string can't match any pattern defined above, we just output it and return an error token.

### end-of-file handling

<<EOF>> can recognize the end of file. We just return 0 to imply the end of the file and tell the scanner to stop.