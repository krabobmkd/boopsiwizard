#include "templates.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <zlib.h>


#include "cJSON.h"

#include <proto/exec.h>
#include <proto/dos.h>

#include <dos/dos.h>


static sWizTemplate *gFirstTemplate=NULL;
static int nbTemplates = 0;
int getNbTemplates(){ return nbTemplates;  }


// copy json string to our struct, using AllocVec
static void getJsString(char **p, cJSON *pm)
{
    if(!pm || !p) return;
    if(*p) FreeVec(*p);
    if( !cJSON_IsString(pm)) return;

    const char *ps = cJSON_GetStringValue(pm);
    if(!ps) return;

    int l = strlen(ps);
    *p = AllocVec(l+1,0);
    if(!(*p)) return;
    strcpy(*p,ps);
    (*p)[l]=0;
}

// copy json string to our struct, using AllocVec
static void getJsStringInObj(char **p, cJSON *jsobj, const char *key )
{
    if(*p != NULL) {
        FreeVec(*p);
        *p = NULL;
    }
    if(!jsobj) return;
    cJSON *pm = cJSON_GetObjectItem(jsobj,key);
    if(!pm) return;   
    getJsString(p,pm);
}
static void closeStringArray( sStringArray *p)
{
    if(!p) return;
    if(p->_p == NULL) return;
    for(int i=0;i<p->_nb;i++)
    {
        if(p->_p[i])
        {
         //printf("subclose:%s\n",p->_p[i]);
         FreeVec(p->_p[i]);
        }
    }
    FreeVec(p->_p);
    p->_p = NULL;
}
static void getJsStringArray( sStringArray *p, cJSON *jsobj, const char *key )
{
    if(!p) return;
    if(p->_p != NULL) {
        closeStringArray(p);
    }
    if(!jsobj) return;
    cJSON *pm = cJSON_GetObjectItem(jsobj,key);
    if(!pm) return;
    if( !cJSON_IsArray(pm)) return;

    int nb = cJSON_GetArraySize(pm);
    if(nb<=0)
    {
        p->_nb = 0;
        return;
    }
    p->_nb = nb;
    p->_p = AllocVec(sizeof(char *)*nb,MEMF_CLEAR);
    if(!p->_p)
    {   //todo tells it's alloc fault.
     p->_nb = 0;
     return;
    }
    for(int i=0;i<nb;i++)
    {
        cJSON *psub = cJSON_GetArrayItem(pm,i);
        if(!psub) continue;
        getJsString(p->_p+i,psub);
    }

}

