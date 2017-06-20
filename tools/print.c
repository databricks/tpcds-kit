/* 
 * Legal Notice 
 * 
 * This document and associated source code (the "Work") is a part of a 
 * benchmark specification maintained by the TPC. 
 * 
 * The TPC reserves all right, title, and interest to the Work as provided 
 * under U.S. and international laws, including without limitation all patent 
 * and trademark rights therein. 
 * 
 * No Warranty 
 * 
 * 1.1 TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THE INFORMATION 
 *     CONTAINED HEREIN IS PROVIDED "AS IS" AND WITH ALL FAULTS, AND THE 
 *     AUTHORS AND DEVELOPERS OF THE WORK HEREBY DISCLAIM ALL OTHER 
 *     WARRANTIES AND CONDITIONS, EITHER EXPRESS, IMPLIED OR STATUTORY, 
 *     INCLUDING, BUT NOT LIMITED TO, ANY (IF ANY) IMPLIED WARRANTIES, 
 *     DUTIES OR CONDITIONS OF MERCHANTABILITY, OF FITNESS FOR A PARTICULAR 
 *     PURPOSE, OF ACCURACY OR COMPLETENESS OF RESPONSES, OF RESULTS, OF 
 *     WORKMANLIKE EFFORT, OF LACK OF VIRUSES, AND OF LACK OF NEGLIGENCE. 
 *     ALSO, THERE IS NO WARRANTY OR CONDITION OF TITLE, QUIET ENJOYMENT, 
 *     QUIET POSSESSION, CORRESPONDENCE TO DESCRIPTION OR NON-INFRINGEMENT 
 *     WITH REGARD TO THE WORK. 
 * 1.2 IN NO EVENT WILL ANY AUTHOR OR DEVELOPER OF THE WORK BE LIABLE TO 
 *     ANY OTHER PARTY FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO THE 
 *     COST OF PROCURING SUBSTITUTE GOODS OR SERVICES, LOST PROFITS, LOSS 
 *     OF USE, LOSS OF DATA, OR ANY INCIDENTAL, CONSEQUENTIAL, DIRECT, 
 *     INDIRECT, OR SPECIAL DAMAGES WHETHER UNDER CONTRACT, TORT, WARRANTY,
 *     OR OTHERWISE, ARISING IN ANY WAY OUT OF THIS OR ANY OTHER AGREEMENT 
 *     RELATING TO THE WORK, WHETHER OR NOT SUCH AUTHOR OR DEVELOPER HAD 
 *     ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. 
 * 
 * Contributors:
 * Gradient Systems
 */ 
#include "config.h"
#include "porting.h"
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "tables.h"
#include "print.h"
#include "tdef_functions.h"
#include "r_params.h"
#include "date.h"
#include "decimal.h"
#include "tdefs.h"
#include "nulls.h"
#include "constants.h"
#include "build_support.h"

static FILE *fpOutfile = NULL;
static FILE *fpDeleteFile;
static char *arDeleteFiles[3] = {"", "delete_", "inventory_delete_"};

static int current_table = -1;
static char linebuf[1024 * 128];
static int linebufpos;
static ds_key_t nullBuf;
static int nullFirst;
#define nullCheck(x) ((nullBuf >> (x - nullFirst)) & 1)

int print_jdate (FILE *pFile, ds_key_t kValue);

void 
print_close(int tbl)
{
   tdef *pTdef = getSimpleTdefsByNumber(tbl);

	fpOutfile = NULL;
	if (pTdef->outfile)
	{
		fclose(pTdef->outfile);
		pTdef->outfile = NULL;
	}

	if (pTdef->flags & FL_PARENT)
      print_close(pTdef->nParam);

	return;
}

int
print_separator (int sep)
{
	int res = 0;
	static char pDelimiter;
	static int init = 0;
	
	if (!init)
	{
        pDelimiter = get_str ("DELIMITER")[0];
        init = 1;
	}
	
	if (sep)
	{
        linebuf[linebufpos++] = pDelimiter;
	}
	   
	return (res);
}

