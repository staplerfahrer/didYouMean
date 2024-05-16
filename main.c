#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <io.h>
#include <windows.h>

#define UCHAR               unsigned char

/*
*TABLE_______________FIELD_____DISTINCT____________________________________________________________________________________________
*/
#define DBF_NAME            "relatedText.dbf"
#define DBF_TEMPLATE        "outputTemplate.dbf"
#define DBF_TEMPLATE_HEADER 328
#define DBF_OUTPUT          "result.dbf"
#define DBF_ROWCOUNT_AT     4
#define DBF_DATA_AT         392
#define DBF_ROW_LEN         132
#define DBF_TABLE_AT        1
#define DBF_TABLE_LEN       20
#define DBF_FIELD_AT        21
#define DBF_FIELD_LEN       10
#define DBF_DISTINCT_AT     31
#define DBF_DISTINCT_LEN    100

int d_min = 0;
int d_max = 0; 

// + 1 for the first byte to contain initial distance number
UCHAR d_matrix[(DBF_DISTINCT_LEN + 1)][(DBF_DISTINCT_LEN + 1)] = {0};

int result_idx = 0;
// + 1 for null terminator
UCHAR results[1000][DBF_DISTINCT_LEN + 1] = {0};

void levenshtein_distance(UCHAR* s, UCHAR* t)
{
	int i = 0;
	int j = 0;
	UCHAR substCost = 0;
	UCHAR x = 0;

	for (i = 0; i < (DBF_DISTINCT_LEN + 1); i++)
	{
		for (j = 0; j < (DBF_DISTINCT_LEN + 1); j++)
			d_matrix[i][j] = 0;
		d_matrix[0][i] = i;
		d_matrix[i][0] = i;
	}

	for (i = 1; i < (DBF_DISTINCT_LEN + 1); i++)
	{
		d_min = DBF_DISTINCT_LEN;
		for (j = 1; j < (DBF_DISTINCT_LEN + 1); j++)
		{
			substCost = 1;
			if (s[i - 1] == t[j - 1]) substCost = 0;

			d_matrix[i][j] =
				d_matrix[i - 1][j    ] + 1;

			x = d_matrix[i    ][j - 1] + 1;
			if (x < d_matrix[i][j]) 
				d_matrix[i][j] = x;

			x = d_matrix[i - 1][j - 1] + substCost;
			if (x < d_matrix[i][j]) 
				d_matrix[i][j] = x;

			// find the lowest value on this line
			if (d_matrix[i][j] < d_min)
				d_min = d_matrix[i][j];
		}
		if (d_min > d_max) return;
	}

	results[result_idx][0] = 255 - d_min;
	memcpy(&results[result_idx][1], t, DBF_DISTINCT_LEN);
	result_idx++;
}

void output_sorted()
{
	// load header
	UCHAR header[DBF_TEMPLATE_HEADER];
	FILE *file = fopen(DBF_TEMPLATE, "r");
	assert(file != NULL);
	assert(fgets(header, sizeof(header), file) != NULL);
	fclose(file);

	// open output file
	FILE *out = fopen(DBF_OUTPUT, "wb");
	assert(out != NULL);
	fwrite(header, sizeof(header), 1, out);

	UCHAR score, score_inverted = 0;
	UCHAR space                 = ' ';
	unsigned short i, row_count = 0;
	for (score = 0; score < 255; score++)
	{
		score_inverted = 255 - score;
		for (i = 0; i < 1000; i++)
		{
			if (results[i][0] == 0) break;
			if (results[i][0] != score_inverted) continue;
			// write the space for a non-deleted record
			fwrite(&space, 1, 1, out);
			// skip the score byte
			fwrite(&results[i][1], DBF_DISTINCT_LEN, 1, out);
			row_count++;
		}
	}

	// write eof
	UCHAR eof = 0x1A;
	fwrite(&eof, 1, 1, out);

	// write row count
	fseek(out, DBF_ROWCOUNT_AT, SEEK_SET);
	fwrite(&row_count, sizeof(row_count), 1, out);
	fclose(out);
}

int main(int argc, const UCHAR * argv[])
{
	// argv:
	// [1]="NAMES" [2]="LNAME" [3]="BRUINS" [4]=d_max

	DWORD start = GetTickCount();

	// init at 0
	UCHAR row[DBF_ROW_LEN] = {0};
	UCHAR tbl_fld_src[DBF_TABLE_LEN + DBF_FIELD_LEN + 1] = {0}; // +1 for \0
	UCHAR tbl_fld_cmp[DBF_TABLE_LEN + DBF_FIELD_LEN + 1] = {0}; // +1 for \0
	UCHAR search_for[DBF_DISTINCT_LEN + 1] = {0};               // +1 for \0
	UCHAR distinct[DBF_DISTINCT_LEN + 1] = {0};                 // +1 for \0
	UCHAR output_name[100] = {0};                 

	// fill with spaces
	int i;
	for (i = 0; i < sizeof(tbl_fld_src) - 1; i++) tbl_fld_src[i] = ' ';
	for (i = 0; i < sizeof(tbl_fld_cmp) - 1; i++) tbl_fld_cmp[i] = ' ';
	for (i = 0; i < sizeof(search_for ) - 1; i++) search_for[i]  = ' ';

	// don't copy the null terminator
	memcpy(tbl_fld_src, argv[1], strlen(argv[1]));                  // [1]
	memcpy(&tbl_fld_src[DBF_FIELD_AT-1], argv[2], strlen(argv[2])); // [2]
	memcpy(search_for, argv[3], strlen(argv[3]));                   // [3]
	d_max = atoi(argv[4]);                                          // [4]
	memcpy(output_name, argv[5], strlen(argv[5]));                  // [5]

	// null terminate at the end
	search_for[DBF_DISTINCT_LEN] = 0;
	distinct[DBF_DISTINCT_LEN] = 0;
	
	FILE *file = fopen(DBF_NAME, "r");
	assert(file != NULL);
	fseek(file, DBF_DATA_AT, SEEK_SET); // skip header
	while (fgets(row, DBF_ROW_LEN, file) != NULL)
	{
		if (row[0] == '*') continue; // skip deleted records
		memcpy(tbl_fld_cmp, &row[1], sizeof(tbl_fld_cmp)-1);
		if (strcmp(tbl_fld_cmp, tbl_fld_src) != 0) continue; // skip non-matching table and field
		memcpy(distinct, &row[DBF_DISTINCT_AT], DBF_DISTINCT_LEN);
		levenshtein_distance(search_for, distinct);
	}
	fclose(file);

	output_sorted();
	
	int ren_res = rename(DBF_OUTPUT, output_name);
	fprintf(stderr, "rename result: %d, took %d ms\n", ren_res, GetTickCount() - start);
}
