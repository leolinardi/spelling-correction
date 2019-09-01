/* * * * * * * *
 * Module for implementing a spelling corrector: a program that 
 * takes a document and attempts to correct misspelled words
 * within that document
 *
 * Task 1 to 4
 * COMP20007 Design of Algorithms - Assignment 2, 2018
 * Created by Leonardo Linardi, 855915 <llinardi@student.unimelb.edu.au>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "spell.h"
#include "hashtbl.h"

#define DEF_FREQ 1	// sets a default frequency for the hash table

// store important values of a possible corrected word, for Task 4
typedef struct {
	char *word;	
	int pos;	// stores the position of the corrected word in the dictionary
	int corr;	// flag that indicates whether a corrected word is found
} possibleword;

/*----------------------------------------------------------------------*/
/* DEFINING FUNCTIONS */
#define MIN(X,Y) (((X)<(Y))? (X):(Y)) // finds the minimum value between X and Y

/*----------------------------------------------------------------------*/
/* HELPER FUNCTION PROTOTYPES */
List *generate_edit(char *word);
void correction_hash(List *editlist, HashTable *table, possibleword *cword);
void correction_lookup(List *dictionary, char *wword, possibleword *cword, int edist);
void free_editword(List *editlist);
int editdistance(char *word1, char *word2, int editd);

/*----------------------------------------------------------------------*/
/* TASK 1 */
/* Finds the minimum Levenshtein edit distance between 'word1' 
 * and 'word2', through substitutions, deletions or insertions
 */
void print_edit_distance(char *word1, char *word2) {
	int n=strlen(word1);
	int m=strlen(word2);
	int **edit;
	int i, j, sub_cost=0;
	int edist;

	// allocating memory for the 2D array
	edit = (int **)malloc(sizeof(int*)*(n+1));
	assert(edit);
	for (i=0; i<n+1; i++) {
		edit[i] = (int *)malloc(sizeof(int)*(m+1));
		assert(edit[i]);
	}

	// fill in the known values for word 1
	for (i=0; i<n+1; i++) {
		edit[i][0]=i;
	}
	// fill in the known values for word 2
	for (j=0; j<m+1; j++) {
		edit[0][j]=j;
	}

	// fill in the rest of the table
	for (i=1; i<n+1; i++) {
		for (j=1; j<m+1; j++) {
			
			// determine whether there is a substitute cost
			if (word1[i-1]==word2[j-1]) {
				sub_cost=0;
			} else {
				sub_cost=1;
			}

			// finding the minimum edit distance 
			// either by substitution, insertion or deletion
			edist = MIN(edit[i-1][j-1]+sub_cost, MIN(edit[i-1][j] + 1, edit[i][j-1] + 1));
			edit[i][j]=edist;
		}
	}
	printf("%d\n", edit[n][m]);

	// frees all the memory allocated, yay!
	for (i=0; i<n+1; i++) {
		free(edit[i]);
	}
	free(edit);
}

/*----------------------------------------------------------------------*/
/* TASK 2 */
/* Enumerating all possible edits within a Levenshtein edit distance
 * of 1 from 'word'
 */
void print_all_edits(char *word) {
	char *ALPHAB="abcdefghijklmnopqrstuvwxyz";
	int i,j;
	int n=strlen(word);		
	int a=strlen(ALPHAB);

	// through substitution
	for (i=0; i<n; i++) {
		for (j=0; j<a; j++) {
			printf("%.*s%c%.*s", i, word, ALPHAB[j], n-i-1, word+i+1);
			printf("\n");
		}
	}
	// through deletion
	for (i=0; i<n; i++) {
		printf("%.*s%.*s", i, word, n-i-1, word+i+1);
		printf("\n");
	}
	// through insertion
	for (i=0; i<n+1; i++) {
		for (j=0; j<a; j++) {
			printf("%.*s%c%.*s", i, word, ALPHAB[j], n-i, word+i);
			printf("\n");
		}
	}
}

/*----------------------------------------------------------------------*/
/* TASK 3 */
/* Check if the words inside 'document' occurs in 'dictionary'
 * that is, if it's a correctly spelled word
 */
