TODO write actual readme

TODO remark that whole codebase is only for combinatorial cells

Idea for speeding up HD comp:
- Use string-based evaluation only during parsing.
- Namely, go through all rows in truth table for each cell's functions
	E.g., A, B, C will give 8 rows
- These rows are evaluated using string evaluation
- The results are stored in vector<bool>
- The vector is used later on, during actual evaluation runs
- There, the vector index/adress is based on the input patterns
	E.g., A=0, B=1, C=0 is index/address 2