static char *format_int(int v) {
    static char tmp[20];
    char *buf = tmp + 20;
    *(--buf) = '\0';
    *(--buf) = '0' + v % 10;
    v /= 10;
    while (v) {
        *(--buf) = '0' + v % 10;
        v /= 10;
    }
    return buf;
}
static char *format_long(long long v) {
    static char tmp[30];
    char *buf = tmp + 30;
    *(--buf) = '\0';
    *(--buf) = '0' + v % 10;
    v /= 10;
    while (v) {
        *(--buf) = '0' + v % 10;
        v /= 10;
    }
    return buf;
}
static char *format_decimal(decimal_t * val) {
    static char tmp[20];
    char *buf = tmp + 20;
    *(--buf) = '\0';
    int v = val->number;
    int f, s = v < 0;
    if (s) v = -v;
    f = v %100;
    v /= 100;
    *(--buf) = '0' + f % 10;
    *(--buf) = '0' + f / 10;
    *(--buf) = '.';
    *(--buf) = '0' + v % 10;
    v /= 10;
    while (v) {
        *(--buf) = '0' + v % 10;
        v /= 10;
    }
    if (s) *(--buf) = '-';
    return buf;
}

/*
* Routine: 
* Purpose: 
* Algorithm:
* Data Structures:
*
* Params:
* Returns:
* Called By: 
* Calls: 
* Assumptions:
* Side Effects:
* TODO: None
*/
FILE *
print_prep (int table, int update)
{
   return (NULL);
}

/*
* Routine: dbg_print()
* Purpose: genralized data print routine
* Algorithm:
* Data Structures:
*
* Params:
* Returns:
* Called By: 
* Calls: 
* Assumptions:
* Side Effects:
* TODO: 20000113 need better separator handling
*		20020125 cast to/from 64b is messy, assumes int/pointer are same size
*/
void
print_integer (int nColumn, int val, int sep)
{
    if (!nullCheck(nColumn))
    {
        char *vs;
        if (val < 0) {
            linebuf[linebufpos++] = '-';
            val = -val;
        }
        vs = format_int(val);
        strcpy(linebuf + linebufpos, vs);
        linebufpos += strlen(vs);
    }
    print_separator (sep);
}

void
print_varchar (int nColumn, char *val, int sep)
{
	size_t nLength;

	if (!nullCheck(nColumn) && (val != NULL))
	{
        strcpy(linebuf + linebufpos, val);
        linebufpos += strlen(val);
	}
	print_separator (sep);
	
   return;
}

void
print_delete (int * val)
{
   if (print_jdate(fpDeleteFile, *val))
	{
      fprintf(stderr, "ERROR: Failed to write delete key\n");
      exit(-1);
	}
   fprintf(fpDeleteFile, "%s", get_str("DELIMITER"));
   if (print_jdate(fpDeleteFile, *(val + 1)))
	{
      fprintf(stderr, "ERROR: Failed to write delete key\n");
      exit(-1);
	}
   if (is_set("TERMINATE"))
	   fprintf(fpDeleteFile, get_str("DELIMITER"));
   fprintf(fpDeleteFile, "\n");
   
   return;
}

/*
void
print_cp_delete (int nCatalog, int nPage)
{
   static int bInit = 0;
   static char *cp;
   
   if (!bInit)
   {
      cp = strdup(get_str("DELIMITER"));
      bInit = 1;
   }
   
   if (fprintf (fpDeleteFile, "%d%s%d\n", nCatalog, cp, nPage) < 0)
	{
      fprintf(stderr, "ERROR: Failed to write delete key\n");
      exit(-1);
	}
   
   return;
}
*/

void
print_date (int nColumn, ds_key_t val, int sep)
{
	if (!nullCheck(nColumn))
	{
		if (val > 0)
		{
            date_t dTemp;
            jtodt (&dTemp, (int) val);
            memcpy(linebuf + linebufpos, dttostr(&dTemp), 10);
            linebufpos += 10;
		}
	}
	
	print_separator (sep);
}

