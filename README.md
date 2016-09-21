# slush
Here are the answers to the lab questions:

1. Taylor Watson and Christopher Lucas

2. We had two primary bugs which may affect use of slush:

a) Piping "ps aux" into another program tends to hang, though the alternative "ps ax" works fine.
b) A syntax error such as "more (( ls" will be evaluated simply as "more ( ls". An error such as "less ( ( ls"
   will simply pipe the string "Invalid null command" into ls instead of breaking the recursion properly.
c) There is a warning on compilation which points to our use of "get_current_dir_name()", disliking our cast.

3. We succeeded at including the extra credit path exercise.
