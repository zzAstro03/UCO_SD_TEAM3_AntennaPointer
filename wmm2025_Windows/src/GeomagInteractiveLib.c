#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "GeomagnetismHeader.h"
#include "GeomagInterativeLib.h"

void MAG_clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        // Simply read and discard each character
    }
}
int MAG_ValidateDMSstring(char *input, int min, int max, char *Error)

/* Validates a latitude DMS string, and returns 1 for a success and returns 0 for a failure.
It copies an error message to the Error string in the event of a failure.

INPUT : input (DMS string)
OUTPUT : Error : Error string
CALLS : none
 */
{
    int degree, minute, second, j = 0, n, max_minute = 60, max_second = 60;
    int i;
    degree = -1000;
    minute = -1;
    second = -1;
    n = (int) strlen(input);
    int Error_size = 255;

    for(i = 0; i <= n - 1; i++) /*tests for legal characters*/
    {
        if((input[i] < '0' || input[i] > '9') && (input[i] != ',' && input[i] != ' ' && input[i] != '-' && input[i] != '\0' && input[i] != '\n'))
        {
          //  The Error is passed as char pointer from MAG_GetDeg(), so I can't use "sizeof()" to estimate its size. The size of Error is 255 and defined in MAG_GetDeg(). MAG_GetDeg() is the only function which will call MAG_ValidateDMSstring().
          MAG_strlcpy_equivalent(Error, "\nError: Input contains an illegal character, legal characters for Degree, Minute, Second format are:\n '0-9' ',' '-' '[space]' '[Enter]'\n", Error_size);
            return FALSE;
        }
        if(input[i] == ',')
            j++;
    }
    if(j == 2)
        j = sscanf(input, "%d, %d, %d", &degree, &minute, &second); /*tests for legal formatting and range*/
    else
        j = sscanf(input, "%d %d %d", &degree, &minute, &second);
    if(j == 1)
    {
        minute = 0;
        second = 0;
        j = 3;
    }
    if(j != 3)
    {
        MAG_strlcpy_equivalent(Error, "\nError: Not enough numbers used for Degrees, Minutes, Seconds format\n or they were incorrectly formatted\n The legal format is DD,MM,SS or DD MM SS\n", Error_size);
        return FALSE;
    }
    if(degree > max || degree < min)
    {
        sprintf(Error, "\nError: Degree input is outside legal range\n The legal range is from %d to %d\n", min, max);
        return FALSE;
    }
    if(degree == max || degree == min)
        max_minute = 0;
    if(minute > max_minute || minute < 0)
    {
        MAG_strlcpy_equivalent(Error, "\nError: Minute input is outside legal range\n The legal minute range is from 0 to 60\n", Error_size);
        return FALSE;
    }
    if(minute == max_minute)
        max_second = 0;
    if(second > max_second || second < 0)
    {
        MAG_strlcpy_equivalent(Error, "\nError: Second input is outside legal range\n The legal second range is from 0 to 60\n", Error_size);
        return FALSE;
    }
    return TRUE;
} /*MAG_ValidateDMSstring*/

