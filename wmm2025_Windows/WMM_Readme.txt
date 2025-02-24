World Magnetic Model 2025 Software and support documents
=======================================================

Sublibrary Files
================
GeomagnetismLibrary.c              WMM Subroutine library, C functions
GeomagnetismHeader.h               WMM Subroutine library, C header file 
GeomagInteractiveLib.c             WMM instractive model library, C functions
GeomagInteractiveLib.h             WMM instractive model library, C header file

Main Programs
===============
main/wmm_point.c                 Command prompt version for single point computation
main/wmm_grid.c                  Grid, profile and time series computation, C main function
main/wmm_file.c                  C program which takes a coordinate file as input with WMM ISO formatted coefficients as input


Excecutables
============

For Windows user:

wmm_point.exe               Command Prompt program for single point
wmm_grid.exe                Grid, time series or profile
wmm_file.exe                File processing

For Linux users:

- The executable files were created in AlmaLinux 9. If you would like to build from source codes, please use the Makefile to build excutable files from source.

Build from source:

- There is a Makefile in the source directory for building with clang/gcc in Linux and Unix OS.
- The library files GeomagnetismLibrary.c and GeomagnetismHeader.h should reside in the same directory for compiling.
- Use the three commands to generate wmm_file, wmm_grid and wmm_point. The excutable file will locate at bin. 
- If users build the excutable files from Makefile, the excutable files will be recreated in build folder.

     - make wmm_file 
     - make wmm_grid
     - make wmm_point
     - make wmm (Create three executable files all at once)


Executing the file processing program (wmm_file.exe)
=====================================
- The file processing program accepts this syntax:
- For Linux package, the example of inputs and outputs file for wmm_file.exe is in bin and data folder(sample_coords.txt, sample_output.txt). 
- For Windows package, the example of inputs and outputs file for wmm_file.exe is in bin folder((sample_coords.txt, sample_output.txt)).

(If you create the executable file by Makefile, please replace wmm_file.exe with ./wmm_file)

wmm_file.exe f INPUT_FILE.txt OUTPUT_FILE.txt

- Additionally to have uncertainty values appended to the output file add the "e" flag or "Errors".  For example:

wmm_file.exe f e INPUT_FILE.txt OUTPUT_FILE.txt

or

wmm_file.exe f --Errors INPUT_FILE.txt OUTPUT_FILE.txt



Data Files
===============
Note: For Linux package, below files are located in bin and data folder.

WMM.COF                 WMM2025 Coefficients file
sample_coords.txt    Sample input file for program wmm_file.exe
sample_output.txt   Sample output file for program  wmm_file.exe


Test Files
===============
WMM2025_TEST_VALUES.txt           Test values for WMM2025

Model Software Support
======================

*  National Centers for Environmental Information (NCEI)
*  E/NE42 325 Broadway
*  Boulder, CO 80303 USA
*  Attn: Manoj Nair or Arnaud Chulliat
*  Phone:  (303) 497-4642 or -6522
*  Email:  geomag.models@noaa.gov
For more details about the World Magnetic Model visit 
http://www.ngdc.noaa.gov/geomag/WMM/DoDWMM.shtml
 




       