static int scanTemplates(BPTR lock, struct FileInfoBlock*fib)
{

    nbTemplates = 0;
    if(!Examine(lock, fib)) return 0;
/*
The allocator used by cJSON_Parse is malloc and free
by default but can be changed (globally) with cJSON_InitHooks.
*/
    if(fib->fib_DirEntryType <= 0) return 0; // if >0, a directory


    while(ExNext(lock, fib))
    {
        if(fib->fib_DirEntryType <= 0)
        {   // is a file.
            // if end with .json
            // fast, no alloc version
            char *p = fib->fib_FileName;
            int l =strlen(p);
            if(l>6 && p[l-5]=='.' &&
                (p[l-4]=='j' || p[l-4]=='J' ) &&
                (p[l-3]=='s' || p[l-4]=='S' ) &&
                (p[l-2]=='o' || p[l-4]=='O' )
                )
            {
                char *pmem = AllocVec(fib->fib_Size+1,0);
                if( pmem )
                {
                    char temp[256];
                    temp[0] = 0;
                    strcat(temp,"PROGDIR:templates/");
                    strcat(temp,fib->fib_FileName);
                    //printf("filesize:%d\n",fib->fib_Size);
                    BPTR fh = Open(temp,MODE_OLDFILE);

// printf("open:%s\n",temp);
                    if(fh)
                    {
                        Read(fh,pmem,fib->fib_Size);
                        pmem[fib->fib_Size]=0;
                        Close(fh);
                    }
                    cJSON *jsroot = cJSON_Parse(pmem); //cJSON_CreateObject();
                    if (jsroot == NULL)
                    {
                        const char *error_ptr = cJSON_GetErrorPtr();
                        if (error_ptr != NULL)
                        {
                            printf("error file: %s\n",fib->fib_FileName);
                            printf("template json error before: %s\n", error_ptr);
                        }
                        FreeVec(pmem);
                        continue;
                    }
                    cJSON *jstemplate = cJSON_GetObjectItemCaseSensitive(jsroot, "template");
                    if(jstemplate)
                    {
                        sWizTemplate *ntmpl = AllocVec(sizeof(sWizTemplate),MEMF_CLEAR);
                        if(ntmpl)
                        {
                    //printf("+1 tmpl\n");

                            ntmpl->_pNext = gFirstTemplate;
                            gFirstTemplate = ntmpl;

                            getJsStringInObj(&(ntmpl->_displayName),jstemplate,"displayname");
                            getJsStringInObj(&(ntmpl->_versionstring),jstemplate,"versionstr");
                            getJsStringInObj(&(ntmpl->_archivename),jstemplate,"archive");
                            getJsStringInObj(&(ntmpl->_defaultname),jstemplate,"defaultname");
                            getJsStringInObj(&(ntmpl->_comment),jstemplate,"comment");

                            getJsStringArray(&(ntmpl->_renames),jstemplate,"renames");

                            //if(ntmpl->_comment) printf(ntmpl->_comment);
                            nbTemplates++;

                        }
                    } else
                    {
                        FreeVec(pmem);
                        printf("error file: %s\n",fib->fib_FileName);
                        printf("json syntax ok but no template chapter.\n");
                         continue;
                    }
                    cJSON_Delete(jsroot);
                    FreeVec(pmem);

                }
            }
        } // end if is file.

    } // end loop per dir file



    return nbTemplates;
}




static void closeTemplates()
{
    sWizTemplate *pt = gFirstTemplate;
    while(pt)
    {
        sWizTemplate *ptnext = pt->_pNext;
        if(pt->_displayName) FreeVec(pt->_displayName);
        if(pt->_versionstring) FreeVec(pt->_versionstring);
        if(pt->_archivename) FreeVec(pt->_archivename);
        if(pt->_defaultname) FreeVec(pt->_defaultname);
        if(pt->_comment) FreeVec(pt->_comment);

        if(pt->_renames._nb>0) closeStringArray(&pt->_renames);

        FreeVec(pt);
        pt = ptnext;
    }
}

void initTemplates(template_notifier n)
{

    atexit(&closeTemplates);

    struct FileInfoBlock *fib;
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if(!fib) return;


    BPTR lock = Lock( "PROGDIR:templates" , ACCESS_READ);
    if(lock)
    {
        scanTemplates(lock,fib);
        UnLock(lock);
    }

    FreeDosObject(DOS_FIB,fib);


}

sWizTemplate *getTemplates()
{
    return gFirstTemplate;
}


/*
 *    cJSON *root = NULL;
    cJSON *fmt = NULL;
    cJSON *img = NULL;
    cJSON *thm = NULL;
    cJSON *fld = NULL;
 *
     read expected output
    expected = read_file(expected_path);
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Failed to read expected output.");

     read and parse test
    tree = parse_file(test_path);
    TEST_ASSERT_NOT_NULL_MESSAGE(tree, "Failed to read of parse test.");

    if (print_preallocated(root) != 0) {
        cJSON_Delete(root);

static cJSON *parse_file(const char *filename)
{
    cJSON *parsed = NULL;
    char *content = read_file(filename);

    parsed = cJSON_Parse(content);

    if (content != NULL)
    {
        free(content);
    }

    return parsed;
}
*/
