//
// Created by Li-Yin Young on 8/19/22.
//

#ifndef ENHANCED_WMM_GEOMAGCALC_H
#define ENHANCED_WMM_GEOMAGCALC_H

void point_calc(MAGtype_Ellipsoid Ellip, MAGtype_CoordGeodetic CoordGeodetic, MAGtype_CoordSpherical* CoordSpherical,
                MAGtype_Date UserDate, MAGtype_MagneticModel* MagneticModel, MAGtype_MagneticModel* TimedMagneticModel,
                MAGtype_GeoMagneticElements* GeoMagneticElements, MAGtype_GeoMagneticElements* Errors);

MAGtype_MagneticModel* allocate_coefsArr_memory(int nMax, MAGtype_MagneticModel* MagneticModel);

#endif //ENHANCED_WMM_GEOMAGCALC_H
