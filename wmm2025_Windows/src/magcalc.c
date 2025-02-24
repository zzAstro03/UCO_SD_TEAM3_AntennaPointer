//
// Created by Li-Yin Young on 8/19/22.
//
#include <stdio.h>
#include "GeomagnetismHeader.h"

/*
 * The function is for point calculation for ewmm_point. The input of hight is already determined whether to
 * covert to Ellipsoid height
 */
void point_calc(MAGtype_Ellipsoid Ellip, MAGtype_CoordGeodetic CoordGeodetic, MAGtype_CoordSpherical* CoordSpherical,
                MAGtype_Date UserDate, MAGtype_MagneticModel* MagneticModel, MAGtype_MagneticModel* TimedMagneticModel,
                MAGtype_GeoMagneticElements* GeoMagneticElements, MAGtype_GeoMagneticElements* Errors){


    MAG_TimelyModifyMagneticModel(UserDate, MagneticModel, TimedMagneticModel); /* Time adjust the coefficients, Equation 19, WMM Technical report */
    MAG_Geomag(Ellip, *CoordSpherical, CoordGeodetic, TimedMagneticModel, GeoMagneticElements); /* Computes the geoMagnetic field elements and their time change*/
    MAG_CalculateGridVariation(CoordGeodetic, GeoMagneticElements);
    #ifdef WMMHR
        MAG_WMMHRErrorCalc(GeoMagneticElements->H, Errors);
    #else
        MAG_WMMErrorCalc(GeoMagneticElements->H, Errors);
    #endif
}

MAGtype_MagneticModel* allocate_coefsArr_memory(int nMax, MAGtype_MagneticModel* MagneticModel){
    int NumTerms;

    MAGtype_MagneticModel* TimedMagneticModel;

    if(nMax < MagneticModel->nMax) nMax = MagneticModel->nMax;
    NumTerms = ((nMax + 1) * (nMax + 2) / 2);
    TimedMagneticModel = MAG_AllocateModelMemory(NumTerms); /* For storing the time modified WMM Model parameters */

    return TimedMagneticModel;
}
