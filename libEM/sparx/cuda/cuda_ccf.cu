/*
 * Copyright 1993-2007 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO USER:
 *
 * This source code is subject to NVIDIA ownership rights under U.S. and
 * international Copyright laws.  Users and possessors of this source code
 * are hereby granted a nonexclusive, royalty-free license to use this code
 * in individual and commercial software.
 *
 * NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR
 * IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOURCE CODE.
 *
 * U.S. Government End Users.   This source code is a "commercial item" as
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of
 * "commercial computer  software"  and "commercial computer software
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995)
 * and is provided to the U.S. Government only as a commercial end item.
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the
 * source code with only those rights set forth herein.
 *
 * Any use of this source code in individual and commercial software must
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */

/* This example demonstrates how to use the CUBLAS library
 * to realize apmq.
 */

/* Includes, system */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* Includes, cuda, cufft */
#include <cufft.h>


/* Matrix size */
#define PI (3.14159265358979)
#define NIMAGE_ROW (512)
#define NIMAGE_IN_TEXTURE (15000)
#define MAX_RING_LENGTH (256)

__global__ void complex_mul(float *ccf, int BLOCK_SIZE, int NRING, int NIMAGE, int KX, int KY);
__global__ void resample_to_polar(float* image, float dx, float dy, int NX, int NY, int RING_LENGTH, int NRING);

texture<float, 2, cudaReadModeElementType> tex;
texture<float, 1, cudaReadModeElementType> texim_subject;
texture<float, 1, cudaReadModeElementType> texim_ref;
texture<float, 1, cudaReadModeElementType> texim_points;
texture<float, 1, cudaReadModeElementType> texim_shifts;
 