void MAG_GetDeg(char* Query_String, double* latitude, double bounds[2]) {
	/*Gets a degree value from the user using the standard input*/
	char buffer[64], Error_Message[255];
	int done, i, j;
	
	printf("%s", Query_String);
    while (NULL == fgets(buffer, sizeof(buffer), stdin)){
        printf("%s", Query_String);
        if (buffer[sizeof(buffer) - 1] != '\n'){
            MAG_clear_input_buffer(); // Remove the left characters from stdin if the buffer is not able to read the whole stdin
        }    
    }
     
    for(i = 0, done = 0, j = 0; i < (int) sizeof(buffer) && !done; i++)
    {
        if(buffer[i] == '.')
        {
            j = sscanf(buffer, "%lf", latitude);
            if(j == 1)
                done = 1;
            else
                done = -1;
        }
        if(buffer[i] == ',')
        {
            if(MAG_ValidateDMSstring(buffer, bounds[0], bounds[1], Error_Message))
            {
                MAG_DMSstringToDegree(buffer, latitude);
                done = 1;
            } else
                done = -1;
        }
        if(buffer[i] == ' ')/* This detects if there is a ' ' somewhere in the string,
		if there is the program tries to interpret the input as Degrees Minutes Seconds.*/
        {
            if(MAG_ValidateDMSstring(buffer, bounds[0], bounds[1], Error_Message))
            {
                MAG_DMSstringToDegree(buffer, latitude);
                done = 1;
            } else
                done = -1;
        }
        if(buffer[i] == '\0' || done == -1)
        {
            if(MAG_ValidateDMSstring(buffer, bounds[0], bounds[1], Error_Message) && done != -1)
            {
                sscanf(buffer, "%lf", latitude);
                done = 1;
            } else
            {
                printf("%s", Error_Message);
                MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
                printf("\nError encountered, please re-enter as '(-)DDD,MM,SS' or in Decimal Degrees DD.ddd:\n");
                while(NULL == fgets(buffer, sizeof(buffer), stdin)) {
                    printf("\nError encountered, please re-enter as '(-)DDD,MM,SS' or in Decimal Degrees DD.ddd:\n");
                    if (buffer[sizeof(buffer) - 1] != '\n'){
                        MAG_clear_input_buffer(); // Remove the left characters from stdin if the buffer is not able to read the whole stdin
                    }
                }
                 
                i = -1;
                done = 0;
            }
        }
    }
}

int MAG_GetAltitude(char* Query_String, MAGtype_Geoid *Geoid, MAGtype_CoordGeodetic* coords, int bounds[2], int AltitudeSetting){
	int done, j, UpBoundOn;
	char tmp;
	char buffer[64];
	double value;
	done = 0;
    if(bounds[1] != NO_ALT_MAX){
        UpBoundOn = TRUE;    
    } else {
        UpBoundOn = FALSE;
    }
    printf("%s", Query_String);
	
    while(!done)
    {
        MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
        while(NULL == fgets(buffer, sizeof(buffer), stdin)) {
            printf("%s", Query_String);
        }
        j = 0;
        if((AltitudeSetting != MSLON) && (buffer[0] == 'e' || buffer[0] == 'E' || AltitudeSetting == WGS84ON)) /* User entered height above WGS-84 ellipsoid, copy it to CoordGeodetic->HeightAboveEllipsoid */
        {
			if(buffer[0]=='e' || buffer[0]=='E') {
				j = sscanf(buffer, "%c%lf", &tmp, &coords->HeightAboveEllipsoid);
			} else {
				j = sscanf(buffer, "%lf", &coords->HeightAboveEllipsoid);
			}
            if(j == 2)
                j = 1;
            Geoid->UseGeoid = 0;
            coords->HeightAboveGeoid = coords->HeightAboveEllipsoid;
			value = coords->HeightAboveEllipsoid;
        } else /* User entered height above MSL, convert it to the height above WGS-84 ellipsoid */
        {
            Geoid->UseGeoid = 1;
            j = sscanf(buffer, "%lf", &coords->HeightAboveGeoid);
            MAG_ConvertGeoidToEllipsoidHeight(coords, Geoid);
			value = coords->HeightAboveGeoid;
        }
        if(j == 1)
            done = 1;
        else
            printf("\nIllegal Format, please re-enter as '(-)HHH.hhh:'\n");
        if((value < bounds[0] || (value > bounds[1] && UpBoundOn)) && done == 1) {
			if(UpBoundOn) {
				done = 0;
				printf("\nWarning: The value you have entered of %f km for the elevation is outside of the required range.\n", value);
				printf(" An elevation between %d km and %d km is needed. \n", bounds[0], bounds[1]);
				if (AltitudeSetting == WGS84ON){
				    printf("Please enter height above WGS-84 Ellipsoid (in kilometers):\n");
				} else if (AltitudeSetting==MSLON){
				    printf("Please enter height above mean sea level (in kilometers):\n");
				} else {
				    printf("Please enter height in kilometers (prepend E for height above WGS-84 Ellipsoid):");
				}
			} else {
				switch(MAG_Warnings(3, value, NULL)) {
					case 0:
						return USER_GAVE_UP;
					case 1:
						done = 0;
						printf("Please enter height above sea level (in kilometers):\n");
						break;
					case 2:
						break;
				}
            }
        }
    }
    return 0;
}
void MAG_GetMinGridInput(double* coord, double* val_bound, char* var_name){

    char buffer[20];

    if (NULL == fgets(buffer, sizeof(buffer), stdin) || sscanf(buffer, "%lf", coord) != 1) {
        *coord = 0;
        printf("Unrecognized input default %lf used\n", *coord);
            
    }
     

    while(*coord < val_bound[0] || *coord > val_bound[1]){
        printf("Error: Degree input is outside range\n"
               " The range is from %.2f to %.2f\n", val_bound[0], val_bound[1]);
        printf("Please re-enter the minimal %s:", var_name);
        if (NULL == fgets(buffer, 20, stdin) || sscanf(buffer, "%lf", coord) != 1) {
            *coord = 0;
            printf("Unrecognized input default %lf used\n", *coord);
              
        }
    }
}

