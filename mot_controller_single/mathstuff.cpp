#include <math.h>

const static double RTD = 180.0 / M_PI; // constant for conversion radians to degrees
const static double DTR = M_PI / 180.0; // constant for conversion degrees to radians

void MBSat_AzEltoXY(float az, float el, float *x, float *y) {
    const float accuracy = 0.01; // rotor accuracy in degrees

    if (el < 0.1) {
        el = 0.1;
    }
    el = fmod(el, 91);
    az = fmod(az, 361);

    if (el <= accuracy) {
        *x = 90.0;
    } else if (el >= (90.0 - accuracy)) {
        *x = 0.0;
    } else {
        *x = -atan(-cos(az * DTR) / tan(el * DTR)) * RTD;
    }

    *y = -asin(sin(az * DTR) * cos(el * DTR)) * RTD;
}

void MBSat_XYtoAzEl(float X, float Y, float *az, float *el) { // do not ask how many stupid hours it took to get this to work
    // Motor Description:
    // X -> lower motor -> North-South
    // Y -> upper motor -> East-West
    // X+ -> North | X- -> South
    // Y+ -> West  | Y- -> East

    float AZIM = acos(sqrt((pow(cos(Y * DTR), 2) * pow(tan(X * DTR), 2)) / (pow(sin(Y * DTR), 2) + pow(tan(X * DTR), 2)))) * RTD;

    // determine quadrant from X and Y angle, then determine correct azimuth and elevation
    if (X < 0) {
        if (Y < 0) {
            *az = 180 - AZIM;
        } else if (Y > 0) {
            *az = AZIM + 180;
        } else { // X movement but no Y movement case
            *az = 180;
            *el = fabs(X + 90);
            return;
        }
    } else if (X > 0) {
        if (Y < 0) {
            *az = AZIM;
        } else if (Y > 0) {
            *az = (360 - AZIM);
        } else { // X movement but no Y movement case
            *az = 0;
            *el = fabs(X - 90);
            return;
        }
    } else { // X=0 case
        if (Y > 0) {
            *az = 270;
            *el = fabs(Y - 90);
        } else if (Y < 0) {
            *az = 90;
            *el = fabs(Y + 90);
        } else { // Both motors are in 0 position -> could Azimuth=0 confuse software?
            *az = 0;
            *el = 90;
        }
        return;
    }

    *el = atan(cos(*az * DTR) / tan(X * DTR)) * RTD; // since we now have azimuth we can solve elevation
}