void
print_time (int nColumn, ds_key_t val, int sep)
{
	int nHours, nMinutes, nSeconds;

	nHours = (int)(val / 3600);
	val -= 3600 * nHours;
	nMinutes = (int)(val / 60);
	val -= 60 * nMinutes;
	nSeconds = (int)(val % 60);
	
	if (!nullCheck(nColumn)) {
        if (val != -1) {
            linebuf[linebufpos++] = '0' + nHours / 10;
            linebuf[linebufpos++] = '0' + nHours % 10;
            linebuf[linebufpos++] = '0' + nMinutes / 10;
            linebuf[linebufpos++] = '0' + nMinutes % 10;
            linebuf[linebufpos++] = '0' + nSeconds / 10;
            linebuf[linebufpos++] = '0' + nSeconds % 10;
        }
    }

	print_separator (sep);
}

void
print_decimal (int nColumn, decimal_t * val, int sep)
{
    if (!nullCheck(nColumn))
    {
        char *vs = format_decimal(val);
        strcpy(linebuf + linebufpos, vs);
        linebufpos += strlen(vs);
    }
    print_separator (sep);

    return;
}
void
print_key (int nColumn, ds_key_t val, int sep)
{
    if (!nullCheck(nColumn) && val != (ds_key_t)-1)
    {
        char *vs = format_long(val);
        strcpy(linebuf + linebufpos, vs);
        linebufpos += strlen(vs);
    }
    print_separator (sep);
}

void
print_id (int nColumn, ds_key_t val, int sep)
{
   char szID[RS_BKEY + 1];
   
   if (!nullCheck(nColumn))
   {
      if (val != (ds_key_t) -1) /* -1 is a special value, indicating NULL */
      {
         mk_bkey(szID, val, 0);
          memcpy(linebuf + linebufpos, szID, RS_BKEY);
          linebufpos += RS_BKEY;
      }
   }
  print_separator (sep);
   
   return;
}

void
print_boolean (int nColumn, int val, int sep)
{
	if (!nullCheck(nColumn))
	{
        linebuf[linebufpos++] = val ? 'Y' : 'N';
	}

	print_separator (sep);

	return;
}

/*
* Routine: print_start(tbl)
* Purpose: open the output file for a given table
* Algorithm:
* Data Structures:
*
* Params:
* Returns:
* Called By: 
* Calls: 
* Assumptions:
* Side Effects:
* TODO: None
*/
int
print_start (int tbl)
{
   int res = 0;
   char path[256];
   tdef *pTdef = getSimpleTdefsByNumber(tbl);


   current_table = tbl;

   if (is_set_filter())
	   fpOutfile = stdout;
   else
   {
	   if (pTdef->outfile == NULL)
	   {
		   if (is_set("PARALLEL"))
			   sprintf (path, "%s%c%s_%d_%d%s",
			   get_str ("DIR"),
			   PATH_SEP, getTableNameByID (tbl), 
			   get_int("CHILD"), get_int("PARALLEL"), (is_set("VALIDATE"))?get_str ("VSUFFIX"):get_str ("SUFFIX"));
		   else 
		   {
			   if (is_set("UPDATE"))
				   sprintf (path, "%s%c%s_%d%s",
				   get_str ("DIR"),
				   PATH_SEP, getTableNameByID (tbl), get_int("UPDATE"), (is_set("VALIDATE"))?get_str ("VSUFFIX"):get_str ("SUFFIX"));
			   else
				   sprintf (path, "%s%c%s%s",
				   get_str ("DIR"),
				   PATH_SEP, getTableNameByID (tbl), (is_set("VALIDATE"))?get_str ("VSUFFIX"):get_str ("SUFFIX"));
		   }
		   if ((access (path, F_OK) != -1) && !is_set ("FORCE"))
		   {
			   fprintf (stderr,
				   "ERROR: %s exists. Either remove it or use the FORCE option to overwrite it.\n",
				   path);
			   exit (-1);
		   }
#ifdef WIN32
		   pTdef->outfile = fopen (path, "wt");
#else
		   pTdef->outfile = fopen (path, "w");
#endif
	   }
	fpOutfile = pTdef->outfile;
   }
   
   res = (fpOutfile != NULL);

   if (!res)                    /* open failed! */
     {
        INTERNAL ("Failed to open output file!");
	exit(0);
     }
#ifdef WIN32
   else if (setvbuf (fpOutfile, NULL, _IOFBF, 32767))
     {
        INTERNAL ("setvbuf() FAILED");
     }
#endif

   pTdef->flags |= FL_OPEN;
   linebufpos = 0;
   nullBuf = pTdef->kNullBitMap;
   nullFirst = pTdef->nFirstColumn;

   return (0);
}