void print_checked(List *dictionary, List *document) {
	char *word;

	// initialise hash table
	HashTable *table = new_hash_table(dictionary->size);

	// store the dictionary inside a hash table
	Node *curr_node = dictionary->head;
	while (curr_node) {
		word = curr_node->data;

		// checks if the word already exists in the hash table
		if (! hash_table_has(table, word)) {
			hash_table_put(table, curr_node->data, DEF_FREQ);
		}
		// skips if the word already exists
		curr_node = curr_node->next;
	}

	// search whether the document words are inside the dictionary
	curr_node = document->head;
	while (curr_node) {
		word = curr_node->data;
		if (hash_table_has(table, word)) {
			printf("%s\n", word);
		}
		// the word is incorrectly spelled
		else {
			printf("%s?\n", word);
		}
		curr_node = curr_node->next;
	}

	// frees all the memory, halleluya!
	free_hash_table(table);
}

/*----------------------------------------------------------------------*/
/* TASK 4 */
/* Check if the words inside 'document' occurs in 'dictionary'
 * and attempts to correct the word if it's incorrectly spelled,
 * with a Levenshtein edit distance of 1, 2 or 3
 */
void print_corrected(List *dictionary, List *document) {
	char *word;
	int order=0;

	// initialise hash table
	HashTable *table = new_hash_table(dictionary->size);

	// create a hash table to store the dictionary words
	Node *curr_node = dictionary->head;
	while (curr_node) {
		word = curr_node->data;

		// checks if the word already exists in the hash table
		if (! hash_table_has(table, word)) {
			hash_table_put(table, curr_node->data, order);
			order++;
		}
		// skips if the word already exists
		curr_node = curr_node->next;
	}

	// initialise some variables
	possibleword cword;
	cword.corr=0;
	cword.pos=dictionary->size;
	char *wword, *word2, *finalword;
	List *editlist1, *editlist2;
	Node *curr_edit1;

	// search for a corrected word for every word in the document
	curr_node = document->head;
	while (curr_node) {
		cword.corr=0;
		cword.pos=dictionary->size;
		wword = curr_node->data;

		//--- CASE 1: Correctly spelled word ---//
		if (hash_table_has(table, wword)) {
			// stores the final corrected word
			finalword=hash_table_get_key(table, wword);
			cword.corr=1;
		}

		//--- CASE 2: One edit-distance away ---//
		if (!cword.corr) {
			// generate the list of 1 edit distance words
			editlist1 = generate_edit(wword);
		
			// searches for the corrected version of the word
			correction_hash(editlist1, table, &cword);
			
			// stores the final corrected word
			if (cword.corr) {
				finalword = cword.word;
			}

			//--- CASE 3: Two edit-distance away ---//
			if (!cword.corr) {				
				curr_edit1 = editlist1->head;

				// for each 1 edit dist word, search for another 1 edit dist words
				while (curr_edit1) {
					word2 = curr_edit1->data;

					// generate the list of 2 edit distance words
					editlist2 = generate_edit(word2);
					
					// searches for the corrected version of the word
					correction_hash(editlist2, table, &cword);
				
					// stores the final corrected word
					if (cword.corr) {
						finalword = cword.word;
					}
					// frees the list everytime it's generated, awesome!
					free_editword(editlist2);
					free_list(editlist2);
					curr_edit1 = curr_edit1->next;
				}
			}
			// keeps on free-ing memories, hooray!
			free_editword(editlist1);
			free_list(editlist1);
		}

		//--- CASE 4: Three edit-distance away ---//
		if (!cword.corr) {
			// perform a direct lookup
			correction_lookup(dictionary, wword, &cword, 3);
			// stores the final corrected word
			if (cword.corr) {
				finalword=cword.word;
			}
		}

		// prints the corrected word
		if (cword.corr) {
			printf("%s\n", finalword);
		}
		else {
			printf("%s?\n", wword);
		}
		curr_node = curr_node->next;
	}

	// frees the memory allocated for the huge table, yippee!
	free_hash_table(table);
}

/*----------------------------------------------------------------------*/
/* SOME HELPER FUNCTIONS */


/* Finds the corrected word that shows first in the dictionary, by comparing 
 * a list of 1 edit distance words to a hash table of dictionary words  
 */
