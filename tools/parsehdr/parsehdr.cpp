/*
 *$Log:	parsehdr.c,v $
 */
/* Code to parse a header (.h) file */
/* Descended from xansi; thanks Geoff! thanks Glenn! */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <malloc.h>     /* For debugging */
#include "parsehdr.h"

dword	userval;

/* the IGNORE byte */
byte	slosh;
byte	last_slosh;
byte	quote1;
byte	quote2;
byte	comment;
byte	hash;
byte    ignore1;        /* Special: ignore egactly 1 char */
byte	double_slash;
byte	spare;

int		buff_idx;
char	buffer[BUFF_SIZE];

byte	start;			/* Started recording to the buffer */
byte	func;			/* Function header detected */
byte	hash_ext;
int		curly; 			/* Level inside curly brackets */
int		xtern;			/* Level inside a extern "C" {} situation */
int		round;			/* Level inside () */
int		line,col;
dword	chars;
char	lastch;

#define NIL         -1  /* Used as an illegal index */

FILE    *datFile;       /* Stream of the data (output) file */
char    fileName[81];   /* Name of current header file */

PH_FUNC_STRUCT *pFunc;  /* Pointer to the functions array */
int     numFunc;        /* How many elements saved so far */
int     allocFunc;      /* How many elements allocated so far */
int     headFunc;       /* Head of the function name linked list */

PH_ARG_STRUCT *pArg;    /* Pointer to the arguements array */
int     numArg;         /* How many elements saved so far */
int     allocArg;       /* How many elements allocated so far */
int     headArg;        /* Head of the arguements linked list */


// DO Callback
boolT
phDoCB(int id, char *data)
{
/*	return callback(hDCX, id, data, userval);*/
	return TRUE;
}


void
phError(char *errmsg)
{
	char	msg[200];

	sprintf(msg,"PH *ERROR*\nFile: %s L=%d C=%d O=%lu\n%s", fileName, line, col,
		chars, errmsg);
	printf(msg);
}


void
phWarning(char *errmsg)
{
	char	msg[200];

	sprintf(msg,"PH -warning-\nFile: %s L=%d C=%d O=%lu\n%s\n", fileName, line,
        col, chars, errmsg);
	printf(msg);
}




int
IsIgnore()
{
	return(	comment ||
			quote1  ||
			quote2  ||
			slosh   ||
			hash    ||
            ignore1 ||
			double_slash
		);
}

boolT
isAlphaNum(char ch)
{
	return(((ch >= 'A') && (ch <= 'Z'))	||
           ((ch >= 'a') && (ch <= 'z'))	||
           ((ch >= '0') && (ch <= '9'))	||
            (ch == '_')
		  );
}


boolT
AddToBuffer(char ch)
{
	if (buff_idx >= BUFF_SIZE)
	{
		ERR("function buffer overflow (function unterminated?)\n");
		return FALSE;
	}
	else
	{
		buffer[buff_idx++] = ch;
		return TRUE;
	}
}

boolT
remFromBuffer()
{
	if (buff_idx == 0)
	{
		ERR("function buffer underflow (internal error?)\n");
		return FALSE;
	}
	else
	{
		buff_idx--;
		return TRUE;
	}
}





/*----------------------------------------------*/
/* This is a function declaration, typedef, etc.*
/* 			Do something with it. 				*/
/*----------------------------------------------*/

void
ProcessBuffer(int id)
{
	if (buff_idx > 0)
	{
		buffer[buff_idx] = '\0';

		// CALL CALL BACK FUNTION WITH APPRORIATE CODE!

		switch (id)
		{
			case PH_FUNCTION:
				// eek, but...
			case PH_PROTO:
				// sort out into params etc
				phBuffToFunc(buffer);
				break;

			case PH_TYPEDEF:
			case PH_DEFINE:
				// sort out into defs
				phBuffToDef(buffer);
				break;

			case PH_MPROTO:
				// eek!

			case PH_VAR:
				// eek!

			case PH_JUNK:
			default:
				phDoCB(id, buffer);
				break;
		}
		start = FALSE;
		func = FALSE;
		buff_idx = 0;
	}
}


