
#include "templates.h"
#include "zlib.h"
#include "unzip.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <dos/dos.h>

/**
    almost same thing as unzipping...
    almost: we replace names also.
*/
static const char *errorstring=NULL;
static const char *errorstringmem="No mem";
static unzFile zF=NULL;

static void extractClose()
{
    if(zF) unzClose(zF);
    zF = NULL;
}
static int exitclset=0;

const char *extractTextError()
{
    return errorstring;
}


static char lowName[32];
static char upName[32];
static char MajName[32];
static void makeSecureNames(const char *pn)
{
    int iln=0;
    int uln=0;
    int majn=0;
    while(*pn != 0 && iln<31 && uln<31)
    {
        char c = *pn++;
        if(c == ' '){
            continue;
        }
        if(c>='A' && c<='Z')
        {
            upName[uln] = c;
            uln++;
            MajName[majn] = c;
            majn++;
            lowName[iln] = c + ((int)'a'-(int)'A');
            iln++;
        } else
        if(c>='a' && c<='z')
        {
            upName[uln] = c;
            uln++;
            lowName[iln] = c ;
            iln++;
            MajName[majn] = c + ((int)'A'-(int)'a');
            majn++;
        }
    }
    upName[uln] = 0;
    lowName[iln] = 0;
    MajName[majn] = 0;
}

enum
{
	PATH_NOT_FOUND,
	PATH_IS_FILE,
	PATH_IS_DIRECTORY
};


int get_path_info(const char *fullpath)
{
    BPTR lock = Lock(fullpath,ACCESS_READ);

    if(!lock) return PATH_NOT_FOUND;

    struct FileInfoBlock *fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if(!fib){
        UnLock(lock);
        return PATH_NOT_FOUND;
    }
    Examine(lock, fib);

    int res = ((fib->fib_DirEntryType<0)?PATH_IS_FILE:PATH_IS_DIRECTORY);
    FreeDosObject(DOS_FIB,fib);
    UnLock(lock);

    return res;
}

// would create directory recusively, return if ok.
// hey people, that one looks C-ace awesome code !
static int assumeDirectory(const char *fullpath)
{
    int dirtype = get_path_info(fullpath);
    if(dirtype == PATH_IS_DIRECTORY) return PATH_IS_DIRECTORY;

    // recurse createdir
    char *lastsep = strrchr(fullpath,'/');
    if(lastsep)
    {
        *lastsep = 0;
        int subtype = assumeDirectory(fullpath);
        *lastsep = '/';
        if(subtype != PATH_IS_DIRECTORY) return PATH_NOT_FOUND;
    }

    BPTR l = CreateDir(fullpath);
    if(l) {
        UnLock(l);
        return PATH_IS_DIRECTORY;
    }
    return PATH_NOT_FOUND;
}
// to be used before creating a file in a possibly non exitant dir.
static int assumeDirectoryBeforeFile(const char *fullpath)
{
    // look previous createdir
    char *lastsep = strrchr(fullpath,'/');
    if(!lastsep) return PATH_IS_DIRECTORY; // would fit dev:file, so ok.
    // if here more lilkely: dev:dir/file

    *lastsep = 0;
    int subtype = assumeDirectory(fullpath); // switch to dir recurse
    *lastsep = '/';
    if(subtype != PATH_IS_DIRECTORY) return PATH_NOT_FOUND;
    return PATH_IS_DIRECTORY;
}
typedef struct {
 const char *key;
 const char *repl;
} sReplace;


void replacefilename(char *p,const char *orig,sReplace *replacers)
{
    while(*orig != 0)
    {
        sReplace *sr = replacers;
        int didreplace=0;
        while(sr->key != NULL)
        {
            if(strncmp(orig,sr->key,strlen(sr->key))==0)
            {
                int l = strlen(sr->repl);
                memcpy(p,sr->repl,l);
                p += l;
                orig += strlen(sr->key);
                didreplace = 1;
                continue;
            }
            sr++;
        }
        if(!didreplace)
        {
            *p++ = *orig++;
        }
    }
    *p = 0;
}



static void replacewriteFh(FILE *fhw,const char *orig,ULONG origbsize,sReplace *replacers)
{
    ULONG borigdone =0;
    while(borigdone<origbsize)
    {
        sReplace *sr = replacers;
        int didreplace=0;
        while(sr->key != NULL)
        {
            if(strncmp(orig,sr->key,strlen(sr->key))==0)
            {
                int l = strlen(sr->repl);
                Write(fhw,sr->repl,l);
                orig += strlen(sr->key);
                borigdone += strlen(sr->key);
                didreplace = 1;
                continue;
            }
            sr++;
        }
        if(!didreplace)
        {
            Write(fhw,orig,1);
            orig++;
            borigdone++;
           // *p++ = *orig++;
        }
    }
}
int EndsWith(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return( strncasecmp(str + lenstr - lensuffix, suffix, lensuffix) == 0);
}


