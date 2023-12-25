#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "sgfilter.h"


extern char* optarg;
char* progname;

int* ivector(long nl, long nh)
{
    int* v;
    v = (int*)malloc((size_t)((nh - nl + 2) * sizeof(int)));
    if (!v)
    {
        //log("Error: Allocation failure.");
        exit(1);
    }
    return v - nl + 1;
}

double* dvector(long nl, long nh)
{
    double* v;
    long k;
    v = (double*)malloc((size_t)((nh - nl + 2) * sizeof(double)));
    if (!v)
    {
        //log("Error: Allocation failure.");
        exit(1);
    }
    for (k = nl; k <= nh; k++)
        v[k] = 0.0;
    return v - nl + 1;
}


double** dmatrix(long nrl, long nrh, long ncl, long nch)
{
    long i, nrow = nrh - nrl + 1, ncol = nch - ncl + 1;
    double** m;
    m = (double**)malloc((size_t)((nrow + 1) * sizeof(double*)));
    if (!m)
    {
        //log("Allocation failure 1 occurred.");
        exit(1);
    }
    m += 1;
    m -= nrl;
    m[nrl] = (double*)malloc((size_t)((nrow * ncol + 1) * sizeof(double)));
    if (!m[nrl])
    {
        //log("Allocation failure 2 occurred.");
        exit(1);
    }
    m[nrl] += 1;
    m[nrl] -= ncl;
    for (i = nrl + 1; i <= nrh; i++)
        m[i] = m[i - 1] + ncol;
    return m;
}

void free_ivector(int* v, long nl, long nh)
{
    free((char*)(v + nl - 1));
}

void free_dvector(double* v, long nl, long nh)
{
    free((char*)(v + nl - 1));
}

void free_dmatrix(double** m, long nrl, long nrh, long ncl, long nch)
{
    free((char*)(m[nrl] + ncl - 1));
    free((char*)(m + nrl - 1));
}

void lubksb(double** a, int n, int* indx, double b[])
{
    int i, ii = 0, ip, j;
    double sum;

    for (i = 1; i <= n; i++)
    {
        ip = indx[i];
        sum = b[ip];
        b[ip] = b[i];
        if (ii)
            for (j = ii; j <= i - 1; j++)
                sum -= a[i][j] * b[j];
        else if (sum)
            ii = i;
        b[i] = sum;
    }
    for (i = n; i >= 1; i--)
    {
        sum = b[i];
        for (j = i + 1; j <= n; j++)
            sum -= a[i][j] * b[j];
        b[i] = sum / a[i][i];
    }
}

void ludcmp(double** a, int n, int* indx, double* d)
{
    int i, imax = 0, j, k;
    double big, dum, sum, temp;
    double* vv;

    vv = dvector(1, n);
    *d = 1.0;
    for (i = 1; i <= n; i++)
    {
        big = 0.0;
        for (j = 1; j <= n; j++)
            if ((temp = fabs(a[i][j])) > big)
                big = temp;
        if (big == 0.0)
        {
            //log("Error: Singular matrix found in routine ludcmp()");
            exit(1);
        }
        vv[i] = 1.0 / big;
    }
    for (j = 1; j <= n; j++)
    {
        for (i = 1; i < j; i++)
        {
            sum = a[i][j];
            for (k = 1; k < i; k++)
                sum -= a[i][k] * a[k][j];
            a[i][j] = sum;
        }
        big = 0.0;
        for (i = j; i <= n; i++)
        {
            sum = a[i][j];
            for (k = 1; k < j; k++)
                sum -= a[i][k] * a[k][j];
            a[i][j] = sum;
            if ((dum = vv[i] * fabs(sum)) >= big)
            {
                big = dum;
                imax = i;
            }
        }
        if (j != imax)
        {
            for (k = 1; k <= n; k++)
            {
                dum = a[imax][k];
                a[imax][k] = a[j][k];
                a[j][k] = dum;
            }
            *d = -(*d);
            vv[imax] = vv[j];
        }
        indx[j] = imax;
        if (a[j][j] == 0.0)
            a[j][j] = EPSILON;
        if (j != n)
        {
            dum = 1.0 / (a[j][j]);
            for (i = j + 1; i <= n; i++)
                a[i][j] *= dum;
        }
    }
    free_dvector(vv, 1, n);
}

#include <math.h>
#define SWAP(a, b) \
    tempr = (a);   \
    (a) = (b);     \
    (b) = tempr
