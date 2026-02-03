#ifndef UTILITY_H_
#define UTILITY_H_

/**
 * 汎用的な操作をまとめたutilityファイル
 * 乱数生成，大小比較，通信判定など
 */

#include <float.h> // DBL_EPSILON
#include <math.h>

#define UTILITY_M_PI 3.14159265358979323846	/* pi math.hのものと同じ */

#define NAME(x) #x
#define SHOW(x) #x,x
#define CONCAT(x,y) x ## y

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define EQUAL(a,b) (fabs((a)-(b))<DBL_EPSILON)
#define IN_BAR(x,a,b) (MIN(a,b)<x && x<MAX(a,b)) // a<x<b 
#define ON_BAR(x,a,b) (MIN(a,b)<=x && x<=MAX(a,b)) // a<=x<=b
#define IN_AREA(x,y,a,b,c,d) (IN_BAR(x,a,b) && (IN_BAR(y,c,d))) // a<x<b && c<y<d
#define ON_AREA(x,y,a,b,c,d) (ON_BAR(x,a,b) && (ON_BAR(y,c,d))) // a<=x<=b && c<=y<=d

#define CONNECT(a,b,r) (fabs((a)-(b))<(r))
#define CONNECT_2D(x1,y1,x2,y2,r) (fabs((x1)-(x2))>(r)) ? false : (fabs((y1)-(y2))>(r) ? false : ((r)*(r)>((x1)-(x2))*((x1)-(x2))+((y1)-(y2))*((y1)-(y2))))

#define RAND_MAX_DIV (1.0/(double)(RAND_MAX+1.0))
#define DBL_RND_UNIFORM() ((double)rand()*RAND_MAX_DIV)
#define DBL_RND_UNIFORM_RANGE(start,end) (DBL_RND_UNIFORM()*((end)-(start))+(start))
#define DBL_RND_NORMAL() (sqrt(-2.0*log(DBL_RND_UNIFORM()))*cos(2.0*UTILITY_M_PI*DBL_RND_UNIFORM()))
#define DBL_RND_EXP(lambda) (-log(DBL_RND_UNIFORM())/(lambda))
#define DBL_RND_EXP_FIXED(lambda) (log(DBL_RND_UNIFORM())*(lambda)) // lambda_fixed = -1.0/lambda
#define PROBABILITY(percent) ((percent)>=DBL_RND_UNIFORM())

#define CNV_UNIFORM(rnd,start,end) ((rnd)*((end)-(start))+(start))
#define CNV_NORMAL(rnd1,rnd2) (sqrt(-2.0*log(rnd1))*cos(2.0*UTILITY_M_PI*(rnd2)))
#define CNV_EXP(rnd,lambda) (-log(rnd)/(lambda))
#define CNV_EXP_FIXED(rnd,lambda) (log(rnd)*(lambda)) // lambda_fixed = -1.0/lambda
#define RND_PROB(percent,rnd) ((percent)>=(rnd))

#endif