/*
* Routine: 
* Purpose: 
* Algorithm:
* Data Structures:
*
* Params:
* Returns:
* Called By: 
* Calls: 
* Assumptions:
* Side Effects:
* TODO: None
*/
int
print_end (int tbl)
{
   int res = 0;
   static int init = 0;
   static int add_term = 0;
   static char term[10];

   if (!init)
     {
        if (is_set ("TERMINATE"))
          {
             strncpy (term, get_str ("DELIMITER"), 9);
             add_term = strlen(term);
          }
        init = 1;
     }

   if (add_term) {
       memcpy(linebuf + linebufpos, term, add_term);
       linebufpos += add_term;
   }
    linebuf[linebufpos++] = '\n';
    fwrite(linebuf, 1, linebufpos, fpOutfile);

   return (res);
}

/*
* Routine: openDeleteFile(void)
* Purpose: open the output file for the delete keys for a given table
* Algorithm:
* Data Structures:
*
* Params:
* Returns:
* Called By: 
* Calls: 
* Assumptions:
* Side Effects:
* TODO: None
*/
int
openDeleteFile(int bOpen)
{
   int res = 0;
   char path[256];
   
   if (!bOpen)
      fclose(fpDeleteFile);
   else
   {
      sprintf (path, "%s%c%s%d%s",
         get_str ("DIR"),
         PATH_SEP, arDeleteFiles[bOpen], get_int("UPDATE"), get_str("SUFFIX"));
      if ((access (path, F_OK) != -1) && !is_set ("FORCE"))
      {
         fprintf (stderr,
            "ERROR: %s exists. Either remove it or use the FORCE option to overwrite it.\n",
            path);
         exit (-1);
      }
#ifdef WIN32
      fpDeleteFile = fopen (path, "wt");
#else
      fpDeleteFile = fopen (path, "w");
#endif
      
      res = (fpDeleteFile != NULL);
      
      if (!res)                    /* open failed! */
      {
         INTERNAL ("Failed to open output file!");
      }
#ifdef WIN32
      else if (setvbuf (fpDeleteFile, NULL, _IOFBF, 32767))
      {
         INTERNAL ("setvbuf() FAILED");
      }
#endif
   }
   
   return (0);
}

/*
* Routine: print_string()
* Purpose: genralized data print routine
* Algorithm:
* Data Structures:
*
* Params:
* Returns:
* Called By: 
* Calls: 
* Assumptions:
* Side Effects:
*/
void
print_string (char *szMessage, ds_key_t val)
{
	if (fprintf (fpOutfile, szMessage, val) < 0)
	{
		fprintf(stderr, "ERROR: Failed to write string\n");
		exit(-1);
	}

	return;
}

/*
* Routine: print_jdate()
* Purpose: genralized data print routine
* Algorithm:
* Data Structures:
*
* Params:
* Returns:
* Called By:
* Calls:
* Assumptions:
* Side Effects:
*/
int
print_jdate (FILE *pFile, ds_key_t kValue)
{

    date_t dTemp;

    jtodt (&dTemp, (int) kValue);
#if (defined(STR_QUOTES) && !defined(_MYSQL))
    if ((fwrite ("\"", 1, 1, pFile) != 1) ||
		(fwrite(dttostr(&dTemp), 1, 10, pFile) != 10) ||
		(fwrite ("\"", 1, 1, pFile)) != 1)
#else
    if (fwrite(dttostr(&dTemp), 1, 10, pFile) != 10)
#endif
        return(-1);
    return(0);
}

/*
* Routine: 
* Purpose: 
* Algorithm:
* Data Structures:
*
* Params:
* Returns:
* Called By: 
* Calls: 
* Assumptions:
* Side Effects:
* TODO: None
*/
void
print_validation(ds_key_t kRowNumber)
{
	static int bInit = 0;
	static char szValidateFormat[20];

	if (!bInit)
	{
		sprintf(szValidateFormat, "Row #%s: ", HUGE_FORMAT);
		bInit = 1;
	}
	
	if (is_set("VALIDATE"))
	{
		print_string(szValidateFormat, kRowNumber);
	}

	return;
}
