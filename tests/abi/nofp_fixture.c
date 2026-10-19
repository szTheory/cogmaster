/* Negative control for the nofp lane: one double division, which GCC must
   refuse to compile under -mgeneral-regs-only. Never part of a build. */
double cogmaster__nofp_fixture(double a, double b);

double cogmaster__nofp_fixture(double a, double b)
{
    return a / b;
}
