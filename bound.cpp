/*
 * bound.c
 *
 *  Created on: 2014.01.14
 *
 */

#include"comm.h"

/*---------------------------------------------------
 * Apply boundary condition in i direction
 * ------------------------------------------------*/
void boundX()
{
	int ir = config1.ni + config1.Ng;
	int jr = config1.nj + config1.Ng;

	/* left side, solid wall */
	int ii = 2*config1.Ng - 1; // 5 ->  0, 1, 2
	for(int i=0; i<config1.Ng; i++)
	{
		for(int j=config1.Ng; j<jr; j++)
		{
			int ic = i*J0 + j;
			int ic1 = ii*J0 + j;
			Ug.q[ic][0] =  Ug.q[ic1][0];
			Ug.q[ic][1] =  -Ug.q[ic1][1];
			Ug.q[ic][2] =  -Ug.q[ic1][2];
			Ug.q[ic][3] =  Ug.q[ic1][3];
			Ug.tem[ic]  =  Ug.tem[ic1];
			Ug.pre[ic]  =  Ug.pre[ic1];

			Ug.gam[ic]  =  Ug.gam[ic1];
			Ug.mu[ic]   =  Ug.mu[ic1];
			Ug.kt[ic]   =  Ug.kt[ic1];
		}
		ii -= 1;
	}

	/* right side, solid wall */
	ii = ir -1;  //N+2 ->  N+3,N+4,N+5
	for(int i=ir; i<I0; i++)
	{
		for(int j=config1.Ng; j<jr; j++)
		{
			int ic = i*J0 + j;
			int ic1 = ii*J0 + j;

			Ug.q[ic][0] =  Ug.q[ic1][0];
			Ug.q[ic][1] =  -Ug.q[ic1][1];
			Ug.q[ic][2] =  -Ug.q[ic1][2];
			Ug.q[ic][3] =  Ug.q[ic1][3];
			Ug.tem[ic]  =  Ug.tem[ic1];
			Ug.pre[ic]  =  Ug.pre[ic1];

			Ug.gam[ic]  =  Ug.gam[ic1];
			Ug.mu[ic]   =  Ug.mu[ic1];
			Ug.kt[ic]   =  Ug.kt[ic1];
		}
		ii -= 1;
	}
}

/*---------------------------------------------------
 * Apply boundary condition in j direction
 * ------------------------------------------------*/
void boundY()
{
	 int ir = config1.ni + config1.Ng;
	 int jr = config1.nj + config1.Ng;

	for(int i=config1.Ng; i<ir; i++)
	{
		/* lower side, solid wall */
		int jj = 2*config1.Ng - 1;
		for(int j=0; j<config1.Ng; j++)
		{
			int ic  = i*J0 + j;
			int ic1 = i*J0 + jj;

			Ug.q[ic][0] =  Ug.q[ic1][0];
			Ug.q[ic][1] = -Ug.q[ic1][1];
			Ug.q[ic][2] = -Ug.q[ic1][2];
			Ug.q[ic][3] =  Ug.q[ic1][3];
			Ug.tem[ic]  =  Ug.tem[ic1];
			Ug.pre[ic]  =  Ug.pre[ic1];

			Ug.gam[ic]  =  Ug.gam[ic1];
			Ug.mu[ic]   =  Ug.mu[ic1];
			Ug.kt[ic]   =  Ug.kt[ic1];
			jj -= 1;
		}

    	/* upper side, plate */
		jj = jr -1;  //N+2, N+1, N ->  N+3,N+4,N+5
		for(int j=jr; j<J0; j++)
		{
			int ic = i*J0 + j;
			int ic1 = i*J0 + jj;

		    Ug.q[ic][0] =  Ug.q[ic1][0];
		    Ug.q[ic][1] =  1.0;
		    Ug.q[ic][2] =  -Ug.q[ic1][2];
		    Ug.q[ic][3] =  Ug.q[ic1][3];
		    Ug.tem[ic]  =  Ug.tem[ic1];
		    Ug.pre[ic]  =  Ug.pre[ic1];

		    Ug.gam[ic]  =  Ug.gam[ic1];
		    Ug.mu[ic]   =  Ug.mu[ic1];
		    Ug.kt[ic]   =  Ug.kt[ic1];
	    	jj -= 1;
		}
    }
}