void
phInit(char *filename)		// filename is for reference only!!!
{
	slosh =
	last_slosh =
	start =
	func =
	comment =
	double_slash =
	hash =
    ignore1 =
	quote1 =
	quote2 =
	hash_ext =
	FALSE;

	buff_idx =
	curly =
	xtern =
	col =
	round = 0;

	line = 1;

	chars = 0L;

	lastch = 0;

}



boolT
phFree(void)
{
	// remove atoms etc, free buffers
	return TRUE;
}






void
phChar(char ch)
{
	col++;
	chars++;
/*	printf("%d%% done\r", chars*100/filelen); /**/


	if (slosh != last_slosh)
	{
		DBG("[SLOSH OFF]");
	}

	switch(ch)
	{
		case ',':
			if (!IsIgnore() && (curly == xtern) && (start) && (func))	
			/* must be multi proto */
			{
				if (lastch == ')')	/* eg int foo(), bar(); */
				{
					ProcessBuffer(PH_MPROTO);
					DBG("[END OF MULTIPROTOTYPE]")
				}
			}
			break;

		case ';':
			if (!IsIgnore() && (curly == xtern) && (start))
			{
				if (func)
				{
					if (lastch == ')')
					{
						ProcessBuffer(PH_PROTO);
						DBG("[END OF PROTOTYPE]")
					}
				}
				else
				{
					ProcessBuffer(PH_VAR);
					DBG("[END OF VARIABLE]")
				}
			}
			break;

		case 10:	/* end of line */
			line++;
/* 			chars++;	*/ /* must have been a CR before it methinks */
			col = 0;
			if (double_slash)
			{
				double_slash = FALSE;
				DBG("[DOUBLE_SLASH_COMMENT OFF]")
			}
			else if (hash)
			{
				if (hash_ext)
				{
					hash_ext = FALSE;
				}
				else
				{
					hash = FALSE;
					DBG("[HASH OFF]")
				}
			}
            if (xtern && (strncmp(buffer, "extern", 6) == 0))
            {
                start = FALSE;          /* Not the start of anything */
                buff_idx = 0;           /* Kill the buffer */
            }
			break;

		case '#':	/* start of # something at beginning of line */
			if ((!IsIgnore()) && (curly == xtern))
			{
				hash = TRUE;
				DBG("[HASH ON]")
			}
			break;

		case '{':
			if (!IsIgnore())
			{
				char	st[80];

				if ((curly == xtern) && (start) && (func))
				{
					ProcessBuffer(PH_FUNCTION);
					DBG("[FUNCTION DECLARED]")
				}

				curly++;
				sprintf(st, "[CURLY++ %d]", curly);
				DBG(st)
			}
			break;

		case '}':
			if (!IsIgnore())
			{
				char	st[80];

				if (curly > 0)
				{
					if (xtern && (xtern == curly))
					{
						xtern = 0;
						DBG("[EXTERN OFF]");
					}
					curly--;
					sprintf(st,"[CURLY-- %d]", curly);
					DBG(st)
				}
				else
				{
                    /* match the {s */
					ERR("too many \"}\"\n");
				}
			}
			break;

		case '(':
			if (!IsIgnore())
			{
				char	st[80];

				if ((curly == xtern) && (round == 0) && (start))
				{
					func = TRUE;
					DBG("[FUNCTION]")
				}
				round++;
				sprintf(st,"[ROUND++ %d]", round);
				DBG(st)
			}
			break;

		case ')':
			if (!IsIgnore())
			{
				char	st[80];

				if (round > 0)
				{
					round--;
					sprintf(st,"[ROUND-- %d]",round);
					DBG(st)
				}
				else
				{
					ERR("too many \")\"\n");
				}
			}
			break;

		case '\\':
			if (!slosh && (quote1 || quote2))
			{
				last_slosh = TRUE;
				DBG("[SLOSH ON]")
			}
			else
			if (hash)
			{
				hash_ext = TRUE;
			}
			break;

		case '*':
			if (lastch == '/')	/* allow nested comments ! */
			{
				char	st[80];

				comment++;

				if (start)
				{
					remFromBuffer();
				}

				sprintf(st, "[COMMENT++ %d]",comment);
				DBG(st)


			}
			break;

		case '/':
			if ((lastch == '*') && (!quote1) && (!quote2) )
			{
				if (comment > 0)
				{
				char	st[80];

					comment--;

                    /* Don't want the closing slash in the buffer */
                    ignore1 = TRUE;

					sprintf(st, "[COMMENT-- %d]",comment);
					DBG(st)
				}
				else
				{
					ERR("too many \"*/\"\n");
				}
			}
			else
			if (lastch == '/')
			{
				/* Double slash to end of line is a comment. */
				double_slash = TRUE;

				if (start)
				{
					remFromBuffer();
				}

				DBG("[DOUBLE_SLASH_COMMENT ON]")
			}
			break;

		case '\"':
			if ((!comment) && (!quote1) && (!slosh))
			{
				quote2 = (byte) (!quote2);
				if (quote2)	DBG("[QUOTE2ON]")
				if (!quote2)	DBG("[QUOTE2OFF]")

				/* We want to catch the extern "C" {} thing... */
				if (!quote2 && start && (lastch == 'C'))
				{
					if (strcmp(buffer, "extern ") == 0)
					{
						char st[80];

						xtern = curly+1;	/* The level inside the extern {} */
						sprintf(st, "[EXTERN ON %d]", xtern);
						DBG(st)
					}
				}
			}
			break;

		case '\'':
			if ((!comment) && (!quote2) && (!slosh))
			{
				{
					quote1 = (byte) (!quote1);
					if (quote1)	DBG("[QUOTE1ON]")
					if (!quote1)	DBG("[QUOTE1OFF]")
				}
			}
			break;

		case '\t':
			ch = ' ';
			break;


		default:
			if ((ch != -1) && !IsIgnore() && (curly == xtern) && (!start) && 
				(ch != ' '))
			{
				start = TRUE;
				DBG("[START OF SOMETHING]")
			}
			break;


	}

	if (ch != -1)
	{
		if (start && !IsIgnore())
		{
			AddToBuffer(ch);
		}
	}

	lastch = ch;
	slosh = last_slosh;
	last_slosh = 0;
    ignore1 = FALSE;

}	/* of phChar */


