/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>


#include "GeomagnetismHeader.h"

/*#include "GeomagnetismLibrary.c"*/
#include "EGM9615.h"
#include "magcalc.h"
#include "version.h"
#include "GeomagInterativeLib.h"
/*---------------------------------------------------------------------------*/

/* 
WMM Point Calculation Program.

The Geomagnetism Library is used to make a command prompt program. The program prompts
the user to enter a location, performs the computations and prints the results to the
standard output. The program expects the files GeomagnetismLibrary.c, GeomagnetismHeader.h,
EWMM.COF and EGM9615.h to be in the same directory. 

Manoj.C.Nair@Noaa.Gov
April 21, 2011

liyin.young@noaa.gov
Updated April, 2023

 */

const char* BOZ_WARN_TEXT_STRONG = "Warning: some calculated locations are "
                                   "in the blackout zone "
                                   "around the magnetic pole\nas defined by "
                                   "the WMM military specification \n(https://"
                                   "www.ngdc.noaa.gov/geomag/WMM/data/MIL-PRF-"
                                   "89500B.pdf).\nCompass accuracy is highly "
                                   "degraded in this region.\n";
const char* BOZ_WARN_TEXT_WEAK = "Caution: some calculated locations approach "
                                "the blackout zone around the magnetic\npole "
                                "as defined by the WMM military specification "
                                "\n(https://www.ngdc.noaa.gov/geomag/WMM/data/"
                                "MIL-PRF-89500B.pdf).\n Compass accuracy may "
                                "be degraded in this region.\n";

const char* WMM_MileSpec_INFO = "Warning: The height validity of the geomagnetic components is dependent on the geomagnetic activity level. For more information see \n(https://www.ncei.noaa.gov/products/world-magnetic-model/accuracy-limitations-error-model)\n";
const char* WMM_MileSpec_WARN = "Warning: WMM will not meet MilSpec at this altitude. For more information see \n(https://www.ncei.noaa.gov/products/world-magnetic-model/accuracy-limitations-error-model)\n";

char MAG_GeomagIntroduction_WMM(MAGtype_MagneticModel *MagneticModel, char *VersionDate, char *ModelDate);

void help_info(MAGtype_MagneticModel *MagneticModel, char* short_name);

