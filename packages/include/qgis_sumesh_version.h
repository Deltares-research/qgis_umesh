#ifndef __QGIS_UMESH_VERSION__          
#define __QGIS_UMESH_VERSION__          
#define qgis_umesh_major "0"     
#define qgis_umesh_minor "30"     
#define qgis_umesh_revision "00"     
#define qgis_umesh_build "6d70ff5"     
#define qgis_umesh_company "Deltares"     
#define qgis_umesh_company_url "https://www.deltares.nl"     
#define qgis_umesh_source_url "https://github.com/Deltares-research/qgis_umesh.git"     
#define qgis_umesh_program "QGIS_SUMESH"     
#define qgis_umesh_version_number "0.30.00.6d70ff5"     
#if defined(WIN32)     
#define qgis_umesh_arch "Win32"     
#elif defined(WIN64)     
#define qgis_umesh_arch "Win64"     
#elif defined(LINUX64)     
#define qgis_umesh_arch "Linux64"     
#elif     
#define qgis_umesh_arch "Unknown"     
#endif     
/*=================================================== DO NOT MAKE CHANGES BELOW THIS LINE ===================================================================== */     
extern "C" {     
    extern char * getcompanystring_qgis_umesh(void);     
    extern char * getfullversionstring_qgis_umesh(void);     
    extern char * getprogramstring_qgis_umesh(void);     
    extern char * getversionstring_qgis_umesh(void);     
    extern char * getsourceurlstring_qgis_umesh(void);     
}     
#endif      