// this is the whole effective part:
// file names and uppercase/lower cases are all replaced while writting
int replaceWrite(const char *originalbin, ULONG origbsize,
                 const char *origfilepath, const char *destbase,sWizTemplate *tmpl)
{
    // assume base dir
    ULONG fullbase_l = strlen(destbase)+2+strlen(upName);
    char *fulldestpath = AllocVec(fullbase_l,0);
    if(!fulldestpath)
    {
     errorstring = errorstringmem;
     return 1;
    }
    *fulldestpath=0;
    strcat(fulldestpath,destbase);
    // if dvice: no need to /
    if(destbase[strlen(destbase)-1]!= ':')  strcat(fulldestpath,"/");
    strcat(fulldestpath,upName);
// printf("dirpath:%s\n",fulldestpath);

    int dirtype = assumeDirectory(fulldestpath);
    if(dirtype != PATH_IS_DIRECTORY)
    {
      errorstring = "can't create project directory";
     return 1;
    }

    // - - - -
    sReplace replacers[]={
        {"basename",lowName},
        {"BaseName",upName},
        {"BASENAME",MajName},
        {NULL,NULL}
    };
    // got to remap file name
    // origfilepath
    int res=0;
    // replace/ write engine.
    {
        char *pfinalnamepath = AllocVec(fullbase_l+1+256,0);
        if(pfinalnamepath)
        {
            *pfinalnamepath = 0;
            strcat(pfinalnamepath,fulldestpath);
            strcat(pfinalnamepath,"/");
            replacefilename(pfinalnamepath+strlen(pfinalnamepath),origfilepath,replacers);
            assumeDirectoryBeforeFile(pfinalnamepath);

            FILE *fhw = Open(pfinalnamepath,MODE_NEWFILE);
            if(!fhw)
            {
                errorstring = "can't write file in directory";
                res = 1;
            } else
            {
                int doReplaceBin = 0;
                // check if bin have to be text-replaced by testing end names
                for( int j=0 ; j<tmpl->_renames._nb ; j++ )
                {
                    if(EndsWith(pfinalnamepath,tmpl->_renames._p[j])) {
                        doReplaceBin = 1;
                        break;
                    }
                }

                if(doReplaceBin) replacewriteFh(fhw,originalbin,origbsize,replacers);
                else
                {
                    Write(fhw,originalbin,origbsize); // direct bin copy.
                }
                Close(fhw);
            }
            FreeVec(pfinalnamepath);
        }
 //       BPTR hdl = Open(fullpath, MODE_NEWFILE);
 //   ULONG fullbase_l = strlen(destbase)+2+strlen(upName);



    } // end writting paragraph
    // - - - -
    FreeVec(fulldestpath);
    return res;
}


int extractTemplate(
            sWizTemplate *ptmpl,
            const char *templateArchive,
            const char *destDir,
            sGeneration *pgen,
            sGenerationReport *report)
{
//    printf("extractTemplate:%s %s %s\n",templateArchive,destDir,pgen->baseName);
    if(exitclset==0) {
        atexit(extractClose);
        exitclset=1;
    }
    char btemp[128];
    if(!templateArchive || !destDir || !pgen || !pgen->baseName)
    {
        errorstring = "archive name or extract dir name uncompliant";
        return 1;
    }
    makeSecureNames(pgen->baseName);

    extractClose(); // in case of previous call.
    if(lowName[0]==0 ||
        upName[0]==0 )
    {
        errorstring = "Project name is empty, use only letters";
        return 1;
    }


    btemp[0]=0;
    strcat(btemp,"PROGDIR:templates/");
    strcat(btemp,templateArchive);
    zF = unzOpen(btemp);
    if(!zF)
    {
        errorstring = "can't open template zip file";
        return 1;
    }

//    printf("unzip ok\n");

    int r = unzGoToFirstFile(zF);
    int rr=0;
    while(r == UNZ_OK)
    {
    //uLong compressed_size;      /* compressed size                 4 bytes */
    //uLong uncompressed_size;    /* uncompressed size               4 bytes */
    //uLong size_filename;        /* filename length                 2 bytes */

        unz_file_info zfinfo;
        char tfilename[256];

        /*int*/  unzGetCurrentFileInfo(zF, // unzFile file,
                                    &zfinfo,//     unz_file_info *pfile_info,
                                         tfilename,//char *szFileName,
                                         255,//uLong fileNameBufferSize,
                                         NULL, //void *extraField,
                                         0, //uLong extraFieldBufferSize,
                                         NULL,//char *szComment,
                                         0 //uLong commentBufferSize
                                         );
        //printf("f:%s\n",tfilename);
        // to read a file in zip, need open/read close,
        int zferr = unzOpenCurrentFile(zF);
        if(zferr == UNZ_OK)
        {
            char *f = AllocVec(zfinfo.uncompressed_size,0);
            if(f)
            {
                int nbdone = unzReadCurrentFile(zF,f,zfinfo.uncompressed_size);

                if(nbdone == zfinfo.uncompressed_size)
                {
                    // extracted !
                    //printf("looks extracted ok!\n");

                    rr |= replaceWrite(f, nbdone,tfilename,destDir,ptmpl); // !=0 if any error
                }
                FreeVec(f);
            }
            unzCloseCurrentFile(zF);
        } // if inner file zopened ok.

        r = unzGoToNextFile(zF);
    }

    extractClose();
    if(rr==0)
    {
         errorstring = "Project created !";
         if(report)
         {
               // assume base dir
                ULONG fullbase_l = strlen(destDir)+2+strlen(upName);
               report->destinationDir = AllocVec(fullbase_l,0);
               char *destbase = (char *)report->destinationDir;
                if(destbase)
                {
                    *destbase=0;
                    strcat(destbase,destDir);
                    // if dvice: no need to /
                    if(destbase[strlen(destbase)-1]!= ':')  strcat(destbase,"/");
                    strcat(destbase,upName);
                }

         }
    }
    return rr;
}
// to be used after extraction if report was passed.
void CloseGenerationReport(sGenerationReport *report)
{
    if(report && report->destinationDir)
    {
        FreeVec(report->destinationDir);
        report->destinationDir = NULL;
    }
}
