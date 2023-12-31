/*
 * jobbody.c
 *
 *  Created on: 2014.01.14
 *  last modified: Sep 10, 2014
 *
 */

#include<math.h>

#include<fstream>
#include<iostream>
#include<chrono>
#include<thread>
#include<mutex>
#include<condition_variable>

#include"comm.h"

/*---------------------------------------------------
 * The main loop for the simulation
 * ------------------------------------------------*/
void jobbody()
{
	std::ofstream log_file("outInfo.dat", std::ios_base::app);

	void allocateU1d(int);
	void freeU1d(int);
	void boundX();
	void boundY();
	void fluxF(int, int);
	void fluxG(int, int);
	void interpoDY(int, int);
	void interpoDX(int, int);
	void vfluxF(int, int);
	void vfluxG(int, int);
	void updateq(double**, double**, double, int, int, int);
	void getstatus(int, int);
	void postprocess(int step);
	double getdt(int step);

	auto time_begin = std::chrono::steady_clock::now();

	getstatus(0, I0*J0);

	double sum_t = 0.;

	const int threadcount = std::thread::hardware_concurrency();
	std::thread *threads = new std::thread[threadcount];
	std::mutex mutex;
	int sync=0;
	std::condition_variable conditional;
	for (int i=0; i<threadcount; i++)
	{
		threads[i] = std::thread(
			[&](int istart, int iend){
				int Istart = istart + config1.Ng;
				int Iend = iend + config1.Ng;
				int U1dlen = MAX(I0, J0);
				allocateU1d(U1dlen);
				for(int iStep = config1.iStep0; iStep <= config1.nStep; iStep++)
				{
					for(int ic = Istart*J0; ic<Iend*J0; ic++)
					{
						for(int iv = 0; iv<neqv; iv++)
							qo[ic][iv] = U.q[ic][iv];
					}

					double dtc = getdt(iStep);

					for(int ik=0; ik<config1.timeOrder; ik++)
					{
						{
							std::unique_lock<std::mutex> unique(mutex);
							sync++;
							if (sync == threadcount)
							{
								boundX();
								boundY();
								interpoDY(istart, iend);
								interpoDX(istart, iend);
								sync = 0;
								conditional.notify_all();
							}
							else
								conditional.wait(unique);
						}
						fluxF(istart, iend);
						fluxG(istart, iend);
						vfluxF(istart, iend);
						vfluxG(istart, iend);
						updateq(U.q, rhs, dtc, ik, istart, iend);
						getstatus(Istart*J0, Iend*J0);
					}

					{
						std::unique_lock<std::mutex> unique(mutex);
						sync++;
						if (sync == threadcount)
						{
							sum_t = sum_t + dtc;

							if(iStep%config1.Samples == 0)
							{
								auto time_cpu = std::chrono::steady_clock::now() - time_begin;
								double Ttot = config2.t0 + sum_t;

								std::cout << "output flow field result, flow_time = " << Ttot << " s." << std::endl;
								log_file << "istep = " << iStep << ", flow_time = " << Ttot << " s, cpu_time = " << time_cpu.count() / 1e9 << std::endl;
								postprocess(iStep);
							}
							sync = 0;
							conditional.notify_all();
						}
						else
							conditional.wait(unique);
					}
				}
				freeU1d(U1dlen);
			},
			config1.ni*i/threadcount,
			config1.ni*(i+1)/threadcount
		);
	}

	for (int i=0; i<threadcount; i++)
		threads[i].join();

	std::cout << "simulation complete! " << std::endl;
	log_file << "\n Program exit normally! " << std::endl;
	log_file.close();
}

/*---------------------------------------------------
 * Calculate the time step
 * ------------------------------------------------*/
double getdt(int step)
{
	// use CFL number
	double dx = MIN(dxc, dyc);
	double CFL;

	if(step > config1.nRamp)
		CFL = config2.CFL1;
	else
		CFL = config2.CFL0 + step*(config2.CFL1 - config2.CFL0)/config1.nRamp;
	
	double um = config2.MaRef*sqrt(config2.gam0*(ru/config2.molWeight)*config2.temRef);
	return CFL*dx/um;
}

/*------------------------------------------------------
 * update the conservative variables for perfect-gas
 * use Runge-kutta method
 * ------------------------------------------------*/