/* Take a lump of data from a header file, and churn the state machine
    through each char */
boolT
phData(char *buff, int ndata)
{
	int		i, j;
#ifdef DEBUG
	char	cLine[81];
    char    cfLine[90];
#endif

	if (ndata < 1)
	{
		ndata = strlen(buff);
	}
	j = 0;
	
	for (i=0; i < ndata; i++)
	{
		phChar(buff[i]);
#ifdef DEBUG
		if (j < 80) cLine[j++] = buff[i];
		if (buff[i] == '\n')
		{
			cLine[j] = '\0';
			sprintf(cfLine, "\n***%03d %s\n", line, cLine);
            DBG(cfLine);
			j = 0;
		}
#endif
	}

	return TRUE;
}


boolT
phPost(void)
{
	boolT	err=TRUE;
	char	msg[80];

	if (quote1)
	{
		WARN("EOF: \' not closed");
		err = FALSE;
	}

	if (quote2)
	{
		WARN("EOF: \" not closed");
		err = FALSE;
	}

	if (comment)
	{
		WARN("EOF: comment not closed");
		err = FALSE;
	}

	if (slosh)
	{
		WARN("EOF: internal slosh set error");
		err = FALSE;
	}

	if (curly > 0)
	{
		sprintf(msg,"EOF: { level = %d", curly);
		WARN(msg);
		err = FALSE;
	}

	if (round > 0)
	{
		sprintf(msg,"EOF: ( level = %d", round);
		WARN(msg);
		err = FALSE;
	}

	if (hash)
	{
		WARN("warning hash is set on last line ???");
		err = FALSE;
	}

	return err;
}



#if 0
enum FUNC_E { NORM , LEFT , RIGHT, NEXT };

void
NamesToLabel(LPPH_FUNC lpF)
{
int	i, j;

	for (i=0; i < lpF->num_params; i++)
	{
		while (isspace(types[i][0]))
		{
			lstrcdel(types[i], 0);
		}

		j = 0;
		while(names[i][j] != '\0')
		{
			if ((names[i][j] != '*')	&&
					(names[i][j] != '[') &&
					(names[i][j] != ']'))
			{
				lstrccat(label[i], names[i][j]);
			}
			j++ ;
		}
	}
}