int main()
{
    MAGtype_MagneticModel * MagneticModels[1], *TimedMagneticModel;
    MAGtype_Ellipsoid Ellip;
    MAGtype_CoordSpherical CoordSpherical;
    MAGtype_CoordGeodetic CoordGeodetic;
    MAGtype_Date UserDate;
    MAGtype_GeoMagneticElements GeoMagneticElements, Errors;
    MAGtype_Geoid Geoid;
    char ans[20], b;
    int print_boz_warning_strong = FALSE;
    int print_boz_warning_weak = FALSE;
    #ifdef WMMHR
        char filename[] = "WMMHR.COF";
        char program_name[] = "wmmhr_point";
    #else
        char filename[] = "WMM.COF";
        char program_name[] = "wmm_point";
    #endif
    char VersionDate[12];
    int NumTerms, Flag = 1, nMax = 0;
    int epochs = 1;
    /* Memory allocation */

    strncpy(VersionDate, VERSIONDATE_LARGE + 39, 11);
    VersionDate[11] = '\0';
    if(!MAG_robustReadMagModels(filename, &MagneticModels, epochs)) {
        printf("\n %s not found.  Press enter to exit... \n ", filename);
        fgets(ans, 20, stdin);
        return 1;
    }

    NumTerms = ((nMax + 1) * (nMax + 2) / 2);

    TimedMagneticModel = MAG_AllocateModelMemory(NumTerms); /* For storing the time modified WMM Model parameters */


    if(MagneticModels[0] == NULL || TimedMagneticModel == NULL)
    {
        MAG_Error(2);
    }

    TimedMagneticModel = allocate_coefsArr_memory(nMax, MagneticModels[0]);

    MAG_SetDefaults(&Ellip, &Geoid); /* Set default values and constants */
    /* Check for Geographic Poles */



    /* Set EGM96 Geoid parameters */
    Geoid.GeoidHeightBuffer = GeoidHeightBuffer;
    Geoid.Geoid_Initialized = 1;
    /* Set EGM96 Geoid parameters END */
    b = MAG_GeomagIntroduction_WMM(MagneticModels[0], VersionDate, MODEL_RELEASE_DATE);
    while(Flag == 1 && b != 'x')
    {
        if(MAG_GetUserInput(MagneticModels[0], &Geoid, &CoordGeodetic, &UserDate) == 1) /*Get User Input */
        {

            MAG_GeodeticToSpherical(Ellip, CoordGeodetic, &CoordSpherical); /*Convert from geodetic to Spherical Equations: 17-18, WMM Technical report*/
            point_calc(Ellip, CoordGeodetic, &CoordSpherical,
                            UserDate, MagneticModels[0], TimedMagneticModel,
                            &GeoMagneticElements, &Errors);
           if(GeoMagneticElements.H <= 2000.0) {
                print_boz_warning_strong = TRUE;
            } else if (GeoMagneticElements.H <= 6000.0) {
                print_boz_warning_weak = TRUE;
            }
#ifndef WMMHR
            if (CoordGeodetic.HeightAboveEllipsoid < -1 || CoordGeodetic.HeightAboveEllipsoid > 1900){
            printf("%s\n", WMM_MileSpec_WARN);
            }
            else{
                printf("%s\n",WMM_MileSpec_INFO);
            }
#endif            
            MAG_PrintUserDataWithUncertainty(GeoMagneticElements, Errors, CoordGeodetic, UserDate, TimedMagneticModel, &Geoid); /* Print the results */
        }
       if(print_boz_warning_strong){ 
            printf("\n %s\n", BOZ_WARN_TEXT_STRONG);
        } else if (print_boz_warning_weak) {
            printf("\n %s\n", BOZ_WARN_TEXT_WEAK);
        }

        do {
            printf("\n\n Do you need more point data ? (y or n) \n ");
        } while (NULL == fgets(ans, 20, stdin));
        switch(ans[0]) {
            case 'Y':
            case 'y':
                Flag = 1;
                break;
            case 'N':
            case 'n':
                Flag = 0;
                break;
            default:
                Flag = 0;
                break;
        }

    }


    MAG_FreeMagneticModelMemory(TimedMagneticModel);
    MAG_FreeMagneticModelMemory(MagneticModels[0]);

    return 0;
}

char MAG_GeomagIntroduction_WMM(MAGtype_MagneticModel *MagneticModel, char *VersionDate, char *ModelDate)
/*Prints the introduction to the Geomagnetic program.  It needs the Magnetic model for the epoch.

 * INPUT  MagneticModel		: MAGtype_MagneticModel With Model epoch 	(input)
  OUTPUT ans   (char)  user selection
  CALLS : none
 */
{
    int ans_size = 10;
    char help[ans_size];
    
    #ifdef WMMHR
        char short_name[50] = "WMMHR";
        printf("\n\n Welcome to the World Magnetic Model High-Resolution (WMMHR) %d C-Program\n\n", (int) MagneticModel->epoch);
    #else
        char short_name[50] = "WMM";
        printf("\n\n Welcome to the World Magnetic Model %d C-Program\n\n", (int) MagneticModel->epoch);
    #endif    
    printf("              --- Model Release Date: %s ---\n", ModelDate);
    printf("            --- Software Release Date: %s ---\n\n", VersionDate);
    printf("\n This program estimates the strength and direction of ");
    printf("\n Earth's main Magnetic field for a given point/area.");
    printf("\n Enter h for help and contact information or c to continue.");
    printf("\n >");
    
    while (fgets(help, ans_size, stdin) == NULL || (help[0] != 'C' && help[0] != 'c')){
               
        if (help[0] == 'h' || help[0] == 'H'){
            help_info(MagneticModel, short_name);
        }
       printf("\n Enter h for help and contact information or c to continue.");
       printf("\n >");
         
    }
    
        
    return help[0];
} /*MAG_GeomagIntroduction_WMM*/