void updateq(double **q, double **rhs, double dt, int idt, int start, int end)
{
	int i, j, ii, jj, ic, ic1;
	double rho, rhoU, rhoV, rhoE, rrho, yas;
	double coef[3][3] = {
		{1., 3./4., 1./3.},
		{0, 1./4., 2./3.},
		{1., 1./4., 2./3.}
	};

	/* the (:,0), and (config1.ni-1, :) are assigned wall condition */

	for(i=start; i<end; i++)
	{
		ii = i + config1.Ng;
		for(j=0; j<config1.nj; j++)
		{
			jj  = j + config1.Ng;

			ic  = i*config1.nj + j;
			ic1 = ii*J0 + jj;
			yas = mesh.yaks[ic1];

			rho  = q[ic1][0];
			rhoU = q[ic1][0]*q[ic1][1];
			rhoV = q[ic1][0]*q[ic1][2];
			rhoE = q[ic1][0]*q[ic1][3];

			q[ic1][0] = coef[0][idt]*qo[ic1][0]            + coef[1][idt]*rho  + coef[2][idt]*dt*rhs[ic][0]/yas;
			q[ic1][1] = coef[0][idt]*qo[ic1][0]*qo[ic1][1] + coef[1][idt]*rhoU + coef[2][idt]*dt*rhs[ic][1]/yas;
			q[ic1][2] = coef[0][idt]*qo[ic1][0]*qo[ic1][2] + coef[1][idt]*rhoV + coef[2][idt]*dt*rhs[ic][2]/yas;
			q[ic1][3] = coef[0][idt]*qo[ic1][0]*qo[ic1][3] + coef[1][idt]*rhoE + coef[2][idt]*dt*rhs[ic][3]/yas;

			rrho = 1./q[ic1][0];
			q[ic1][1] = q[ic1][1]*rrho;
			q[ic1][2] = q[ic1][2]*rrho;
			q[ic1][3] = q[ic1][3]*rrho;
		}
	}
}

void getstatus(int start, int end)
{
	double rgas0 = (ru/config2.molWeight);
	for (int i=start; i<end; i++)
	{
		double ek = 0.5*(U.q[i][1]*U.q[i][1] + U.q[i][2]*U.q[i][2]);
		U.rgas[i] = rgas0;
		U.gam[i] = config2.gam0;
		U.cv[i] = rgas0/(U.gam[i] - 1);
		U.pre[i] = (U.gam[i] - 1)*(U.q[i][3] - ek)*U.q[i][0];
		U.tem[i] = U.pre[i]/U.q[i][0]/U.rgas[i];
		
		double ratio1 = U.tem[i]/config2.suthC1;
		double ratio2 = (config2.suthC1 + config2.suthC2)/(U.tem[i] + config2.suthC2);
		U.mu[i] = config2.muRef*ratio2*pow(ratio1, 1.5);
		U.kt[i] = U.mu[i]*(U.gam[i]*U.cv[i])/config2.Pr0;
	}
}

/*---------------------------------------------------
 *  postprocess subroutine
 *  Created on: Apr 7, 2014
 * ------------------------------------------------*/
void postprocess(int istep)
{
	char filename[50];
	FILE  *fp;

    sprintf(filename, "nstep%d_field.dat", istep);
    fp = fopen(filename, "w");
    fprintf(fp, "Title = \"Flow field\"\n");
	fprintf(fp, "Variables = x, y, rho, u, v, p, T, e\n");
    fprintf(fp,"ZONE T='1', I= %d, J= %d, f=point \n", config1.ni, config1.nj);

	for(int j=0; j<config1.nj; j++)
		for(int i=0; i<config1.ni; i++)
		{
			int ic1 = (i + config1.Ng) * J0 + j + config1.Ng;
    		double x = mesh.x[ic1];
    		double y = mesh.y[ic1];

    		double rho = U.q[ic1][0];
			double u  =  U.q[ic1][1];
			double v  =  U.q[ic1][2];
			double p  =  U.pre[ic1];
			double T  =  U.tem[ic1];
			double e  =  U.q[ic1][3];
			fprintf(fp, "%8.4e %8.4e %6.4e %6.4e %6.4e %6.4e %10.4f %6.4e", x, y, rho, u, v, p, T, e);
			fprintf(fp, "\n");
    	}
    fclose(fp);
    printf("nstep=%d postprocess complete!!! \n", istep);
}