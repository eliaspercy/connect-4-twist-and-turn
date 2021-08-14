#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "connect4.h"

void search_for_winner(board u);

// Structure containing the contents of the board and its dimensions
struct board_structure {
	int rows;
	int cols;
	char* grid; // String, or 1-D array, containing the content of the grid
};

// Initialise a board instance
board setup_board() 
{
	board new_board = NULL;
	new_board = malloc(sizeof(*new_board));
	if (new_board == NULL) {
		fprintf(stderr, "[ERROR] Allocating memory for the board structure failed; program halting.");
		exit(1);
	}	
	new_board->rows = new_board->cols = -1; // initialise as -1
	new_board->grid = NULL;
	return new_board;
}

// Free all memory allocated the the board and the board's grid.
void cleanup_board(board u) 
{
	free(u->grid);
	u->grid = NULL;
	free(u);
	u = NULL;
}

// Reads in the board from a file, initialises the game and ensures the board is valid.
void read_in_file(FILE *infile, board u) 
{
	int cols_count, rows_count;
	int i;
	char c;

	// Ensure the infile is read correctly, ie there is a board file
	if (infile == NULL) {
		fprintf(stderr, "[ERROR] Reading the file has failed; program halting.\n");
		exit(1);
	}
	// Checks if the board is uninitilised (indicated by the rows being equal to -1); if so, then find the dimensions of the board (and ensure that it is valid)
	else if (u->rows == -1 && u->cols == -1) {
		cols_count = rows_count = 0; 
		do {                      
			c = fgetc(infile);
			switch(c) {
			case '\r': // Ignore Windows' annoying newline thingy
				continue;
			case '\n': // If the first newline has been reached, set the number of columns (and cease counting)
				if (cols_count != 0) { // ignore blank lines
					++rows_count;
					// If the number of columns hasn't yet been set, set this
					if (u->cols == -1) { u->cols = cols_count; }
					else if (cols_count != u->cols) {// If the number of columns has already been set and a new line contains a number of elements not equal to the already set number of columns, then this is invalid
						fprintf(stderr, "[ERROR] The input file contains an invalid row structure; program halting.\n");
						exit(1);
					}
					cols_count = 0; // reset the column counter
				}
				break;
			case EOF:  // Otherwise, if the new char is the EOF signal, then simply increment the row count once more and assign this now to the number of rows variable - the do-while loop will cease after this
				if (rows_count == 0) { // Handle for cases when there is only one row
					u->rows = ++rows_count;
					u->cols = cols_count;
				}
				else if (cols_count != 0) { // Handle for cases when the final line is blank
					u->rows = ++rows_count;
				}
				else { // Handle for "default" cases
					u->rows = rows_count;
					cols_count = u->cols;   
				}   
				// Error check:
				if (u->rows < 1) {
					fprintf(stderr, "[ERROR] The input file contains an insufficient number of rows; program halting.\n");
					exit(1);
				}
				else if (u->cols < 4 || cols_count != u->cols) {
					fprintf(stderr, "[ERROR] The input file contains an insufficient number of columns; program halting.\n");
					exit(1);
				}
				break;
			default:
				if (c != '.' && c != 'x' && c != 'o') {// Ensure that the character being read is, if not any of the triggering ones above, a valid one:
					fprintf(stderr, "[ERROR] The input file contains invalid characters; program halting.\n");
					exit(1);
				}
				// If none of the above are triggered, continue to increment the column count and ensure this doesn't exceed 512
				else if (++cols_count > 512) {// Since the column counter is incremented once more when a newline is reached, if it is equal to 512 here then it will certainly exceed it on the next loop
					fprintf(stderr, "[ERROR] The input file contains too many columns; program halting.\n");
					exit(1);
				}
				break;
			}
		}         
		while (c != EOF); // Once the end of the file has been reached, stop looping        

		// Dynamically allocate memory to the grid, which will be in the form of a one dimensional array, or string
		if (u->rows*u->cols < 8) {
			fprintf(stderr, "[ERROR] The capacity of the grid is too small for anybody to win; program halting.\n");
			exit(1);
		}
		u->grid = malloc((sizeof(char)*u->rows*u->cols) + sizeof('\0'));  // 1-D array of chars
		if (u->grid == NULL) {
			fprintf(stderr, "[ERROR] Allocating memory for u->grid failed; program halting.\n");
			exit(1);
		}
		u->grid[u->rows*u->cols] = '\0';

		// Fill the board's grid and check if it either contains any tokens or contains no blanks (ie is full)
		rewind(infile);
		i = 0;
		int is_full = 1;
		int contains_tokens = 0;
		while ((c = fgetc(infile)) != EOF) {
			if (is_full && c == '.') { is_full = 0; }
			if (!contains_tokens && (c == 'x' || c == 'o')) { contains_tokens = 1; }
			if (c != '\n') { u->grid[i++] = c; }
		}

		// If the board contains tokens, check gravity has worked, and tokens are valid
		if (contains_tokens) {
			int threshold = (u->rows*u->cols) - 1 - u->cols;
			for (i = 0; i < u->rows*u->cols; i++) {
				if (u->grid[i] != '.') {
					if (i <= threshold) {
						if (u->grid[i + u->cols] == '.') {
							fprintf(stderr, "[ERROR] The input file contains floating tokens, remedy this and retry! Program halting.\n");
							exit(1);
						}
					}
					if (u->grid[i] == 'x' && u->grid[i] == 'o') {
						fprintf(stderr, "[ERROR] Something went wrong after the grid values were assigned; program halting\n");
						exit(1);
					}
				}
			}
			next_player(u); // Check that the input board has a valid x:o ration
			search_for_winner(u); // Finally, if the board contains tokens then check if the layout is a winning one (will also check if the board is full)
		} 
	}
	else {
		fprintf(stderr, "[ERROR] The board structure has already been initialised, this should not be the case!\n");
		exit(1);
	}
}

