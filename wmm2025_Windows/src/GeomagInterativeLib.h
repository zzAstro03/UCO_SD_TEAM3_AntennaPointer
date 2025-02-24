int MAG_ValidateDMSstring(char *input, int min, int max, char *Error);
void MAG_GetDeg(char* Query_String, double* latitude, double bounds[2]);
int MAG_GetAltitude(char* Query_String, MAGtype_Geoid *Geoid, MAGtype_CoordGeodetic* coords, int bounds[2], int AltitudeSetting);
int MAG_GetUserGrid(MAGtype_CoordGeodetic *minimum, MAGtype_CoordGeodetic *maximum, double *step_size, double *a_step_size, double *step_time, MAGtype_Date
        *StartDate, MAGtype_Date *EndDate, int *ElementOption, int *PrintOption, char *OutputFile, MAGtype_Geoid *Geoid, MAGtype_MagneticModel* model);
int MAG_GetUserInput(MAGtype_MagneticModel *MagneticModel, MAGtype_Geoid *Geoid, MAGtype_CoordGeodetic *CoordGeodetic, MAGtype_Date *MagneticDate);

void MAG_clear_input_buffer();
void MAG_GetMinGridInput(double* coord, double* val_bound, char* var_name);
void MAG_GetMaxGridInput(double* coord, double* val_bound, char* var_name);

void MAG_GetMaxGridInputAlt(double* coord, double min_val);

void MAG_GetMinGridInputDecYear(double* coord, double* val_bound);
void MAG_GetMaxGridInputDecYear(double* coord, double* val_bound);


