// Algo based on use of lambert's fraction (https://varietyofsound.wordpress.com/2011/02/14/efficient-tanh-computation-using-lamberts-continued-fraction/)
inline double fast_tanh(double x)
{
    // Clip output when it goes above 1 or below -1
    if (x < -4.971786858528029)
    {
        return -1;
    }
    if (x > 4.971786858528029)
    {
        return 1;
    }
    double x2 = x * x;
    double a = x * (135135.0 + x2 * (17325.0 + x2 * (378.0 + x2)));
    double b = 135135.0 + x2 * (62370.0 + x2 * (3150.0 + x2 * 28.0));
    return a / b;
}