void MAG_GetMinGridInputDecYear(double* coord, double* val_bound){

    char buffer[20];

    if (NULL == fgets(buffer, sizeof(buffer), stdin) || sscanf(buffer, "%lf", coord) != 1) {
        *coord = 0;   
    }

       while(*coord < val_bound[0] || *coord >= val_bound[1]){
        printf("Error: Decimal year input is outside range\n"
               " The range is from %.2f to %.2f\n", val_bound[0], val_bound[1]);
        printf("Please re-enter the minimal decimal year:");
        if (NULL == fgets(buffer, 20, stdin) || sscanf(buffer, "%lf", coord) != 1) {
            *coord = 0;
            if (buffer[sizeof(buffer) - 1] != '\n'){
                MAG_clear_input_buffer(); // Remove the left characters from stdin if the buffer is not able to read the whole stdin
            } 
        }
    }
}

void MAG_GetMaxGridInputDecYear(double* coord, double* val_bound){

    char buffer[20];

    if (NULL == fgets(buffer, sizeof(buffer), stdin) || sscanf(buffer, "%lf", coord) != 1) {
        *coord = 0; 
    }

     

    while(*coord < val_bound[0] || *coord >= val_bound[1] ){
        printf("Error: Decimal year input is outside range\n"
               " The range is from %.2f to %.2f\n", val_bound[0], val_bound[1]);
        printf("Please re-enter the maximal decimal year:");
        if (NULL == fgets(buffer, 20, stdin) || sscanf(buffer, "%lf", coord) != 1) {
            *coord = 0;
            if (buffer[sizeof(buffer) - 1] != '\n'){
                MAG_clear_input_buffer(); // Remove the left characters from stdin if the buffer is not able to read the whole stdin
            }
        }
    }
}




void MAG_GetMaxGridInputAlt(double* coord, double min){
	 
     char buffer[20];

    if (NULL == fgets(buffer, sizeof(buffer), stdin) || sscanf(buffer, "%lf", coord) != 1) {
        *coord = 0;
        printf("Unrecognized input default %lf used\n", *coord);
             
    }

 
   
    while(*coord < min ){
        printf("Error maximum altitude is less than minimum altitude\n");
        printf("Please re-enter the maximal altitude:");
        if (NULL == fgets(buffer, 20, stdin) || sscanf(buffer, "%lf", coord) != 1) {
            *coord = 0;
            printf("Unrecognized input default %lf used\n", *coord);
                     
        }
    }


}

