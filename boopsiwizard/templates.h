#ifndef WIZTEMPLATES_H_
#define WIZTEMPLATES_H_

/**
    The software part of the wizard
    (no interface code here, interface should just use it.)

*/
typedef struct _sWTmpl_pref
{
    int type;
    char *_displayname;
} sWTmpl_pref;

typedef struct _sWTmpl_pref_bool
{
    int type;
    char *_displayname;
    char *_rid;

} sWTmpl_pref_bool;

typedef struct _sStringArray
{
    int _nb;
    char **_p;
} sStringArray;

/** one instance per known template, described in json */
typedef struct _sWizTemplate
{
    struct _sWizTemplate *_pNext;
    char *_displayName;
    char *_versionstring;
    char *_archivename;
    char *_defaultname;
    char *_comment;
    // list of prefs
    sWTmpl_pref *_firstPRef;
    sStringArray _renames;

} sWizTemplate;



typedef void (*template_notifier)(int ilog,const char *log);

void initTemplates(template_notifier n);

sWizTemplate *getTemplates();
int getNbTemplates();

// - - - - - - - -  - --

// parameters for generation, could evolve with settings from json and interface...
typedef struct _Generation
{
    const char *baseName;

} sGeneration;

// return information about generated project, when succeed.
// must be close with CloseGenerationReport()
typedef struct _GenerationReport
{
    const char *destinationDir;

} sGenerationReport;


int extractTemplate(sWizTemplate *ptmpl,
                    const char *templateArchive,
                    const char *destDir,
                    sGeneration *pgen,
                    sGenerationReport *report);
// if return !=0
const char *extractTextError();
// to be used after extraction if report was passed.
void CloseGenerationReport(sGenerationReport *report);


#endif