// Output the current status of the board; with any winning tokens capitalised.
void write_out_file(FILE *outfile, board u)
{ 
	int i, j;
	int index;
	for (i=0; i < u->rows; i++) {
		for (j=0; j < u->cols; j++) {
			index = i*u->cols + j;
	   		fprintf(outfile, "%c", u->grid[index]);
		}
		fprintf(outfile, "\n");
	}
}

// Scan the board to check who the next player is (also ensure the board contains a valid number of each x and o)
char next_player(board u) 
{ 
	/*
		1. Scan the board, incrementing the x_count variable whenever an 'x' is encountered and decrementing it whenever an 'o' is encountered
		2. If x_count is less than 0, then there are too many 'x's on the grid, which implies the board configuration is invalid
		3. Similarly, if x_count is greater than 1, then there are too many 'o's on the board, also implying the configuration is invalid
		4. Otherwise, if x_count == 0, return 'x', or if x_count == 1, return 'o'.
	*/
	int x_count;
	int i;
	x_count = 0;
	for (i=0; i<(u->cols*u->rows); i++) {
		if (u->grid[i] == 'x') { x_count++; }
		if (u->grid[i] == 'o') { x_count--; }
	}
	if (x_count < 0) {
		fprintf(stderr, "[ERROR] Something went wrong when obtaining the next player: there are too many 'o' tokens on the board; program halting.\n");
		exit(1);
	}
	else if (x_count > 1) {
		fprintf(stderr, "[ERROR] Something went wrong when obtaining the next player: there are too many 'x' tokens on the board; program halting.\n");
		exit(1);
	}
	return x_count ? 'o' : 'x';
}

// Check the board to see if anybody has won, or if it's a draw.
char current_winner(board u) 
{ 
	/*
		1. Check if x has "won" by searching for a capitalised 'X' on the grid; if so then assign 'x' to winner
		2. Check if o has "won" by searching for a capitalised 'O' on the grid; if so then check whether 'x' has also won: assign 'o' or 'd' to winner accordingly
		3. Check if the board is full by ensuring there is atleast one '.' on the grid. If the board is full, assign 'd' to winner only if a winner has not been found
	*/
	char winner;
	winner = '.';
	if (strchr(u->grid, 'X') != NULL) { winner = 'x'; }
	if (strchr(u->grid, 'O') != NULL) { winner = (winner == '.') ? 'o' : 'd'; }
	if (strchr(u->grid, '.') == NULL) { winner = (winner == '.') ? 'd' : winner;}  // Check if the board is full and react accordingly
	if (winner != '.' && winner != 'd' && winner != 'x' && winner != 'o') {
		fprintf(stderr, "[ERROR] Something went wrong when obtaining the winner; program halting.\n");
		exit(1);
	}
	return winner;
}

// Take inputs from the user (via stdin) to read in a move. Handles I/O parsing.
struct move read_in_move(board u) 
{ 
	struct move next_move;
	next_move.row = 0;
	next_move.column = 0;
	char buf[50];
	int n, end, len;
	printf("Player %c enter column to place your token: ", next_player(u)); //Do not edit this line
	if (fgets(buf, sizeof buf, stdin) == NULL) {
		fprintf(stderr, "[ERROR]: Something went wrong whilst taking an input; program halting.\n");
		exit(1);
	}
	// Remove newline from buffer and check for overflow
	len = strlen(buf);
	if (len > 0) {
		if (buf[len-1] != '\n') { next_move.column = u->cols*2; }
	}
	end = 0;
	if (sscanf(buf, "%d %n", &n, &end) != 1 || buf[end] != '\0') {
		next_move.column = u->cols*2;
	}
	else { next_move.column = n; }