void MAG_GetMaxGridInput(double* coord, double* val_bound, char* var_name){

    char buffer[20];

    if (NULL == fgets(buffer, sizeof(buffer), stdin) || sscanf(buffer, "%lf", coord) != 1) {
        *coord = 0;
        printf("Unrecognized input default %lf used\n", *coord);
             
    }

   

    while(*coord < val_bound[0] || *coord > val_bound[1] ){
        printf("Error: Degree input is outside range\n"
               " The range is from %.2f to %.2f\n", val_bound[0], val_bound[1]);
        printf("Please re-enter the maximal %s:", var_name);
        if (NULL == fgets(buffer, 20, stdin) || sscanf(buffer, "%lf", coord) != 1) {
            *coord = 0;
            printf("Unrecognized input default %lf used\n", *coord);
                
        }
    }
}

int MAG_GetUserInput(MAGtype_MagneticModel *MagneticModel, MAGtype_Geoid *Geoid, MAGtype_CoordGeodetic *CoordGeodetic, MAGtype_Date *MagneticDate)

/*
This prompts the user for coordinates, and accepts many entry formats.
It takes the MagneticModel and Geoid as input and outputs the Geographic coordinates and Date as objects.
Returns 0 when the user wants to exit and 1 if the user enters valid input data.
INPUT :  MagneticModel  : Data structure with the following elements used here
                        double epoch;       Base time of Geomagnetic model epoch (yrs)
                : Geoid Pointer to data structure MAGtype_Geoid (used for converting HeightAboveGeoid to HeightABoveEllipsoid

OUTPUT: CoordGeodetic : Pointer to data structure. Following elements are updated
                        double lambda; (longitude)
                        double phi; ( geodetic latitude)
                        double HeightAboveEllipsoid; (height above the ellipsoid (HaE) )
                        double HeightAboveGeoid;(height above the Geoid )

                MagneticDate : Pointer to data structure MAGtype_Date with the following elements updated
                        int	Year; (If user directly enters decimal year this field is not populated)
                        int	Month;(If user directly enters decimal year this field is not populated)
                        int	Day; (If user directly enters decimal year this field is not populated)
                        double DecimalYear;      decimal years

CALLS: 	MAG_DMSstringToDegree(buffer, &CoordGeodetic->lambda); (The program uses this to convert the string into a decimal longitude.)
                MAG_ValidateDMSstringlong(buffer, Error_Message)
                MAG_ValidateDMSstringlat(buffer, Error_Message)
                MAG_Warnings
                MAG_ConvertGeoidToEllipsoidHeight
                MAG_DateToYear

 */
{
    char Error_Message[255];
    char buffer[40];
    buffer[sizeof(buffer) - 1] = '\0';
    int i, j, a, b, c, done = 0;
	double lat_bound[2] = {LAT_BOUND_MIN, LAT_BOUND_MAX};
	double lon_bound[2] = {LON_BOUND_MIN, LON_BOUND_MAX};
    int alt_bound[2] = {ALT_BOUND_MIN, NO_ALT_MAX}; 
	int Qstring_size = 1028;
    char* Qstring = malloc(sizeof(char) * Qstring_size);
    memset(Qstring, '\0', Qstring_size); 
    MAG_strlcpy_equivalent(Qstring, "\nPlease enter latitude\nNorth latitude positive, For example:\n30, 30, 30 (D,M,S) or 30.508 (Decimal Degrees) (both are north)\n", Qstring_size);
	MAG_GetDeg(Qstring, &CoordGeodetic->phi, lat_bound);
    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer)); /*Clear the input*/
    memset(Qstring, '\0', Qstring_size); // Clear QString to avoid show the string copied from previous time
    MAG_strlcpy_equivalent(Qstring,"\nPlease enter longitude\nEast longitude positive, West negative.  For example:\n-100.5 or -100, 30, 0 for 100.5 degrees west\n", Qstring_size);
	MAG_GetDeg(Qstring, &CoordGeodetic->lambda, lon_bound);
	
    memset(Qstring, '\0', Qstring_size);    
	MAG_strlcpy_equivalent(Qstring,"\nPlease enter height above mean sea level (in kilometers):\n[For height above WGS-84 ellipsoid prefix E, for example (E20.1)]\n", Qstring_size);
    if(MAG_GetAltitude(Qstring, Geoid, CoordGeodetic, alt_bound, FALSE)==USER_GAVE_UP)
        return FALSE;
    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
    printf("\nPlease enter the decimal year or calendar date\n (YYYY.yyy, MM DD YYYY or MM/DD/YYYY):\n");
    while (NULL == fgets(buffer, sizeof(buffer), stdin)) {
        printf("\nPlease enter the decimal year or calendar date\n (YYYY.yyy, MM DD YYYY or MM/DD/YYYY):\n");
        
    }
     
    for(i = 0, done = 0; i < sizeof(buffer) && !done; i++)
    {
        if(buffer[i] == '.')
        {
            j = sscanf(buffer, "%lf", &MagneticDate->DecimalYear);
            if(j == 1)
                done = 1;
            else
                buffer[i] = '\0';
        }
        if(buffer[i] == '/')
        {
            sscanf(buffer, "%d/%d/%d", &MagneticDate->Month, &MagneticDate->Day, &MagneticDate->Year);
            if(!MAG_DateToYear(MagneticDate, Error_Message))
            {
                printf("%s", Error_Message);
                printf("\nPlease re-enter Date in MM/DD/YYYY or MM DD YYYY format, or as a decimal year\n");
                while (NULL == fgets(buffer, sizeof(buffer), stdin)) {
                    printf("\nPlease re-enter Date in MM/DD/YYYY or MM DD YYYY format, or as a decimal year\n");
                    if (buffer[sizeof(buffer) - 1] != '\n'){
                        MAG_clear_input_buffer(); // Remove the left characters from stdin if the buffer is not able to read the whole stdin
                    }
                }
                 
                i = 0;
            } else
                done = 1;
        }
        if((i < sizeof(buffer) - 1 && buffer[i] == ' ' && buffer[i + 1] != '/') || buffer[i] == '\0')
        {
            if(3 == sscanf(buffer, "%d %d %d", &a, &b, &c))
            {
                MagneticDate->Month = a;
                MagneticDate->Day = b;
                MagneticDate->Year = c;
                MagneticDate->DecimalYear = 99999;
            } else if(1 == sscanf(buffer, "%d %d %d", &a, &b, &c))
            {
                MagneticDate->DecimalYear = a;
                done = 1;
            }
            if(!(MagneticDate->DecimalYear == a))
            {
                if(!MAG_DateToYear(MagneticDate, Error_Message))
                {
                    printf("%s", Error_Message);
                    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
                    printf("\nError encountered, please re-enter Date in MM/DD/YYYY or MM DD YYYY format, or as a decimal year\n");
                    while( NULL== fgets(buffer, sizeof(buffer), stdin)){
                        printf("\nError encountered, please re-enter Date in MM/DD/YYYY or MM DD YYYY format, or as a decimal year\n");
                        if (buffer[strlen(buffer) - 1] != '\n') {
                            MAG_clear_input_buffer(); 
                        }
                    }
                    
                    i = -1;
                } else
                    done = 1;
            }
        }
        if(buffer[i] == '\0' && i != -1 && done != 1)
        {
            MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
            printf("\nError encountered, please re-enter as MM/DD/YYYY, MM DD YYYY, or as YYYY.yyy:\n");
            while (NULL ==fgets(buffer, sizeof(buffer), stdin)) {
                printf("\nError encountered, please re-enter as MM/DD/YYYY, MM DD YYYY, or as YYYY.yyy:\n"); 
                if (buffer[sizeof(buffer) - 1] != '\n'){
                    MAG_clear_input_buffer(); // Remove the left characters from stdin if the buffer is not able to read the whole stdin
                }
            }
             
            i = -1;
        }
        if(done)
        {
            if(MagneticDate->DecimalYear > MagneticModel->CoefficientFileEndDate || MagneticDate->DecimalYear < MagneticModel->min_year)
            {
                switch(MAG_Warnings(4, MagneticDate->DecimalYear, MagneticModel)) {
                    case 0:
                        return 0;
                    case 1:
                        done = 0;
                        i = -1;
                        MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
                        printf("\nPlease enter the decimal year or calendar date\n (YYYY.yyy, MM DD YYYY or MM/DD/YYYY):\n");
                        while(NULL == fgets(buffer, sizeof(buffer), stdin)){
                            printf("\nPlease enter the decimal year or calendar date\n (YYYY.yyy, MM DD YYYY or MM/DD/YYYY):\n");
                            if (buffer[sizeof(buffer) - 1] != '\n'){
                                MAG_clear_input_buffer(); // Remove the left characters from stdin if the buffer is not able to read the whole stdin
                            }
                        }
                         
                        break;
                    case 2:
                        break;
                }
            }
        }
    }
    free(Qstring);
    return TRUE;
} /*MAG_GetUserInput*/