/* Main */
void calculate_ccf(float *subject_image, float *ref_image, float *ccf, int NIMAGE, int NX, int NY, int RING_LENGTH, int NRING, float STEP, int KX, int KY)
{    
    cufftHandle plan_subject_image, plan_ref_image, plan_ccf;
    cufftPlan1d(&plan_subject_image, RING_LENGTH, CUFFT_R2C, NRING*NIMAGE);
    cufftPlan1d(&plan_ref_image, RING_LENGTH, CUFFT_R2C, NRING);
    cufftPlan1d(&plan_ccf, RING_LENGTH, CUFFT_C2R, NIMAGE*(2*KX+1)*(2*KY+1)*2);
    float *subject_image_polar, *ref_image_polar;
    float *d_subject_image_polar, *d_ref_image_polar;
    float *d_ccf;
    float *points, *d_points;
    float *shifts, *d_shifts;
    int i, j, k, index;
    int ccf_base_addr;
    float x, y, ang;

    int BLOCK_SIZE = RING_LENGTH/2+1;
    int NROW = NIMAGE/NIMAGE_ROW;
    int NIMAGE_LEFT = NIMAGE%NIMAGE_ROW;
    int NTEXTURE = NIMAGE/NIMAGE_IN_TEXTURE;
    int NIMAGE_LEFT_TEXTURE = NIMAGE%NIMAGE_IN_TEXTURE;

    cudaArray *ref_image_array, *subject_image_array[NROW], *subject_image_array_left;
    dim3 GridSize1(NRING, NIMAGE_ROW);
    dim3 GridSize2(NRING, NIMAGE_LEFT);
    cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc<float>();
    const textureReference *texPtr;
 
    tex.normalized = false;
    tex.filterMode = cudaFilterModeLinear;
    tex.addressMode[0] = cudaAddressModeWrap;
    tex.addressMode[1] = cudaAddressModeWrap;


    /* Allocate host memory for the matrix for NIMAGE subject images in polar coordinates */
    subject_image_polar = (float*)malloc(NIMAGE*(RING_LENGTH+2)*NRING*sizeof(float));
    if (subject_image_polar == 0) {
        fprintf (stderr, "Host memory allocation error!\n");
        return;
    }

    /* Allocate host memory for the matrix for the reference image in polar coordinates */
    ref_image_polar = (float*)malloc((RING_LENGTH+2)*NRING*sizeof(float));
    if (ref_image_polar == 0) {
        fprintf (stderr, "Host memory allocation error!\n");
        return;
    }

    /* Allocate host memory for the coordinates of sampling points */
    points = (float*)malloc(RING_LENGTH*NRING*2*sizeof(float));
    if (points == 0) {
       fprintf (stderr, "Host memory allocation error!\n");
       return;
    }

    /* Allocate host memory for the coordinates of all shifts */
    shifts = (float*)malloc(NIMAGE_ROW*2*sizeof(float));
    if (shifts == 0) {
       fprintf (stderr, "Host memory allocation error!\n");
       return;
    }

    printf("Initialization on the host memory done.\n");


    printf("\nMemory to be allocated on the video card:\n");
    printf("For %5d subject images                      : %10.3f MB\n", NIMAGE, NIMAGE*NX*NY*4/1000000.0);
    printf("For reference image                           : %10.3f KB\n", NX*NY*4/1000.0);
    printf("For %5d subject images in polar coordinates : %10.3f MB\n", NIMAGE, NIMAGE*(RING_LENGTH+2)*NRING*4/1000000.0);
    printf("For reference image in polar coordinates      : %10.3f KB\n", (RING_LENGTH+2)*NRING*4/1000.0);
    printf("For all cross-correlation functions (CCF)     : %10.3f MB\n", (RING_LENGTH+2)*NIMAGE*(2*KX+1)*(2*KY+1)*2*4/1000000.0);
    printf("Total memory used                             : %10.3f MB\n\n", ((NIMAGE+1)*(NX*NY+(RING_LENGTH+2)*NRING)+(RING_LENGTH+2)*NIMAGE*(2*KX+1)*(2*KY+1)*2)*4/1000000.0);


    /* Allocate the matrix for all NIMAGE subject images on the video card */
    for (k=0; k<NROW; k++)
       cudaMallocArray(&subject_image_array[k], &channelDesc, NX, NY*NIMAGE_ROW);
    if (NIMAGE_LEFT != 0)
       cudaMallocArray(&subject_image_array_left, &channelDesc, NX, NY*NIMAGE_LEFT);
 
    /* Allocate the matrix for the reference image on the video card */
    cudaMallocArray(&ref_image_array, &channelDesc, NX, NY);
 
    /* Allocate the matrix for all NIMAGE subject images in polar coordinates on the video card */
    cudaMalloc((void**)&d_subject_image_polar, NIMAGE*(RING_LENGTH+2)*NRING*sizeof(float));
   
    /* Allocate the matrix for the reference image in polar coordinates on the video card */
    cudaMalloc((void**)&d_ref_image_polar, (RING_LENGTH+2)*NRING*sizeof(float));
 
    /* Allocate the matrix for the ccf on the video card */
    cudaMalloc((void**)&d_ccf, 2*(RING_LENGTH+2)*NIMAGE*(2*KX+1)*(2*KY+1)*sizeof(float));

    /* Allocate the matrix for the coordinates of the sampling points on the video card */
    cudaMalloc((void**)&d_points, RING_LENGTH*NRING*2*sizeof(float));

    /* Allocate the matrix for the coordinates of the shifts on the video card */
    cudaMalloc((void**)&d_shifts, NIMAGE_ROW*2*sizeof(float));
 

    /* Fill the matrix for the coordinates of sampling points */
    for (i = 0; i < NRING; i++) 
    	for (j = 0; j < RING_LENGTH; j++) {
		index = i*RING_LENGTH+j;
		ang = float(j)/RING_LENGTH*PI*2;
 	      	points[index*2] = (i+1)*cosf(ang);
		points[index*2+1] = (i+1)*sinf(ang);
    	}
	
    /* Fill the matrix for the coordinates of shifts */
    for (i = 0; i < NIMAGE_ROW*2; i++)    	shifts[i] = 0.0f;


    /* Copy the matrix for the coordinates of sampling points to the video card */
    cudaMemcpy(d_points, points, RING_LENGTH*NRING*2*sizeof(float), cudaMemcpyHostToDevice);
    cudaGetTextureReference(&texPtr, "texim_points");
    cudaBindTexture(0, texPtr, d_points, &channelDesc, RING_LENGTH*NRING*2*sizeof(float));

    /* Copy the matrix for the coordinates of shifts to the video card */
    cudaMemcpy(d_shifts, shifts, NIMAGE_ROW*2*sizeof(float), cudaMemcpyHostToDevice);
    cudaGetTextureReference(&texPtr, "texim_shifts");
    cudaBindTexture(0, texPtr, d_shifts, &channelDesc, NIMAGE_ROW*2*sizeof(float));

    /* Copy the matrix for the reference image to the video card */
    cudaMemcpyToArray(ref_image_array, 0, 0, ref_image,  NX*NY*sizeof(float), cudaMemcpyHostToDevice);
    cudaBindTextureToArray(tex, ref_image_array, channelDesc);
 
    /* Convert the reference image to polar coordinates */
    resample_to_polar<<<NRING, RING_LENGTH>>>(d_ref_image_polar, 0.0, 0.0, NX, NY, RING_LENGTH, NRING);
    cudaThreadSynchronize();
    cudaUnbindTexture(tex);

    /* Conduct FFT of the reference image in polar coordinates */
    cufftExecR2C(plan_ref_image, (cufftReal *)d_ref_image_polar, (cufftComplex *)d_ref_image_polar);


    /* Copy the matrix for NIMAGE_ROW*NROW subject images to the video card */
    for (k=0; k<NROW; k++)  
        cudaMemcpyToArray(subject_image_array[k], 0, 0, subject_image+k*NIMAGE_ROW*NX*NY, NIMAGE_ROW*NX*NY*sizeof(float), cudaMemcpyHostToDevice);
    /* Copy the matrix for NIMAGE_LEFT subject images to the video card */
    if (NIMAGE_LEFT != 0) 
       	cudaMemcpyToArray(subject_image_array_left, 0, 0, subject_image+NROW*NIMAGE_ROW*NX*NY, NIMAGE_LEFT*NX*NY*sizeof(float), cudaMemcpyHostToDevice);
   
    for (i=-KX; i<=KX; i++) {
	for (j=-KY; j<=KY; j++) {
		
		x = i*STEP;
		y = j*STEP;

                for (k=0; k<NROW; k++) { 
    			cudaBindTextureToArray(tex, subject_image_array[k], channelDesc);

			/* Convert NIMAGE_ROW subject images to the polar coordinates */
			resample_to_polar<<<GridSize1, RING_LENGTH>>>(d_subject_image_polar+k*NIMAGE_ROW*(RING_LENGTH+2)*NRING, x, y, NX, NY, RING_LENGTH, NRING);
                	cudaThreadSynchronize();
     			cudaUnbindTexture(tex);
		}
		if (NIMAGE_LEFT != 0) {
    			cudaBindTextureToArray(tex, subject_image_array_left, channelDesc);
   		
			/* Convert NIMAGE_LEFT subject images to the polar coordinates */
			resample_to_polar<<<GridSize2, RING_LENGTH>>>(d_subject_image_polar+NROW*NIMAGE_ROW*(RING_LENGTH+2)*NRING, x, y, NX, NY, RING_LENGTH, NRING);
                	cudaThreadSynchronize();
    			cudaUnbindTexture(tex);
		}

		/* Conduct FFT for all subject images */
		cufftExecR2C(plan_subject_image, (cufftReal *)d_subject_image_polar, (cufftComplex *)d_subject_image_polar);

		cudaGetTextureReference(&texPtr, "texim_ref");
		cudaBindTexture(0, texPtr, d_ref_image_polar, &channelDesc, (RING_LENGTH+2)*NRING*sizeof(float));
   	
		cudaGetTextureReference(&texPtr, "texim_subject");
                ccf_base_addr = ((j+KY)*(2*KX+1)+(i+KX))*NIMAGE*(RING_LENGTH+2);
		for (k=0; k<NTEXTURE; k++) {
			cudaBindTexture(0, texPtr, d_subject_image_polar+k*NIMAGE_IN_TEXTURE*(RING_LENGTH+2)*NRING, &channelDesc, NIMAGE_IN_TEXTURE*(RING_LENGTH+2)*NRING*sizeof(float));
			complex_mul<<<NIMAGE_IN_TEXTURE, BLOCK_SIZE>>>(d_ccf+ccf_base_addr+k*NIMAGE_IN_TEXTURE*(RING_LENGTH+2), BLOCK_SIZE, NRING, NIMAGE, KX, KY);
		}
		if (NIMAGE_LEFT_TEXTURE != 0) {
			cudaBindTexture(0, texPtr, d_subject_image_polar+NTEXTURE*NIMAGE_IN_TEXTURE*(RING_LENGTH+2)*NRING, &channelDesc, NIMAGE_LEFT_TEXTURE*(RING_LENGTH+2)*NRING*sizeof(float));
			complex_mul<<<NIMAGE_LEFT_TEXTURE, BLOCK_SIZE>>>(d_ccf+ccf_base_addr+NTEXTURE*NIMAGE_IN_TEXTURE*(RING_LENGTH+2), BLOCK_SIZE, NRING, NIMAGE, KX, KY);
		}
		cudaUnbindTexture(texim_subject);
		cudaUnbindTexture(texim_ref);
	}
    }
    cudaUnbindTexture(texim_points);
    cudaUnbindTexture(texim_shifts); 

    cufftExecC2R(plan_ccf, (cufftComplex *)d_ccf, (cufftReal *)d_ccf);

    cudaMemcpy(ccf, d_ccf, 2*(RING_LENGTH+2)*NIMAGE*(2*KX+1)*(2*KY+1)*sizeof(float), cudaMemcpyDeviceToHost);


    /* Memory clean up */
    free(subject_image_polar);
    free(ref_image_polar);
    for (k=0; k<NROW; k++)
	cudaFreeArray(subject_image_array[k]);
    if (NIMAGE_LEFT!=0)
        cudaFreeArray(subject_image_array_left);
    cudaFreeArray(ref_image_array);
    cudaFree(d_subject_image_polar);
    cudaFree(d_ref_image_polar);
    cudaFree(d_ccf);
    cudaFree(d_points);
    cudaFree(d_shifts);
    cufftDestroy(plan_subject_image);
    cufftDestroy(plan_ref_image);
    cufftDestroy(plan_ccf);

    return;
}