	memset(buf, 0, strlen(buf));  // Clear the buffer
	printf("Player %c enter row to rotate: ", next_player(u)); // Do not edit this line
	if (fgets(buf, sizeof buf, stdin) == NULL) {
		fprintf(stderr, "[ERROR]: Something went wrong whilst taking an input; program halting.\n");
		exit(1);
	}
	len = strlen(buf);
	if (len > 0) {
		if (buf[len-1] != '\n') { next_move.row = u->rows*2; }
	}
	end = 0;
	if (sscanf(buf, "%d %n", &n, &end) != 1 || buf[end] != '\0') {
		next_move.row = u->rows*2;
	}
	else { next_move.row = n; }
	memset(buf, 0, strlen(buf));  // Clear the buffer

	return next_move;
}

// Ensure a given move will be valid for a given board: does so by ensuring the selected row or column is in range, and whether the column is full.
int is_valid_move(struct move m, board u) 
{
	if (m.column > u->cols || m.column < 1 || u->grid[m.column-1] != '.') {
		// printf("Invalid move! Ensure column is valid/not full.\n");
		return 0;
	}
	else if (m.row < -1*u->rows || m.row > u->rows) {
		// printf("Invalid move! Ensure row is valid.\n");
		return 0;
	}
	else return 1;  
}

// Check whether, upon playing a given move, the board will be in a winning configuraion. Return the winner, if this is the case.
char is_winning_move(struct move m, board u) 
{ 
	/*
		1. duplicate the grid (after checking move is valid)
		2. call play_move (which internally calls search_for_winner)
		3. call is_winning_move: store winner
		4. free the duplicated grid
		5. return the "winner"
	*/
	board dupe_board;
	char winner;
	if (!is_valid_move(m, u)) { return '.'; }
	dupe_board = setup_board();
	dupe_board->rows = u->rows;
	dupe_board->cols = u->cols;
	dupe_board->grid = malloc((sizeof(char)*dupe_board->rows*dupe_board->cols) + sizeof('\0'));
	strcpy(dupe_board->grid, u->grid);
	play_move(m, dupe_board);
	winner = current_winner(dupe_board);
	cleanup_board(dupe_board);
	return winner;
}

// Given a board and a valid move, modify the board according to the move
void play_move(struct move m, board u) 
{
	/*
		1. Ensure the given move is valid (if not, then raise an error)
		2. Modify the board according to m.column by inserting the correct token in the inputted column
		3. If m.row is not 0, rotate the corresponding row in the corresponding direction
		4. Apply "gravity" to the board to ensure any floating tokens move to the bottom (specifically required for when a row is rotated)
		5. Call the search_for_winner() function to check if the board is in a winning configuration, and alter the board accordingly
	*/
	int position = m.column - 1; 
	int placed = 0;
	int i;
	int r, row_start, row_end, temp;
	if (!is_valid_move(m, u)) { 
		fprintf(stderr, "[ERROR] The move is invalid, this function should not be called! Program halting.");
		exit(1);
	}
	// Insert counter
	position += u->cols * u->rows;
	while (placed == 0) {
		if (position < u->rows*u->cols && u->grid[position] == '.') {// available position
			u->grid[position] = next_player(u);
			placed = 1;
		}
		else {
			position -= u->cols;
			if (position < 0) {
				fprintf(stderr, "[ERROR] Something went wrong when playing a move; program halting.\n");
				exit(1);
			} 
		}
	}
	// Rotate row, if necesarry 
	if (m.row < 0) {
		r = u->rows + m.row;
		row_start = r * u->cols;
		row_end = row_start + u->cols - 1;
		temp = u->grid[row_start];
		for (i = row_start; i < row_end; i++) {
			u->grid[i] = u->grid[i+1];
		}
		u->grid[i] = temp;
	}
	else if (m.row > 0) {
		r = u->rows - m.row;
		row_start = r * u->cols;
		row_end = row_start + u->cols - 1;
		temp = u->grid[row_end];
		for (i = row_end; i > row_start; i--) {
			u->grid[i] = u->grid[i-1];
		}
		u->grid[i] = temp;
	}
	// Gravity: force any floating tokens to drop to the bottom of the board by scanning the board and checking whether any token is above a '.', and swapping these slots
	// Works backwards, from the bottom right of the grid to the top left. If a floating token is detected, then the above and below "sliders" also drop down so they can handle any higher tokens
	int below, above, threshold;
	below = (u->rows*u->cols) - 1;
	above = below - u->cols;
	threshold = above;
	while (above >= 0) {
		if (u->grid[above] != '.' && u->grid[below] == '.') {
			u->grid[below] = u->grid[above];
			u->grid[above] = '.';
			if (below <= threshold) {
				above = below;
				below += u->cols;
				continue;
			}
		}
		--below;
		--above;
	}
	// Check if, now the move has been played, there are any winners, and modify the board accordingly
	search_for_winner(u);
}