void help_info(MAGtype_MagneticModel *MagneticModel, char* short_name){
    printf("\n Help information ");

    #ifdef WMMHR
        printf("\n The World Magnetic Model High-Resolution (WMMHR) for %d", (int) MagneticModel->epoch);
    #else
        printf("\n The World Magnetic Model (WMM) for %d", (int) MagneticModel->epoch); 
    #endif 
    printf("\n is a model of Earth's main magnetic field. The %s", short_name);
    printf("\n is recomputed every five (5) years, in years divisible by ");
    printf("\n five (i.e. 2020, 2025). See the contact information below");
    printf("\n to obtain more information on the %s and associated software.", short_name);
    printf("\n ");
    printf("\n Input required is the location in geodetic latitude and");
    printf("\n longitude (positive for northern latitudes and eastern ");
    printf("\n longitudes), geodetic altitude in meters, and the date of ");
    printf("\n interest in years.");

    printf("\n\n\n The program computes the estimated Magnetic Declination");
    printf("\n (Decl) which is sometimes called MagneticVAR, Inclination (Incl), Total");
    printf("\n Intensity (F or TI), Horizontal Intensity (H or HI), Vertical");
    printf("\n Intensity (Z), and Grid Variation (GV). Declination and Grid");
    printf("\n Variation are measured in units of degrees and are considered");
    printf("\n positive when east or north. Inclination is measured in units");
    printf("\n of degrees and is considered positive when pointing down (into");
    printf("\n the Earth). The %s is referenced to the WGS-84 ellipsoid and", short_name);
    printf("\n is valid for 5 years after the base epoch. Uncertainties for the");
    printf("\n %s are one standard deviation uncertainties averaged over the globe.", short_name);
    printf("\n We represent the uncertainty as constant values in Incl, F, H, X,");
    printf("\n Y, and Z. Uncertainty in Declination varies depending on the strength");
    printf("\n of the horizontal field. For more information see the %s Technical", short_name);
    printf("\n Report.");
    
    printf("\n\n\n It is very important to note that a degree and order 133 model,");
    printf("\n such as %s, describes only the longest wavelength spatial magnetic ", short_name);
    printf("\n fluctuations due to Earth's core, mantle and crust. Not included in the %s series", short_name);
    printf("\n models are intermediate and short wavelength spatial fluctuations ");
    printf("\n that originate in Earth's mantle and crust. Consequently, isolated");
    printf("\n angular errors at various positions on the surface (primarily over");
    printf("\n land, along continental margins and over oceanic sea-mounts, ridges and");
    printf("\n trenches) of several degrees may be expected. Also not included in");
    printf("\n the model are temporal fluctuations of magnetospheric and ionospheric");
    printf("\n origin. On the days during and immediately following magnetic storms,");
    printf("\n temporal fluctuations can cause substantial deviations of the Geomagnetic");
    printf("\n field from model values. If the required declination accuracy is");
    printf("\n more stringent than the %s series of models provide, the user is", short_name);
    printf("\n advised to request special (regional or local) surveys be performed");
    #ifdef WMMHR
        printf("\n and models prepared. The World Magnetic Model High-Resolution is a joint product of");
    #else
        printf("\n and models prepared. The World Magnetic Model is a joint product of"); 
    #endif
    printf("\n the United States' National Geospatial-Intelligence Agency (NGA) and");
    printf("\n the United Kingdom's Defence Geographic Centre (DGC). The %s was", short_name);
    printf("\n developed jointly by the National Centers for Environmental Information");
    printf("\n (NCEI, Boulder CO, USA) and the British Geological Survey (BGS, ");
    printf("\n Edinburgh, Scotland).");

    printf("\n\n\n Contact Information");

    printf("\n  Software and Model Support");
    printf("\n	National Centers for Environmental Information");
    printf("\n	NOAA E/NE42");
    printf("\n	325 Broadway");
    printf("\n	Boulder, CO 80305 USA");
    printf("\n	Attn: Manoj Nair or Arnaud Chulliat");
    printf("\n	Email:  Geomag.Models@noaa.gov \n");

}