__global__ void complex_mul(float *ccf, int BLOCK_SIZE, int NRING, int NIMAGE, int KX, int KY) {

    // Block index
    int bx = blockIdx.x;

    // Thread index
    int tx = threadIdx.x;

    float ccf_s_real = 0.0;
    float ccf_s_imag = 0.0;
    float ccf_m_real = 0.0;
    float ccf_m_imag = 0.0;
    float sub_real, sub_imag, ref_real, ref_imag;
    int i_block, b_image, subject_index, ref_index, ccf_index;
    float rr_sr, ri_si, rr_si, ri_sr;

    b_image = bx*BLOCK_SIZE*NRING;
    for (int i = 0; i < NRING; i++) {
	i_block = i*BLOCK_SIZE;
	subject_index = (b_image+i_block+tx)*2;
	ref_index = (i_block+tx)*2;

	sub_real = tex1Dfetch(texim_subject, subject_index);
	sub_imag = tex1Dfetch(texim_subject, subject_index+1);
	ref_real = tex1Dfetch(texim_ref, ref_index);
	ref_imag = tex1Dfetch(texim_ref, ref_index+1);
	rr_sr = ref_real*sub_real;
	ri_si = ref_imag*sub_imag;
	rr_si = ref_real*sub_imag;
	ri_sr = ref_imag*sub_real;
	ccf_s_real += rr_sr+ri_si;
	ccf_s_imag += -rr_si+ri_sr;
	ccf_m_real += rr_sr-ri_si;
	ccf_m_imag += -rr_si-ri_sr;
    }

    ccf_index = (bx*BLOCK_SIZE+tx)*2;
    int ccf_OFFSET = BLOCK_SIZE*2*NIMAGE*(2*KX+1)*(2*KY+1);
    ccf[ccf_index] = ccf_s_real;
    ccf[ccf_index+1] = ccf_s_imag;
    ccf[ccf_index+ccf_OFFSET] = ccf_m_real;
    ccf[ccf_index+ccf_OFFSET+1] = ccf_m_imag;
}

__global__ void resample_to_polar(float* image, float dx, float dy, int NX, int NY, int RING_LENGTH, int NRING) {

    // Block index
    int bx = blockIdx.x;
    int by = blockIdx.y;
    
    // Thread index
    int tx = threadIdx.x;

    float cnx = NX/2+dx+0.5;
    float cny = by*NY+NY/2+dy+0.5;

    int img_index = (by*NRING+bx)*(RING_LENGTH+2)+tx;
    int index = bx*RING_LENGTH+tx;
    float points_x = tex1Dfetch(texim_points, index*2);
    float points_y = tex1Dfetch(texim_points, index*2+1);
    float shifts_x = tex1Dfetch(texim_shifts, by*2);
    float shifts_y = tex1Dfetch(texim_shifts, by*2+1);

    image[img_index] = tex2D(tex, cnx+points_x+shifts_x, cny+points_y+shifts_y);
}