void four1(double data[], unsigned long nn, int isign)
{
    unsigned long n, mmax, m, j, istep, i;
    double wtemp, wr, wpr, wpi, wi, theta;
    double tempr, tempi;

    n = nn << 1;
    j = 1;
    for (i = 1; i < n; i += 2)
    {
        if (j > i)
        {
            SWAP(data[j], data[i]);
            SWAP(data[j + 1], data[i + 1]);
        }
        m = n >> 1;
        while (m >= 2 && j > m)
        {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
    mmax = 2;
    while (n > mmax)
    {
        istep = mmax << 1;
        theta = isign * (6.28318530717959 / mmax);
        wtemp = sin(0.5 * theta);
        wpr = -2.0 * wtemp * wtemp;
        wpi = sin(theta);
        wr = 1.0;
        wi = 0.0;
        for (m = 1; m < mmax; m += 2)
        {
            for (i = m; i <= n; i += istep)
            {
                j = i + mmax;
                tempr = wr * data[j] - wi * data[j + 1];
                tempi = wr * data[j + 1] + wi * data[j];
                data[j] = data[i] - tempr;
                data[j + 1] = data[i + 1] - tempi;
                data[i] += tempr;
                data[i + 1] += tempi;
            }
            wr = (wtemp = wr) * wpr - wi * wpi + wr;
            wi = wi * wpr + wtemp * wpi + wi;
        }
        mmax = istep;
    }
}
#undef SWAP

void twofft(double data1[], double data2[], double fft1[], double fft2[],
    unsigned long n)
{
    void four1(double data[], unsigned long nn, int isign);
    unsigned long nn3, nn2, jj, j;
    double rep, rem, aip, aim;

    nn3 = 1 + (nn2 = 2 + n + n);
    for (j = 1, jj = 2; j <= n; j++, jj += 2)
    {
        fft1[jj - 1] = data1[j];
        fft1[jj] = data2[j];
    }
    four1(fft1, n, 1);
    fft2[1] = fft1[2];
    fft1[2] = fft2[2] = 0.0;
    for (j = 3; j <= n + 1; j += 2)
    {
        rep = 0.5 * (fft1[j] + fft1[nn2 - j]);
        rem = 0.5 * (fft1[j] - fft1[nn2 - j]);
        aip = 0.5 * (fft1[j + 1] + fft1[nn3 - j]);
        aim = 0.5 * (fft1[j + 1] - fft1[nn3 - j]);
        fft1[j] = rep;
        fft1[j + 1] = aim;
        fft1[nn2 - j] = rep;
        fft1[nn3 - j] = -aim;
        fft2[j] = aip;
        fft2[j + 1] = -rem;
        fft2[nn2 - j] = aip;
        fft2[nn3 - j] = rem;
    }
}

void realft(double data[], unsigned long n, int isign)
{
    void four1(double data[], unsigned long nn, int isign);
    unsigned long i, i1, i2, i3, i4, np3;
    double c1 = 0.5, c2, h1r, h1i, h2r, h2i;
    double wr, wi, wpr, wpi, wtemp, theta;

    theta = 3.141592653589793 / (double)(n >> 1);
    if (isign == 1)
    {
        c2 = -0.5;
        four1(data, n >> 1, 1);
    }
    else
    {
        c2 = 0.5;
        theta = -theta;
    }
    wtemp = sin(0.5 * theta);
    wpr = -2.0 * wtemp * wtemp;
    wpi = sin(theta);
    wr = 1.0 + wpr;
    wi = wpi;
    np3 = n + 3;
    for (i = 2; i <= (n >> 2); i++)
    {
        i4 = 1 + (i3 = np3 - (i2 = 1 + (i1 = i + i - 1)));
        h1r = c1 * (data[i1] + data[i3]);
        h1i = c1 * (data[i2] - data[i4]);
        h2r = -c2 * (data[i2] + data[i4]);
        h2i = c2 * (data[i1] - data[i3]);
        data[i1] = h1r + wr * h2r - wi * h2i;
        data[i2] = h1i + wr * h2i + wi * h2r;
        data[i3] = h1r - wr * h2r + wi * h2i;
        data[i4] = -h1i + wr * h2i + wi * h2r;
        wr = (wtemp = wr) * wpr - wi * wpi + wr;
        wi = wi * wpr + wtemp * wpi + wi;
    }
    if (isign == 1)
    {
        data[1] = (h1r = data[1]) + data[2];
        data[2] = h1r - data[2];
    }
    else
    {
        data[1] = c1 * ((h1r = data[1]) + data[2]);
        data[2] = c1 * (h1r - data[2]);
        four1(data, n >> 1, -1);
    }
}

char convlv(double data[], unsigned long n, double respns[], unsigned long m,
    int isign, double ans[])
{
    void realft(double data[], unsigned long n, int isign);
    void twofft(double data1[], double data2[], double fft1[], double fft2[],
        unsigned long n);
    unsigned long i, no2;
    double dum, mag2, * fft;

    fft = dvector(1, n << 1);
    for (i = 1; i <= (m - 1) / 2; i++)
        respns[n + 1 - i] = respns[m + 1 - i];
    for (i = (m + 3) / 2; i <= n - (m - 1) / 2; i++)
        respns[i] = 0.0;
    twofft(data, respns, fft, ans, n);
    no2 = n >> 1;
    for (i = 2; i <= n + 2; i += 2)
    {
        if (isign == 1)
        {
            ans[i - 1] = (fft[i - 1] * (dum = ans[i - 1]) - fft[i] * ans[i]) / no2;
            ans[i] = (fft[i] * dum + fft[i - 1] * ans[i]) / no2;
        }
        else if (isign == -1)
        {
            if ((mag2 = ans[i - 1] * ans[i - 1] + ans[i] * ans[i]) == 0.0)
            {
                //log("Attempt of deconvolving at zero response in convlv().");
                return (1);
            }
            ans[i - 1] = (fft[i - 1] * (dum = ans[i - 1]) + fft[i] * ans[i]) / mag2 / no2;
            ans[i] = (fft[i] * dum - fft[i - 1] * ans[i]) / mag2 / no2;
        }
        else
        {
            //log("No meaning for isign in convlv().");
            return (1);
        }
    }
    ans[2] = ans[n + 1];
    realft(ans, n, -1);
    free_dvector(fft, 1, n << 1);
    return (0);
}

char sgcoeff(double c[], int np, int nl, int nr, int ld, int m)
{
    void lubksb(double** a, int n, int* indx, double b[]);
    void ludcmp(double** a, int n, int* indx, double* d);
    int imj, ipj, j, k, kk, mm, * indx;
    double d, fac, sum, ** a, * b;

    if (np < nl + nr + 1 || nl < 0 || nr < 0 || ld > m || nl + nr < m)
    {
        //log("Inconsistent arguments detected in routine sgcoeff.");
        return (1);
    }
    indx = ivector(1, m + 1);
    a = dmatrix(1, m + 1, 1, m + 1);
    b = dvector(1, m + 1);
    for (ipj = 0; ipj <= (m << 1); ipj++)
    {
        sum = (ipj ? 0.0 : 1.0);
        for (k = 1; k <= nr; k++)
            sum += pow((double)k, (double)ipj);
        for (k = 1; k <= nl; k++)
            sum += pow((double)-k, (double)ipj);
        mm = (ipj < 2 * m - ipj ? ipj : 2 * m - ipj);
        for (imj = -mm; imj <= mm; imj += 2)
            a[1 + (ipj + imj) / 2][1 + (ipj - imj) / 2] = sum;
    }
    ludcmp(a, m + 1, indx, &d);
    for (j = 1; j <= m + 1; j++)
        b[j] = 0.0;
    b[ld + 1] = 1.0;
    lubksb(a, m + 1, indx, b);
    for (kk = 1; kk <= np; kk++)
        c[kk] = 0.0;
    for (k = -nl; k <= nr; k++)
    {
        sum = b[1];
        fac = 1.0;
        for (mm = 1; mm <= m; mm++)
            sum += b[mm + 1] * (fac *= k);
        kk = ((np - k) % np) + 1;
        c[kk] = sum;
    }
    free_dvector(b, 1, m + 1);
    free_dmatrix(a, 1, m + 1, 1, m + 1);
    free_ivector(indx, 1, m + 1);
    return (0);
}

char sgfilter(double yr[], double yf[], int mm, int nl, int nr, int ld, int m)
{
    int np = nl + 1 + nr;
    double* c;
    char retval;

#if CONVOLVE_WITH_NR_CONVLV
    c = dvector(1, mm);
    retval = sgcoeff(c, np, nl, nr, ld, m);
    if (retval == 0)
        convlv(yr, mm, c, np, 1, yf);
    free_dvector(c, 1, mm);
#else
    int j;
    long int k;
    c = dvector(1, nl + nr + 1);
    retval = sgcoeff(c, np, nl, nr, ld, m);
    if (retval == 0)
    {
        for (k = 1; k <= nl; k++)
        {
            for (yf[k] = 0.0, j = -nl; j <= nr; j++)
            {
                if (k + j >= 1)
                {
                    yf[k] += c[(j >= 0 ? j + 1 : nr + nl + 2 + j)] * yr[k + j];
                }
            }
        }
        for (k = nl + 1; k <= mm - nr; k++)
        {
            for (yf[k] = 0.0, j = -nl; j <= nr; j++)
            {
                yf[k] += c[(j >= 0 ? j + 1 : nr + nl + 2 + j)] * yr[k + j];
            }
        }
        for (k = mm - nr + 1; k <= mm; k++)
        {
            for (yf[k] = 0.0, j = -nl; j <= nr; j++)
            {
                if (k + j <= mm)
                {
                    yf[k] += c[(j >= 0 ? j + 1 : nr + nl + 2 + j)] * yr[k + j];
                }
            }
        }
    }
    free_dvector(c, 1, nr + nl + 1);
#endif
    return (retval);
}

short pathcharacter(int ch)
{
    return (isalnum(ch) || (ch == '.') || (ch == '/') || (ch == '\\') || (ch == '_') ||
        (ch == '-') || (ch == '+'));
}

char* strip_away_path(char filename[])
{
    int j, k = 0;
    while (pathcharacter(filename[k]))
        k++;
    j = (--k);
    while (isalnum((int)(filename[j])))
        j--;
    j++;
    return (&filename[j]);
}


//int main(int argc, char* argv[])
//{
//    int no_arg;
//    int nl = DEFAULT_NL;
//    int nr = DEFAULT_NR;
//    int ld = DEFAULT_LD;
//    int m = DEFAULT_M;
//    //mm is pais of x and y
//    long int k, mm = 0;
//    double* x, * yr, * yf;
//    char input_filename[NCHMAX] = "", output_filename[NCHMAX] = "";
//    char verbose = 0;
//
//    if (mm < nl + nr + 1)
//    {
//        //log("Error: The number M=%ld of data points must be at least nl+nr+1=%d",
//            mm, nl + nr + 1);
//        //log("Please check your -nl or -nr options.");
//        exit(1);
//    }
//    if (verbose)
//    {
//        //log("Loading %ld unfiltered samples from %s...", mm, input_filename);
//        //log(" ... allocating memory for storage ...");
//    }
//    x = dvector(1, mm);
//    yr = dvector(1, mm);
//#if CONVOLVE_WITH_NR_CONVLV
//    yf = dvector(1, 2 * mm);
//#else
//    yf = dvector(1, mm);
//#endif
//    if (verbose)
//        //log(" ... scanning %s for input data ...", input_filename);
//    for (k = 1; k <= mm; k++)
//    {
//        fscanf(file, "%lf", &x[k]);
//        fscanf(file, "%lf", &yr[k]);
//    }
//    fclose(file);
//    if (verbose)
//        //log(" ... done. Input now residing in RAM.");
//
//    if (!sgfilter(yr, yf, mm, nl, nr, ld, m))
//    {
//        if (verbose)
//            //log("Successfully performed Savitzky-Golay filtering.");
//    }
//    else
//    {
//        if (verbose)
//            //log("Error: Could not perform Savitzky-Golay filtering.");
//    }
//
//    /*:19*/
//#line 985 "./sgfilter.w"
//
///*20:*/
//#line 1178 "./sgfilter.w"
//
//    if (!strcmp(output_filename, ""))
//    {
//        if (verbose)
//            //log("Writing %ld filtered samples to console...", mm);
//    }
//    else
//    {
//        if (verbose)
//            //log("Writing %ld filtered samples to %s...", mm, output_filename);
//        if ((file = freopen(output_filename, "w", stdout)) == NULL)
//        {
//            //log("Error: Unable to redirect stdout stream to file %s.",
//                output_filename);
//            exit(1);
//        }
//    }
//    for (k = 1; k <= mm; k++)
//        fprintf(stdout, "%1.8f %1.8f\n", x[k], yf[k]);
//#ifdef UNIX_LIKE_OS
//    freopen("/dev/tty", "a", stdout);
//#endif
//    if (verbose)
//        //log(" ... done.");
//
//    /*:20*/
//#line 986 "./sgfilter.w"
//
///*21:*/
//#line 1199 "./sgfilter.w"
//
//    free_dvector(x, 1, mm);
//    free_dvector(yr, 1, mm);
//#if CONVOLVE_WITH_NR_CONVLV
//    free_dvector(yf, 1, 2 * mm);
//#else
//    free_dvector(yf, 1, mm);
//#endif
//
//    /*:21*/
//#line 987 "./sgfilter.w"
//
//    return (EXIT_SUCCESS);
//}
//
///*:13*/