boolT
MoveLastWord(char *src, char *dest)		/* take arg name from type  */
{
int		i, l;
LPSTR	s2;

	if (s2 = strchr(src, '*'))	/* if there is a * use it as starting point */
	{
		lstrcpy(dest, s2);
		*s2 = '\0';	/* terminate */
	}
	else
	{
		l = strlen(src);
		i = l-1;
		while ((i > 0) && ((isspace(src[i])) || (src[i] == '[') || (src[i] == ']')))
		/* find last non space or non [] */
		{
			if (isspace(src[i]))
			{
				lstrcdel(src, i);
			}	/* remove trailing spaces */
			i--;
		}
		while ((i > 0) && (!isspace(src[i])))	/* find the previous space */
		{
			i--;
		}
		if (i)
		{
			i++;
			lstrcpy(dest, &src[i]);
			src[i] = '\0';		/* terminate */
		}
		else
		{
			/* no type !!! */

//			if ((mode == TOANSI) ||
//						((mode == FROMANSI) && (strstr(src, "...") == NULL))
//				 )

			if (strstr(src, "...") == NULL)	// not a var arg perhaps?
			{
			char	msg[80];

//				sprintf(msg,"no type for arg # %d, \"%s\"\n",lpF->num_params, (LPSTR) src);
//				ERR(msg);
				return FALSE;
			}
			else	// d'oh!
			{
				WARN("looks like a vararg to me!");
				/* maybe it is a ... !!!!!! */
			}
		}
	}


	i = strlen(src) - 1;

	while ((isspace(src[i])) && (i >= 0))
	{
		src[i--] = '\0';
	}

	i = 0;
	while (dest[i] != '\0')
	{
		if (isspace(dest[i]))
		{
			lstrcdel(dest, i);
		}
		i++ ;
	}

	while ((isspace(src[0])) && (src[0] != '\0'))
	{
		lstrcdel(src, 0);
	}

	return TRUE;
}




int
Lookup(char *aname)	/* lookup var name in labels and return arg number */
{
int		i, p;
char	tname[NAMES_L];
int		bstate = FALSE;

/* eg: lookup *fred[] for match to fred */

	tname[0] = '\0';
	p = -1;		/* default return for no match */

	/* tname is aname without puncs etc */
	i = 0;
	while(aname[i] != '\0')
	{
		if (aname[i] == '[')
		{
			bstate = TRUE;
		}
		else
		if (aname[i] == ']')
		{
			bstate = FALSE;
		}

		if ((isalnum(aname[i]) || aname[i] == '-' || aname[i] == '_') && (!bstate))
		{
			lstrccat(tname, aname[i]);
		}
		i++;
	}

	/* lookup tname in the labels and find out which arg it is */
	for (i=0; i < num_params; i++)
	{
		if (lstrcmp(tname, label[i]) == 0)
		{
			p = i;	/* this one ! */
			break;
		}
	}
	return p;
}


/* put the name and type at correct arg number */


boolT
Plop(char *atype, char *aname, int FAR *num_params, int FAR *num_found)
{
char	msg[80];
int	t;

	if (num_found >= num_params)
	{
		sprintf(msg,"extra argument \"%s\" in func \"%s\"\n", (LPSTR) aname, (LPSTR) func_name);
		ERR(msg);
		return FALSE;
	}

	t = Lookup(aname);	/* arg number */

	if (t == -1)	/* couldn't find it */
	{
		sprintf(msg,"bad argument \"%s\" in func \"%s\"\n", (LPSTR) aname, (LPSTR) func_name);
	  	ERR(msg);
		return FALSE;
	}

	if ((strlen(types[t]) > 0) || (strlen(names[t]) > 0))	/* in use ? */
	{
		
		sprintf(msg,"argument \"%s\" already used in \"%s\"\n", (LPSTR) aname, (LPSTR) func_name);
		ERR(msg);
		return FALSE;
	}

	lstrcpy(types[t], atype);
	lstrcpy(names[t], aname);

	num_found++;		/* got another! */

	return TRUE;
}

#define	IGN ((*chp == '(') || (*chp == ')') || (*chp == ',') || \
						(*chp == ';') || (isspace(*chp)))

#define	IGNP ((*chp == '(') || (*chp == ')') || (*chp == ',') || \
						(*chp == ';') || (*chp == '\n'))

#endif