//You may put additional functions here if you wish.

// Scan the grid, capitalising any winning indices 
void search_for_winner(board u) 
{
	/*
		1. Initialise current_winner to '.' and scan through the board, keeping track of the current row:
		2. Whenever a token is encountered:
			- Check for a horizontal 4-in-a-row by checking the four tokens to the right of the token (wrapping around the row if necessary)
			- Check for a vertical 4-in-a-row (only do this if the current row is at a height of at least 4)
			- Check for diagonol 4-in-a-rows by scanning the left-downwards diagonol and right right-downwards diagonol (again, only if the row height is 4 or above)
		3. If a 4-in-a-row is found, capitalise the winning indices and set current_winner to the corresponding token; continue the search, now ignoring any tokens by this newfound winner
		4. If a second 4-in-a-row is found (by the opponent to the current_winner), capitalise these indices and call break to halt the search
		5. Check that the found winner corresponds to a valid token.
	*/
	int i, j;
	int current_row;
	char token;
	char current_winner;
	token = '.';
	current_winner = '.';

	// First, find if there is any winner at all
	for (i=0, current_row=0; i<u->rows*u->cols; i++) {
		if (i>0 && i%(u->cols) == 0) current_row++;

		// Check if the current index of the board contains a token (that isn't the "current_winner"), if not continue
		if (u->grid[i] != '.' && tolower(u->grid[i]) != tolower(current_winner)) {
			token = u->grid[i];
	
			// Check if the row contains a connect-4; only does this if there are enough columns for this to even occur
			if (u->cols >= 4
				&& u->grid[(current_row)*(u->cols) + (i + 1)%(u->cols)] == token   	 
				&& u->grid[(current_row)*(u->cols) + (i + 2)%(u->cols)] == token 
				&& u->grid[(current_row)*(u->cols) + (i + 3)%(u->cols)] == token) 
			{
				for (j=0; j<4; j++) {
					u->grid[(current_row)*(u->cols) + (i + j)%(u->cols)] = toupper(token);
				}
				if (current_winner == '.') { current_winner = token; }
				else { break; }
			}

			// Only check these if there is enough space for a verticle or diagonol connect 4 to occur
			else if (u->rows - current_row > 3) {
				// Check if the right-diagonol contains a connect-4
				if (u->grid[(current_row + 1)*(u->cols) + (i + 1)%(u->cols)] == token 
					&& u->grid[(current_row + 2)*(u->cols) + (i + 2)%(u->cols)] == token 
					&& u->grid[(current_row + 3)*(u->cols) + (i + 3)%(u->cols)] == token)
				{
					for (j=0; j<4; j++) {
						u->grid[(current_row + j)*(u->cols) + (i + j)%(u->cols)] = toupper(token);
					}
					if (current_winner == '.') { current_winner = token; }
					else { break; }
				}

				// Check if the left-diagonol contains a connect-4
				if (u->grid[(current_row + 1)*(u->cols) + (i - 1)%(u->cols)] == token 
					&& u->grid[(current_row + 2)*(u->cols) + (i - 2)%(u->cols)] == token 
					&& u->grid[(current_row + 3)*(u->cols) + (i - 3)%(u->cols)] == token)
				{
					for (j=0; j<4; j++) {
						u->grid[(current_row + j)*(u->cols) + (i - j)%(u->cols)] = toupper(token);
					}
					if (current_winner == '.') { current_winner = token; }
					else  { break; }
				}

				// 5. Check if the column contains a connect-4
				else if (u->grid[(current_row + 1)*(u->cols) + i%(u->cols)] == token 
						 && u->grid[(current_row + 2)*(u->cols) + i%(u->cols)] == token 
						 && u->grid[(current_row + 3)*(u->cols) + i%(u->cols)] == token)
				{
					for (j=0; j<4; j++) {
						u->grid[(current_row + j)*(u->cols) + i%(u->cols)] = toupper(token);
					}
					if (current_winner == '.') { current_winner = token; }
					else { break; }
				}
			}
		}
	}
	if (current_winner != '.' && current_winner != 'x' && current_winner != 'o' && current_winner != 'd') {
		fprintf(stderr, "[ERROR] Something went wrong whilst validating the winning move; program halting.\n");
		exit(1);
	}
}