void correction_hash(List *editlist, HashTable *table, possibleword *cword) {
	Node *curr_word=editlist->head;
	while (curr_word) {

		// a corrected word is found
		if (hash_table_has(table, curr_word->data)) {

			// finds the corrected word that shows up first in the dictionary
			if (hash_table_get_val(table, curr_word->data) <= cword->pos) {
				// store some values of the corrected word
				cword->word = hash_table_get_key(table, curr_word->data);
				cword->pos = hash_table_get_val(table, curr_word->data);
				cword->corr=1;
			}
		}
		curr_word = curr_word->next;
	}
}

/* Finds the corrected word that shows first in the dictionary, by iterating 
 * through the whole dictionary and comparing it to the wrong word,
 * with a given specific edit distance
 */
void correction_lookup(List *dictionary, char *wword, possibleword *cword, int edist) {
	// iterates through the whole dictionary
	Node *curr_word=dictionary->head;
	while (curr_word) {
		
		// compares it's edit distance
		if (editdistance(curr_word->data, wword, edist)) {
			// a corrected word is found
			cword->word = curr_word->data;
			cword->corr=1;
			break;
		}
		curr_word = curr_word->next;
	}
}

/* Generates a list of all words that is 1 edit distance away from 'word'
 */
List *generate_edit(char *word) {
	char *ALPHAB="abcdefghijklmnopqrstuvwxyz";
	int i,j;
	int n=strlen(word);		
	int a=strlen(ALPHAB);

	// create a list of words 
	List *editlist=new_list();
	char *editword;

	// through substitution
	for (i=0; i<n; i++) {
		for (j=0; j<a; j++) {
			// allocate enough memory to store the edited word
			editword = (char *)malloc(sizeof(char)*(strlen(word)+1));
			assert(editword);
			sprintf(editword, "%.*s%c%.*s", i, word, ALPHAB[j], n-i-1, word+i+1); 
			list_add_end(editlist, editword);
		}
	}
	// through deletion
	for (i=0; i<n; i++) {
		editword = (char *)malloc(sizeof(char)*(strlen(word)));
		assert(editword);
		sprintf(editword, "%.*s%.*s", i, word, n-i-1, word+i+1);
		list_add_end(editlist, editword);
	}
	// through insertion
	for (i=0; i<n+1; i++) {
		for (j=0; j<a; j++) {
			editword = (char *)malloc(sizeof(char)*(strlen(word)+2));
			assert(editword);
			sprintf(editword, "%.*s%c%.*s", i, word, ALPHAB[j], n-i, word+i);
			list_add_end(editlist, editword);
		}
	}
	return editlist;
}

/* Frees the memory allocated for the edited words list
 */
void free_editword(List *editlist) {
	Node *curr_node = editlist->head;
	while (curr_node) {	
		free(curr_node->data);
		curr_node=curr_node->next;
	}
}

/* Finds the edit distance between 'word1' and 'word2', and checks whether
 * it is equal to the required edit distance 'editd'
 */
int editdistance(char *word1, char *word2, int editd) {
	int n=strlen(word1);
	int m=strlen(word2);
	int **edit;
	int i, j, sub_cost=0;
	int edist, match=0;

	// allocating memory for the 2D array
	edit = (int **)malloc(sizeof(int*)*(n+1));
	assert(edit);
	for (i=0; i<n+1; i++) {
		edit[i] = (int *)malloc(sizeof(int)*(m+1));
		assert(edit[i]);
	}

	// fill in the known values for word 1
	for (i=0; i<n+1; i++) {
		edit[i][0]=i;
	}
	// fill in the known values for word 2
	for (j=0; j<m+1; j++) {
		edit[0][j]=j;
	}

	// fill in the rest of the table
	for (i=1; i<n+1; i++) {
		for (j=1; j<m+1; j++) {
			
			// determine whether there is a substitute cost
			if (word1[i-1]==word2[j-1]) {
				sub_cost=0;
			} else {
				sub_cost=1;
			}

			// finding the minimum edit distance 
			// either by substitution, insertion or deletion
			edist = MIN(edit[i-1][j-1]+sub_cost, MIN(edit[i-1][j] + 1, edit[i][j-1] + 1));
			edit[i][j]=edist;
		}
	}

	// checks if the edit distance matches the required 'editd'
	if (edit[n][m]==editd) {
		match=1;
	}
	// frees all the memory allocated, finally!
	for (i=0; i<n+1; i++) {
		free(edit[i]);
	}
	free(edit);
	return match;
}