char    token[40];      /* Strings that might be types, nodifiers or idents */
char    ident[40];      /* Names of functions or protos go here */
char    lastChar;
char   *p;
int     indirect;
boolT   isLong, isShort, isUnsigned;
int     lastTokPos;     /* For "^" in error messages */
char   *buffP;
int     tok;            /* Current token */
baseType bt;            /* Type of current param (or return type) */
int     argNum;         /* Arg number (in case no name: arg1, arg2...) */


void
initType(void)
{
    indirect = 0;
    isLong = isShort = isUnsigned = FALSE;
	bt = BT_INT;
}

void
errorParse(char *msg)
{
    printf("%s: got ", msg);
    if (tok == TOK_NAME) printf("<%s>", token);
    else if (tok == TOK_DOTS) printf("...");
    else printf("%c (%X)", tok, tok);
    printf("\n%s\n", buffP);
    printf("%*c\n", lastTokPos+1, '^');
}


/* Get a token from pointer p */
int
getToken(void)
{
    char ch;

    memset(token, 0, sizeof(token));
    while (*p && ((*p == ' ') || (*p == '\n'))) p++;
    lastTokPos = p - buffP;             /* For error messages */
    if (lastChar)
    {
        ch = lastChar;
        lastChar = '\0';
        return ch;
    }

    while (ch = *p++)
    {
        switch (ch)
        {
            case '*':
            case '[':
            case ']':
            case '(':
            case ')':
            case ',':
            case ';':
            case ' ':
            case '\n':
                if (strlen(token))
                {
                    if ((ch != ' ') && (ch != '\n')) lastChar = ch;
                    return TOK_NAME;
                }
                else if ((ch == ' ') || (ch == '\n')) break;
                else return ch;

            case '.':
                if ((*p == '.') && (p[1] == '.'))
                {
                    p += 2;
                    return TOK_DOTS;
                }
                    

            default:
                token[strlen(token)] = ch;
        }
    }
    return TOK_EOL;
}

boolT
isBaseType(void)
{
    if (tok != TOK_NAME) return FALSE;

    if (strcmp(token, "int") == 0)
    {
        bt = BT_INT;
    }
    else if (strcmp(token, "char") == 0)
    {
        bt = BT_CHAR;
    }
    else if (strcmp(token, "void") == 0)
    {
        bt = BT_VOID;
    }
    else if (strcmp(token, "float") == 0)
    {
        bt = BT_FLOAT;
    }
    else if (strcmp(token, "double") == 0)
    {
        bt = BT_DOUBLE;
    }
    else if (strcmp(token, "struct") == 0)
    {
        bt = BT_STRUCT;
        tok = getToken();       /* The name of the struct */
        /* Do something with the struct name */
    }
    else if (strcmp(token, "union") == 0)
    {
        bt = BT_STRUCT;         /* Well its still a struct */
        tok = getToken();       /* The name of the union */
        /* Do something with the union name */
    }
    else if (strcmp(token, "FILE") == 0)
    {
        bt = BT_STRUCT;
    }
    else if (strcmp(token, "size_t") == 0)
    {
        bt = BT_INT;
        isUnsigned = TRUE;
    }
    else if (strcmp(token, "va_list") == 0)
    {
        bt = BT_VOID;
        indirect = 1;           /* va_list is a void* */
    }
    else return FALSE;
    return TRUE;
}

boolT
isModifier(void)
{
    if (tok != TOK_NAME) return FALSE;
    if (strcmp(token, "long") == 0)
    {
        isLong = TRUE;
    }
    else if (strcmp(token, "unsigned") == 0)
    {
        isUnsigned = TRUE;
    }
    else if (strcmp(token, "short") == 0)
    {
        isShort = TRUE;
    }
    else if (strcmp(token, "const") == 0)
    {

    }
    else if (strcmp(token, "_far") == 0)
    {

    }
    else return FALSE;
    return TRUE;
}

boolT
isAttrib(void)
{

    if (tok != TOK_NAME) return FALSE;
    if (strcmp(token, "far") == 0)
    {
        /* Not implemented yet */
    }
    else if (strcmp(token, "__far") == 0)
    {
        /* Not implemented yet */
    }
    else if (strcmp(token, "__interrupt") == 0)
    {
        /* Not implemented yet */
    }
    else return FALSE;
    return TRUE;
}


boolT
isCdecl(void)
{
    return (
        (strcmp(token, "__cdecl") == 0)  ||
        (strcmp(token, "_Cdecl") == 0) ||
        (strcmp(token, "cdecl") == 0));
}