int MAG_GetUserGrid(MAGtype_CoordGeodetic *minimum, MAGtype_CoordGeodetic *maximum, double *step_size, double *a_step_size, double *step_time, MAGtype_Date
        *StartDate, MAGtype_Date *EndDate, int *ElementOption, int *PrintOption, char *OutputFile, MAGtype_Geoid *Geoid, MAGtype_MagneticModel* model)

/* Prompts user to enter parameters to compute a grid - for use with the MAG_grid function
Note: The user entries are not validated before here. The function populates the input variables & data structures.

UPDATE : minimum Pointer to data structure with the following elements
                double lambda; (longitude)
                double phi; ( geodetic latitude)
                double HeightAboveEllipsoid; (height above the ellipsoid (HaE) )
                double HeightAboveGeoid;(height above the Geoid )

                maximum   -same as the above -MAG_USE_GEOID
                step_size  : double pointer : spatial step size, in decimal degrees
                a_step_size : double pointer :  double altitude step size (km)
                step_time : double pointer : time step size (decimal years)
                StartDate : pointer to data structure with the following elements updates
                                        double DecimalYear;     ( decimal years )
                EndDate :	Same as the above
CALLS : none


 */
{
    FILE *fileout;
    char filename[] = "GridProgramDirective.txt";
    char buffer[20];
    int strcopy_size = 32;
    char default_outfile[16] = "GridResults.txt";

    int dummy;
    double lat_bound[2] = {LAT_BOUND_MIN, LAT_BOUND_MAX};
    double lon_bound[2] = {LON_BOUND_MIN, LON_BOUND_MAX};


    printf("Please Enter Minimum Latitude (in decimal degrees):\n");
    char var_name[20] = "latitude";
    MAG_GetMinGridInput(&minimum->phi, lat_bound, var_name);

    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
    lat_bound[0] = minimum->phi;
    printf("Please Enter Maximum Latitude (in decimal degrees):\n");
    MAG_GetMaxGridInput(&maximum->phi, lat_bound, var_name);

    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
    printf("Please Enter Minimum Longitude (in decimal degrees):\n");
    MAG_strlcpy_equivalent(var_name, "lontitude", sizeof(var_name));
    MAG_GetMinGridInput(&minimum->lambda, lon_bound, var_name);

    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
    lon_bound[0] = minimum->lambda;
    printf("Please Enter Maximum Longitude (in decimal degrees):\n");
    MAG_GetMaxGridInput(&maximum->lambda, lon_bound, var_name);

    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
    printf("Please Enter Step Size (in decimal degrees):\n");
    if (NULL == fgets(buffer, sizeof(buffer), stdin) || sscanf(buffer, "%lf", step_size) != 1){
        *step_size = fmax(maximum->phi - minimum->phi, maximum->lambda - minimum->lambda);
        printf("Unrecognized input default %lf used\n", *step_size);
        
    } else {
        sscanf(buffer, "%lf", step_size);
    }
    
     
    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));

    printf("Select height (default : above MSL) \n1. Above Mean Sea Level\n2. Above WGS-84 Ellipsoid \n");
    if (NULL == fgets(buffer, sizeof(buffer), stdin)) {
        Geoid->UseGeoid = 1;
        printf("Unrecognized option, height above MSL used.");
        
    } else {
        sscanf(buffer, "%d", &dummy);
        if(dummy == 2) Geoid->UseGeoid = 0;
        else Geoid->UseGeoid = 1;
    }
    
     
    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
    if(Geoid->UseGeoid == 1)
    {
        printf("Please Enter Minimum Height above MSL (in km):\n");
        if (NULL == fgets(buffer, sizeof(buffer), stdin) || sscanf(buffer, "%lf", &minimum->HeightAboveGeoid) != 1) {
            minimum->HeightAboveGeoid = 0;
            printf("Unrecognized input default %lf used\n", minimum->HeightAboveGeoid);
        } else {
            sscanf(buffer, "%lf", &minimum->HeightAboveGeoid);
        }
        MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
        printf("Please Enter Maximum Height above MSL (in km):\n");
        MAG_GetMaxGridInputAlt(&maximum->HeightAboveGeoid, minimum->HeightAboveGeoid);
        MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));

    } else
    {
        printf("Please Enter Minimum Height above the WGS-84 Ellipsoid (in km):\n");
        if (NULL == fgets(buffer, sizeof(buffer), stdin) || sscanf(buffer, "%lf", &maximum->HeightAboveGeoid) != 1) {
            maximum->HeightAboveGeoid = 0;
            printf("Unrecognized input default %lf used\n", maximum->HeightAboveGeoid);
        
                    
        } else {
            sscanf(buffer, "%lf", &minimum->HeightAboveGeoid);
        }
     
    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
    printf("Please Enter Maximum Height above the WGS-84 Ellipsoid (in km):\n");
    MAG_GetMaxGridInputAlt(&maximum->HeightAboveGeoid, minimum->HeightAboveGeoid);
	MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
    }
    printf("Please Enter height step size (in km):\n");
    if (NULL == fgets(buffer, sizeof(buffer), stdin) || sscanf(buffer, "%lf", a_step_size) != 1) {
        *a_step_size = maximum->HeightAboveGeoid - minimum->HeightAboveGeoid;
        printf("Unrecognized input default %lf used\n", *a_step_size);
        
    } else {
        sscanf(buffer, "%lf", a_step_size);
    }
     
    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));

    double dec_year_bound[2] = {model->min_year, model->CoefficientFileEndDate};
    printf("\nPlease Enter the decimal year starting time:\n");
    MAG_GetMinGridInputDecYear(&StartDate->DecimalYear,dec_year_bound );

    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));

    dec_year_bound[0] = StartDate->DecimalYear;
    printf("Please Enter the decimal year ending time:\n");
    MAG_GetMaxGridInputDecYear(&EndDate->DecimalYear,dec_year_bound );

    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
    printf("Please Enter the time step size:\n");
    if (NULL == fgets(buffer, sizeof(buffer), stdin) || sscanf(buffer, "%lf", step_time) != 1) {
        *step_time = EndDate->DecimalYear - StartDate->DecimalYear;
        printf("Unrecognized input, default of %lf used\n", *step_time);
        
    } else {
        sscanf(buffer, "%lf", step_time);
    }
    
     
    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
    printf("Enter a geomagnetic element to print. Your options are:\n");
    printf(" 1. Declination	9.   Ddot\n 2. Inclination	10. Idot\n 3. F		11. Fdot\n 4. H		12. Hdot\n 5. X		13. Xdot\n 6. Y		14. Ydot\n 7. Z		15. Zdot\n 8. GV		16. GVdot\nFor gradients enter: 17\n");
    if (NULL == fgets(buffer, sizeof(buffer), stdin)) {
        *ElementOption = 1;
        printf("Unrecognized input, default of %d used\n", *ElementOption);
        
    }
    sscanf(buffer, "%d", ElementOption);
    
     
    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
    if(*ElementOption == 17)
    {
        printf("Enter a gradient element to print. Your options are:\n");
        printf(" 1. dX/dphi \t2. dY/dphi \t3. dZ/dphi\n");
        printf(" 4. dX/dlambda \t5. dY/dlambda \t6. dZ/dlambda\n");
        printf(" 7. dX/dz \t8. dY/dz \t9. dZ/dz\n");
        MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
        if (NULL == fgets(buffer, 20, stdin)) {
            *ElementOption=1;
            printf("Unrecognized input, default of %d used\n", *ElementOption);
        } else {
            sscanf(buffer, "%d", ElementOption);
        }
        MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
        *ElementOption+=16;
    }
    printf("Select output :\n");
    printf(" 1. Print to a file \n 2. Print to Screen\n");
    if (NULL ==fgets(buffer, sizeof(buffer), stdin)){
        *PrintOption = 2;
        printf("Unrecognized input, default of printing to screen\n");
        
    } else {
        sscanf(buffer, "%d", PrintOption);
    }
    
     
    MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
    fileout = fopen(filename, "a");
    if(*PrintOption == 1)
    {
        printf("Please enter output filename\nfor default ('%s') press enter:\n", default_outfile);
        if(NULL==fgets(buffer, sizeof(buffer), stdin) || strlen(buffer) <= 1)
        {
            
            fprintf(fileout, "\nResults printed in: %s\n", default_outfile);
            // The size of OutputFile is 32 which is defined in #127 in GeomagnetismHeader.h
            strcopy_size = (sizeof(default_outfile) > 32)? 32:sizeof(default_outfile);
            MAG_strlcpy_equivalent(OutputFile, "GridResults.txt", strcopy_size);
            
        } else
        {
            sscanf(buffer, "%s", OutputFile);
            fprintf(fileout, "\nResults printed in: %s\n", OutputFile);
        }
        /*strcpy(OutputFile, buffer);*/
         
        MAG_strlcpy_equivalent(buffer, "", sizeof(buffer));
        /*sscanf(buffer, "%s", OutputFile);*/
    } else
        fprintf(fileout, "\nResults printed in Console\n");
    fprintf(fileout, "Minimum Latitude: %f\t\tMaximum Latitude: %f\t\tStep Size: %f\nMinimum Longitude: %f\t\tMaximum Longitude: %f\t\tStep Size: %f\n", minimum->phi, maximum->phi, *step_size, minimum->lambda, maximum->lambda, *step_size);
    if(Geoid->UseGeoid == 1)
        fprintf(fileout, "Minimum Altitude above MSL: %f\tMaximum Altitude above MSL: %f\tStep Size: %f\n", minimum->HeightAboveGeoid, maximum->HeightAboveGeoid, *a_step_size);
    else
        fprintf(fileout, "Minimum Altitude above WGS-84 Ellipsoid: %f\tMaximum Altitude above WGS-84 Ellipsoid: %f\tStep Size: %f\n", minimum->HeightAboveEllipsoid, maximum->HeightAboveEllipsoid, *a_step_size);
    fprintf(fileout, "Starting Date: %f\t\tEnding Date: %f\t\tStep Time: %f\n\n\n", StartDate->DecimalYear, EndDate->DecimalYear, *step_time);
    fclose(fileout);
    return TRUE;
}