void
getTypeAndIdent(void)
{
    /* Get a type and ident pair. Complicated by the fact that types are
        actually optional modifiers followed by types, and the identifier
        is also optional. For example:
            myfunc(unsigned footype *foovar);
        declares an arg named foovar, of type unsigned pointer to footype.
        But we don't exand typedefs and #defines, so the footype may not
        be recognised as a type. Then it is not possible to know whether
        footype is an identifier or an unknown type until the next token is
        read (in this case, the star). If it is a comma or paren, then footype
        is actually an identifier. (This function gets called for the function
        return type and name as well, so "(" is possible as well as ")" and
        ",").
        The identifier is copied to ident.
    */

    boolT im = FALSE, ib;

    while (isModifier())
    {
        tok = getToken();
        im = TRUE;
    }

    if (!im && (tok != TOK_NAME))
    {
        errorParse("Expected type");
    }

    ib = isBaseType();
    if (ib) tok = getToken();

    /* Could be modifiers like "far", "interrupt" etc */
    while (isAttrib())
    {
        tok = getToken();
    }

    while (tok == '*')
    {
        indirect++;
        tok = getToken();
    }

    /* Ignore the cdecl's */
    while (isCdecl()) tok = getToken();

    if (tok == TOK_NAME)
    {
        /* This could be an ident or an unknown type */
        strcpy(ident, token);
        tok = getToken();
    }

    if (!ib && (tok != ',') && (tok != '(') && (tok != ')'))
    {
        /* That was (probably) not an ident! Assume it was an unknown type */
printf("Unknown type %s\n", ident);
        ident[0] = '\0';
        bt = BT_UNKWN;

        while (tok == '*')
        {
            indirect++;
            tok = getToken();
        }
    
        /* Ignore the cdecl's */
        while (isCdecl()) tok = getToken();
    }

    if (tok == TOK_NAME)
    {
        /* This has to be the ident */
        strcpy(ident, token);
        tok = getToken();
    }

    while (tok == '[')
    {
        indirect++;             /* Treat x[] like *x */
        do
        {
            tok = getToken();   /* Ignore stuff between the '[' and ']' */
        }
        while (tok != ']');
        tok = getToken();
    }

}


hlType
convType(void)
{
    /* Convert from base type and signed/unsigned flags, etc, to a htType
        as Cristina currently uses */

    if (indirect >= 1)
    {
        if (bt == BT_CHAR) return TYPE_STR; /* Assume char* is ptr */
        /* Pointer to anything else (even unknown) is type pointer */
        else return TYPE_PTR;
    }
    switch (bt)
    {
        case BT_INT:
            if (isLong)
            {
                if (isUnsigned) return TYPE_LONG_UNSIGN;
                else return TYPE_LONG_SIGN;
            }
            else
            {
                if (isUnsigned) return TYPE_WORD_UNSIGN;
                else return TYPE_WORD_SIGN;
            }

        case BT_CHAR:
            if (isUnsigned) return TYPE_BYTE_UNSIGN;
            else return TYPE_BYTE_SIGN;

        case BT_FLOAT:
            return TYPE_FLOAT;
        case BT_DOUBLE:
            return TYPE_DOUBLE;

        case BT_STRUCT:
            return TYPE_RECORD;

        case BT_VOID:
        default:
            return TYPE_UNKNOWN;
    }
}


/* Add a new function to the array of function name and return types. The
    array is logically sorted by a linked list. Note that numArg is filled
    in later */
boolT
addNewFunc(char *name, hlType typ)
{
    int i, prev, res;

    /* First see if the name already exists */
    prev = NIL;
    for (i=headFunc; i != NIL; i = pFunc[i].next)
    {
        res = strcmp(pFunc[i].name, name);
        if (res > 0)
        {
            break;          /* Exit this loop when just past insert point */
        }
        if (res == 0)
        {
            /* Already have this function name */
            return TRUE;
        }
        prev = i;
    }

    if (numFunc >= allocFunc)
    {
        allocFunc += DELTA_FUNC;
        pFunc = realloc(pFunc, allocFunc * sizeof(PH_FUNC_STRUCT));
        if (pFunc == NULL)
        {
            fprintf(stderr, "Could not allocate %ud bytes for function array\n",
                allocFunc * sizeof(PH_FUNC_STRUCT));
            exit(1);
        }
        memset(&pFunc[allocFunc - DELTA_FUNC], 0,
            DELTA_FUNC * sizeof(PH_FUNC_STRUCT));
    }

    name[SYMLEN-1] = '\0';
    strcpy(pFunc[numFunc].name, name);
    pFunc[numFunc].typ = typ;
    pFunc[numFunc].firstArg = numArg;
    if (prev == NIL)
    {
        pFunc[numFunc].next = headFunc;
        headFunc = numFunc;
    }
    else
    {
        pFunc[numFunc].next = pFunc[prev].next;
        pFunc[prev].next = numFunc;
    }
    numFunc++;

    return FALSE;
}

/* Add a new arguement to the array of arguement name and types. The
    array is logically sorted by a linked list */
void
addNewArg(char *name, hlType typ)
{
    if (numArg >= allocArg)
    {
        allocArg += DELTA_FUNC;
        pArg = realloc(pArg, allocArg * sizeof(PH_ARG_STRUCT));
        if (pArg == NULL)
        {
            fprintf(stderr, "Could not allocate %ud bytes for arguement array\n",
                allocArg * sizeof(PH_ARG_STRUCT));
            exit(1);
        }
        memset(&pArg[allocArg - DELTA_FUNC], 0,
            DELTA_FUNC * sizeof(PH_ARG_STRUCT));
    }
    name[SYMLEN-1] = '\0';
    strcpy(pArg[numArg].name, name);
    pArg[numArg].typ = typ;
    numArg++;

}

void
parseParam(void)
{
    initType();
    if (tok == TOK_DOTS)
    {
        tok = getToken();
        pFunc[numFunc-1].bVararg = TRUE;
        return;
    }

    getTypeAndIdent();

    if ((bt == BT_VOID) && (indirect == 0))
    {
        /* Just a void arg list. Ignore and pFunc[].numArgs will be set to zero */
        return;
    }
    argNum++;
    if (ident[0])
    {
        addNewArg(ident, convType());
    }
    else
    {
        sprintf(ident, "arg%d", argNum);
        addNewArg(ident, convType());
    }

}


/* Parse the prototype as follows:
<type> [<cdecl>] ["*"]... <funcName>"("<paramDef> [","<paramDef>]...")"
where <type> is
["const"] [<modifier>] [<basetype>]          modifier could be short, long, far
and where paramdef is
["const"] <type> ["*"]... [<paramName>]   or   "..."
Note that the closing semicolon is not seen.
*/


void
phBuffToFunc(char *buff)
{

    initType();
    p = buffP = buff;
    tok = getToken();

    /* Ignore typedefs, for now */
    if ((tok == TOK_NAME) && (strcmp(token, "typedef") == 0)) return;

    getTypeAndIdent();

    if (ident[0] == '\0')
    {
        errorParse("Expected function name");
        return;
    }

    if (addNewFunc(ident, convType()))
    {
        /* Already have this prototype, so ignore it */
        return;
    }

    if (tok != '(')
    {
        errorParse("Expected '('");
        return;
    }
    tok = getToken();

    argNum = 0;
    while (tok != TOK_EOL)
    {
        parseParam();
        if ((tok != ',') && (tok != ')'))
        {
            errorParse("Expected ',' between parameter defs");
            return;
        }
        tok = getToken();
    }
    pFunc[numFunc-1].numArg = argNum;       /* Number of args this func */
}


void
phBuffToDef(char *buff)
{

}


void
writeFile(char *buffer, int len)
{
	if ((int)fwrite(buffer, 1, len, datFile) != len)
	{
		printf("Could not write to file\n");
		exit(1);
	}
}


void
writeFileShort(word w)
{
	byte b;

	b = (byte)(w & 0xFF);
	writeFile(&b, 1);		/* Write a short little endian */
	b = (byte)(w>>8);
	writeFile(&b, 1);
}


void
saveFile(void)
{
    int i;

    fprintf(datFile, "dccp");       /* Signature */
    fprintf(datFile, "FN");         /* Function name tag */
    writeFileShort(numFunc);        /* Number of func name records */
    for (i=headFunc; i != NIL; i = pFunc[i].next)
    {
        writeFile(pFunc[i].name, SYMLEN);
        writeFileShort((word)pFunc[i].typ);
        writeFileShort((word)pFunc[i].numArg);
        writeFileShort((word)pFunc[i].firstArg);
        writeFile((char *)&pFunc[i].bVararg, 1);
    }

    fprintf(datFile, "PM");         /* Parameter Name tag */
    writeFileShort(numArg);         /* Number of args */
    for (i=0; i < numArg; i++)
    {
/*      writeFile(pArg[i].name, SYMLEN);    /* Don't want names yet */
        writeFileShort((word)pArg[i].typ);
    }

}

void
main(int argc, char *argv[])
{
	char   *buf;
	long   fSize;
    int    ndata;
	FILE   *f, *fl;
    int    i;
    char   *p;

    if (argc != 2)
    {
        printf("Usage: parsehdr <listfile>\n"
            "where <listfile> is a file of header file names to parse.\n"
            "The file dcclibs.dat will be written\n");
        exit(1);
    }

	fl = fopen(argv[1], "rt");
	if (fl == NULL)
	{
		printf("Could not open file list file %s\n", argv[1]);
		exit(1);
	}

    datFile = fopen("dcclibs.dat", "wb");
    if (datFile == NULL)
	{
		printf("Could not open output file dcclibs.dat\n");
		exit(2);
	}

    /* Allocate the arrys for function and proto names and types */
    pFunc = malloc(DELTA_FUNC * sizeof(PH_FUNC_STRUCT));
    if (pFunc == 0)
    {
        fprintf(stderr, "Could not malloc %ud bytes for function name array\n",
            DELTA_FUNC * sizeof(PH_FUNC_STRUCT));
        exit(1);
    }
    memset(pFunc, 0, DELTA_FUNC * sizeof(PH_FUNC_STRUCT));
    allocFunc = DELTA_FUNC;
    numFunc = 0;

    pArg = malloc(DELTA_FUNC * sizeof(PH_ARG_STRUCT));
    if (pArg == 0)
    {
        fprintf(stderr, "Could not malloc %ud bytes for arguement array\n",
            DELTA_FUNC * sizeof(PH_ARG_STRUCT));
        exit(1);
    }
    memset(pArg, 0, DELTA_FUNC * sizeof(PH_ARG_STRUCT));
    allocArg = DELTA_FUNC;
    numArg = 0;

    headFunc = headArg = NIL;

    buf = NULL;
    while (!feof(fl))
    {
        /* Get another filename from the file list */
        p = fgets(fileName, 80, fl);
        if (p == NULL) break;       /* Otherwise read last filename twice */
        i = strlen(fileName)-1;
        if (fileName[i] == '\n') fileName[i] = '\0';
	    f = fopen(fileName, "rt");
	    if (f == NULL)
	    {
		    printf("Could not open header file %s\n", fileName);
		    exit(1);
	    }

        printf("Processing %s...\n", fileName);

	    fSize = _lseek(_fileno(f), 0, SEEK_END);
	    fseek(f, 0, SEEK_SET);
        ndata = (int) min(fSize, FBUF_SIZE);
        if (buf) free(buf);
	    buf = (char *)malloc(ndata);
	    if (buf == 0)
	    {
		    printf("Could not malloc input file buffer of %d bytes\n", ndata);
		    exit(1);
	    }

        while (!feof(f))
        {
	        ndata = fread(buf, 1, ndata, f);
	        phData(buf, ndata);
        }
        phPost();
        fclose(f);
    }
    saveFile();
    fclose(datFile);
    fclose(fl);

    free(buf);
    free(pFunc);
    free(pArg);
}


#if CHECK_HEAP
void
checkHeap(char *msg)

/* HEAPCHK.C: This program checks the heap for
 * consistency and prints an appropriate message.
 */
{
   int  heapstatus;

   printf("%s\n", msg);

   /* Check heap status */
   heapstatus = _heapchk();
   switch( heapstatus )
   {
   case _HEAPOK:
      printf(" OK - heap is fine\n" );
      break;
   case _HEAPEMPTY:
      printf(" OK - heap is empty\n" );
      break;
   case _HEAPBADBEGIN:
      printf( "ERROR - bad start of heap\n" );
      break;
   case _HEAPBADNODE:
      printf( "ERROR - bad node in heap\n" );
      break;
   }
}

#endif

