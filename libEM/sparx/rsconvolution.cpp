/*
 * Author: Pawel A.Penczek, 09/09/2006 (Pawel.A.Penczek@uth.tmc.edu)
 * Copyright (c) 2000-2006 The University of Texas - Houston Medical School
 *
 * This software is issued under a joint BSD/GNU license. You may use the
 * source code in this file under either license. However, note that the
 * complete EMAN2 and SPARX software packages have some GPL dependencies,
 * so you are responsible for compliance with the licenses of these packages
 * if you opt to use BSD licensing. The warranty disclaimer below holds
 * in either instance.
 *
 * This complete copyright notice must be included in any revised version of the
 * source code. Additional authorship citations may be added, but existing
 * author citations must be preserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "emdata.h"

#include <algorithm>

#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp;

using namespace EMAN;
using std::swap;

namespace {
	// K(i,j,k)*f(a-i, b-j, c-k) <-- internal, so no boundary condition issues
	inline float mult_internal(EMData& K, EMData& f, 
						       int kzmin, int kzmax, int kymin, int kymax, 
							   int kxmin, int kxmax, 
							   int iz, int iy, int ix) {
		float sum = 0.f;
		for (int kz = kzmin; kz <= kzmax; kz++) {
			for (int ky = kymin; ky <= kymax; ky++) {
				for (int kx = kxmin; kx <= kxmax; kx++) {
					float Kp = K(kx,ky,kz);
					float fp = f(ix-kx,iy-ky,iz-kz);
					sum += Kp*fp;
				}
			}
		}
		return sum;
	}
	// K(i,j,k)*f(a-i, b-j, c-k) <-- Circulant boundary conditions
	inline float mult_circ(EMData& K, EMData& f, int kzmin, int kzmax,
			               int kymin, int kymax, int kxmin, int kxmax,
						   int nzf, int nyf, int nxf, int iz, int iy, int ix) {
		float sum = 0.f;
		for (int kz = kzmin; kz <= kzmax; kz++) {
			int jz = (iz - kz) % nzf;
			if (jz < 0) jz += nzf;
			for (int ky = kymin; ky <= kymax; ky++) {
				int jy = (iy - ky) % nyf; 
				if (jy < 0) jy += nyf;
				for (int kx = kxmin; kx <= kxmax; kx++) {
					int jx = (ix - kx) % nxf; 
					if (jx < 0) jx += nxf;
					float Kp = K(kx,ky,kz);
					float fp = f(jx,jy,jz);
					sum += Kp*fp;
				}
			}
		}
		return sum;
	}
	// In the future we may want to add other boundary conditions here
	
	inline float select_nth_largest(int k, int n, float *arr)
	{
		int i, ir, j, l, mid;
		float a, temp;

		l = 1;
		ir = n;
		for (;;) {
			if (ir <= l+1) {
				if (ir == l+1 && arr[ir] < arr[l] ) {
					SWAP(arr[l],arr[ir])
				}
				return arr[k];
			} else {
				mid = (l+ir) >> 1;
				SWAP(arr[mid], arr[l+1])
				if (arr[l+1] > arr[ir]) {
					SWAP(arr[l+1],arr[ir])
				}
				if (arr[l] > arr[ir]) {
					SWAP(arr[l],arr[ir])
				}
				if (arr[l+1] > arr[l]) {
					SWAP(arr[l+1],arr[l])
				}
				i = l+1;
				j = ir;
				a = arr[l];
				for (;;) {
					do i++; while (arr[i] < a);
					do j--; while (arr[j] > a);
					if (j < i) break;
					SWAP(arr[i],arr[j])
				}
				arr[l] = arr[j];
				arr[j] = a;
				if (j >= k) ir = j-1;
				if (j <= k) l = i;
			}
		}
	}

	inline float median(EMData& f, int nxk, int nyk, int nzk, kernel_shape myshape, int iz, int iy, int ix) {
		int index = 0;
		int dimension = 3;
		float median_value = 0.f;
		float *table;

		int nxf = (&f)->get_xsize();
		int nyf = (&f)->get_ysize();
		int nzf = (&f)->get_zsize();

		int nxk2 = (nxk-1)/2;
		int nyk2 = (nyk-1)/2;
		int nzk2 = (nzk-1)/2;

		int kzmin = iz-nzk2;
		int kzmax = iz+nzk2;
		int kymin = iy-nyk2;
		int kymax = iy+nyk2;
		int kxmin = ix-nxk2;
		int kxmax = ix+nxk2;

		if ( nzf == 1 ) {
			dimension--;
			if ( nyf == 1 )  dimension--; 
		}

		switch (myshape) {
		case BLOCK:
			switch (dimension) {
			case 1: 
				table = (float*)malloc(nxk*sizeof(float));
				break;
			case 2: table = (float*)malloc(nxk*nyk*sizeof(float));
				break;
			case 3: table = (float*)malloc(nxk*nyk*nzk*sizeof(float));
			 	break;
			}	
			for (int kz = kzmin; kz <= kzmax; kz++) {
				int jz = kz < 0 ? kz+nzf : kz % nzf;
				for (int ky = kymin; ky <= kymax; ky++) {
					int jy = ky < 0 ? ky+nyf : ky % nyf; 
					for (int kx = kxmin; kx <= kxmax; kx++) {
						int jx = kx < 0 ? kx+nxf : kx % nxf; 
						table[index] = f(jx,jy,jz);
						index++;
					}
				}
			}
			break;
		case CIRCULAR:
			switch (dimension) {
			case 1: 
				table = (float*)malloc(nxk*sizeof(float));
				break;
			case 2: table = (float*)malloc(nxk*nxk*sizeof(float));
				break;
			case 3: table = (float*)malloc(nxk*nxk*nxk*sizeof(float));
			 	break;
			}	
			for (int kz = kzmin; kz <= kzmax; kz++) {
				int jz = kz < 0 ? kz+nzf : kz % nzf;
				for (int ky = kymin; ky <= kymax; ky++) {
					int jy = ky < 0 ? ky+nyf : ky % nyf; 
					for (int kx = kxmin; kx <= kxmax; kx++) {
						int jx = kx < 0 ? kx+nxf : kx % nxf; 
						if ( (kz-iz)*(kz-iz)+(ky-iy)*(ky-iy)+(kx-ix)*(kx-ix) <= nxk2*nxk2 ) {
							table[index] = f(jx,jy,jz);
							index++;
						}
					}
				}
			}
			break;
		case CROSS:
			if ( nzf != 1 )  {
				table = (float*)malloc((nxk+nyk+nzk-2)*sizeof(float));
				for (int kz = kzmin; kz <= kzmax; kz++) {
					int jz = kz < 0 ? kz+nzf : kz % nzf;
					if ( kz != iz ) { table[index] = f(ix,iy,jz); index++; }
				}
				for (int ky = kymin; ky <= kymax; ky++) {
					int jy = ky < 0 ? ky+nyf : ky % nyf; 
					if ( ky != iy ) { table[index] = f(ix,jy,iz); index++; }
				}
				for (int kx = kxmin; kx <= kxmax; kx++) {
					int jx = kx < 0 ? kx+nxf : kx % nxf; 
					table[index] = f(jx,iy,iz);
					index++;
				}
			} else if  ( nyf != 1 ) {
				table = (float*)malloc((nxk+nyk-1)*sizeof(float));
				for (int ky = kymin; ky <= kymax; ky++) {
					int jy = ky < 0 ? ky+nyf : ky % nyf; 
					if ( ky != iy ) { table[index] = f(ix,jy,iz); index++; }
				}
				for (int kx = kxmin; kx <= kxmax; kx++) {
					int jx = kx < 0 ? kx+nxf : kx % nxf; 
					table[index] = f(jx,iy,iz);
					index++;
				}
			} else {
				table = (float*)malloc(nxk*sizeof(float));
				for (int kx = kxmin; kx <= kxmax; kx++) {
					int jx = kx < 0 ? kx+nxf : kx % nxf; 
					table[index] = f(jx,iy,iz);
					index++;
				}
			}
			break;
		default: throw ImageDimensionException("Illegal Kernal Shape!");
		}
		median_value=select_nth_largest((index+1)/2, index, table-1);
		free((void *)table);
		return median_value;
	}
}

namespace EMAN {

    EMData* rsconvolution(EMData* f, EMData* K) {
		// Kernel should be the smaller image
		int nxf=f->get_xsize(); int nyf=f->get_ysize(); int nzf=f->get_zsize();
		int nxK=K->get_xsize(); int nyK=K->get_ysize(); int nzK=K->get_zsize();
		if ((nxf<nxK)&&(nyf<nyK)&&(nzf<nzK)) {
			// whoops, f smaller than K
			swap(f,K); swap(nxf,nxK); swap(nyf,nyK); swap(nzf,nzK);
		} else if ((nxK<=nxf)&&(nyK<=nyf)&&(nzK<=nzf)) {
			// that's what it should be, so do nothing
			;
		} else {
			// incommensurate sizes
			throw ImageDimensionException("input images are incommensurate");
		}
		// Kernel needs to be _odd_ in size
		if ((nxK % 2 != 1) || (nyK % 2 != 1) || (nzK % 2 != 1))
			throw ImageDimensionException("Real-space convolution kernel"
				" must have odd nx,ny,nz (so the center is well-defined).");
		EMData* result = new EMData();
		result->set_size(nxf, nyf, nzf);
		result->to_zero();
		// kernel corners, need to check for degenerate case
		int kxmin = -nxK/2; int kymin = -nyK/2; int kzmin = -nzK/2;
		int kxmax = (1 == nxK % 2) ? -kxmin : -kxmin - 1;
		int kymax = (1 == nyK % 2) ? -kymin : -kymin - 1;
		int kzmax = (1 == nzK % 2) ? -kzmin : -kzmin - 1;
		vector<int> K_saved_offsets = K->get_array_offsets();
		K->set_array_offsets(kxmin,kymin,kzmin);
		// interior boundaries, need to check for degenerate cases
		int izmin = 0, izmax = 0, iymin = 0, iymax = 0, ixmin = 0, ixmax = 0;
		if (1 != nzf) {
			izmin = -kzmin;
			izmax = nzf - 1 - kzmax;
		}
		if (1 != nyf) {
			iymin = -kymin;
			iymax = nyf - 1 - kymax;
		}
		if (1 != nxf) {
			ixmin = -kxmin;
			ixmax = nxf - 1 - kxmax;
		}
		// interior (no boundary condition issues here)
		for (int iz = izmin; iz <= izmax; iz++) {
			for (int iy = iymin; iy <= iymax; iy++) {
				for (int ix = ixmin; ix <= ixmax; ix++) {
					(*result)(ix,iy,iz) =
						mult_internal(*K, *f, 
								      kzmin, kzmax, kymin, kymax, kxmin, kxmax,
									  iz, iy, ix);
				}
			}
		}
		// corners
		// corner sizes, with checking for degenerate cases
		int sz = (1 == nzK) ? 1 : -kzmin + kzmax;
		int sy = (1 == nyK) ? 1 : -kymin + kymax;
		int sx = (1 == nxK) ? 1 : -kxmin + kxmax;
		// corner starting locations, with checking for degenerate cases
		int zstart = (0 == izmin) ? 0 : izmin - 1;
		int ystart = (0 == iymin) ? 0 : iymin - 1;
		int xstart = (0 == ixmin) ? 0 : ixmin - 1;
		// corners
		for (int cz = 0; cz < sz; cz++) {
			int iz = (zstart - cz) % nzf;
			if (iz < 0) iz += nzf;
			for (int cy = 0; cy < sy; cy++) {
				int iy = (ystart - cy) % nyf;
				if (iy < 0) iy += nyf;
				for (int cx=0; cx < sx; cx++) {
					int ix = (xstart - cx) % nxf;
					if (ix < 0) ix += nxf;
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, 
								 kymax, kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		// remaining stripes -- should use a more elegant (non-3D-specific) method here
		// ix < ixmin
		for (int ix = 0; ix < ixmin; ix++) {
			for (int iy = iymin; iy <= iymax; iy++) {
				for (int iz = izmin; iz <= izmax; iz++) {
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, kymax, 
								 kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		// ix > ixmax
		for (int ix = ixmax+1; ix < nxf; ix++) {
			for (int iy = iymin; iy <= iymax; iy++) {
				for (int iz = izmin; iz <= izmax; iz++) {
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, kymax, 
								 kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		// iy < iymin
		for (int iy = 0; iy < iymin; iy++) {
			for (int ix = ixmin; ix <= ixmax; ix++) {
				for (int iz = izmin; iz <= izmax; iz++) {
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, kymax, 
								 kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		// iy > iymax
		for (int iy = iymax+1; iy < nyf; iy++) {
			for (int ix = ixmin; ix <= ixmax; ix++) {
				for (int iz = izmin; iz <= izmax; iz++) {
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, kymax, 
								 kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		// iz < izmin
		for (int iz = 0; iz < izmin; iz++) {
			for (int ix = ixmin; ix <= ixmax; ix++) {
				for (int iy = iymin; iy <= iymax; iy++) {
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, kymax, 
								 kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		// iz > izmax
		for (int iz = izmax+1; iz < nzf; iz++) {
			for (int ix = ixmin; ix <= ixmax; ix++) {
				for (int iy = iymin; iy <= iymax; iy++) {
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, kymax, 
								 kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		K->set_array_offsets(K_saved_offsets);
		result->done_data();
		return result;
	}

    EMData* filt_median(EMData* f, int nxk, int nyk, int nzk, kernel_shape myshape) {
		
 		int nxf = f->get_xsize();
		int nyf = f->get_ysize(); 
		int nzf = f->get_zsize();
		
		if ( nxk > nxf || nyk > nyf || nzk > nzf ) {
			// Kernel should be smaller than the size of image
			throw ImageDimensionException("Kernel should be smaller than the size of image.");
		}	

		if ( nxk % 2 != 1 || nyk % 2 != 1 || nzk % 2 != 1 ) {
			// Kernel needs to be odd in size
			throw ImageDimensionException("Real-space kernel must have odd size (so the center is well-defined).");
		}

		if ( myshape == CIRCULAR ) {
			// For CIRCULAR kernal, size must be same on all dimensions
			if ( nzf != 1 && ( nxk != nyk || nxk != nzk ) || nzf == 1 && nyf != 1 && nxk != nyk ) {
				throw ImageDimensionException("For CIRCULAR kernal, size must be same on all dimensions.");
			}
		}

		EMData* result = new EMData();
		result->set_size(nxf, nyf, nzf);
		result->to_zero();

		for (int iz = 0; iz <= nzf-1; iz++) {
			for (int iy = 0; iy <= nyf-1; iy++) {
				for (int ix = 0; ix <= nxf-1; ix++) {
					(*result)(ix,iy,iz) = median (*f, nxk, nyk, nzk, myshape, iz, iy, ix);					
				}
			}
		}
		
		return result;
	}

}

/*
namespace {
	// K(i,j,k)*f(a-i, b-j, c-k) <-- internal, so no boundary condition issues
	inline float kmlt_internal(EMData& K, EMData& f, 
						       int kzmin, int kzmax, int kymin, int kymax, 
							   int kxmin, int kxmax, 
							   int iz, int iy, int ix) {
		float sum = 0.f;
		for (int kz = kzmin; kz <= kzmax; kz++) {
			for (int ky = kymin; ky <= kymax; ky++) {
				for (int kx = kxmin; kx <= kxmax; kx++) {
					float Kp = K(kx,ky,kz);
					float fp = f(ix-kx,iy-ky,iz-kz);
					sum += Kp*fp;
				}
			}
		}
		return sum;
	}
	// K(i,j,k)*f(a-i, b-j, c-k) <-- Circulant boundary conditions
	inline float kmlt_circ(EMData& K, EMData& f, int kzmin, int kzmax,
			               int kymin, int kymax, int kxmin, int kxmax,
						   int nzf, int nyf, int nxf, int iz, int iy, int ix) {
		float sum = 0.f;
		for (int kz = kzmin; kz <= kzmax; kz++) {
			int jz = (iz - kz) % nzf;
			if (jz < 0) jz += nzf;
			for (int ky = kymin; ky <= kymax; ky++) {
				int jy = (iy - ky) % nyf; 
				if (jy < 0) jy += nyf;
				for (int kx = kxmin; kx <= kxmax; kx++) {
					int jx = (ix - kx) % nxf; 
					if (jx < 0) jx += nxf;
					float Kp = K(kx,ky,kz);
					float fp = f(jx,jy,jz);
					sum += Kp*fp;
				}
			}
		}
		return sum;
	}
	// In the future we may want to add other boundary conditions here
}
namespace EMAN {

    EMData* rscp(EMData* f) {
		// Kernel should be the smaller image
		int nxf=f->get_xsize(); int nyf=f->get_ysize(); int nzf=f->get_zsize();
		const int npad = 2;
		const int m = Util::get_min(nxf,nyf,nzf);
		const int n = m*npad;

		const int K = 6;  //params["kb_K"];
		const float alpha = 1.75;  //params["kb_alpha"];
		Util::KaiserBessel kb(alpha, K, m/2,K/(2.*n),n);

                int nxK = K/2+1; nyK=nxK; nzK=nxK;

		EMData* result = new EMData();
		result->set_size(nxf, nyf, nzf);
		result->to_zero();
		// kernel corners, need to check for degenerate case
		int kxmin = -nxK/2; int kymin = -nyK/2; int kzmin = -nzK/2;
		int kxmax = (1 == nxK % 2) ? -kxmin : -kxmin - 1;
		int kymax = (1 == nyK % 2) ? -kymin : -kymin - 1;
		int kzmax = (1 == nzK % 2) ? -kzmin : -kzmin - 1;
		// interior boundaries, need to check for degenerate cases
		int izmin = 0, izmax = 0, iymin = 0, iymax = 0, ixmin = 0, ixmax = 0;
		if (1 != nzf) {
			izmin = -kzmin;
			izmax = nzf - 1 - kzmax;
		}
		if (1 != nyf) {
			iymin = -kymin;
			iymax = nyf - 1 - kymax;
		}
		if (1 != nxf) {
			ixmin = -kxmin;
			ixmax = nxf - 1 - kxmax;
		}
		// interior (no boundary condition issues here)
		for (int iz = izmin; iz <= izmax; iz++) {
			for (int iy = iymin; iy <= iymax; iy++) {
				for (int ix = ixmin; ix <= ixmax; ix++) {
					(*result)(ix,iy,iz) =
						mult_internal(*K, *f, 
								      kzmin, kzmax, kymin, kymax, kxmin, kxmax,
									  iz, iy, ix);
				}
			}
		}
		//   INITIALLY SKIP IT / corners  
		// corner sizes, with checking for degenerate cases
		int sz = (1 == nzK) ? 1 : -kzmin + kzmax;
		int sy = (1 == nyK) ? 1 : -kymin + kymax;
		int sx = (1 == nxK) ? 1 : -kxmin + kxmax;
		// corner starting locations, with checking for degenerate cases
		int zstart = (0 == izmin) ? 0 : izmin - 1;
		int ystart = (0 == iymin) ? 0 : iymin - 1;
		int xstart = (0 == ixmin) ? 0 : ixmin - 1;
		// corners
		for (int cz = 0; cz < sz; cz++) {
			int iz = (zstart - cz) % nzf;
			if (iz < 0) iz += nzf;
			for (int cy = 0; cy < sy; cy++) {
				int iy = (ystart - cy) % nyf;
				if (iy < 0) iy += nyf;
				for (int cx=0; cx < sx; cx++) {
					int ix = (xstart - cx) % nxf;
					if (ix < 0) ix += nxf;
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, 
								 kymax, kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		// remaining stripes -- should use a more elegant (non-3D-specific) method here
		// ix < ixmin
		for (int ix = 0; ix < ixmin; ix++) {
			for (int iy = iymin; iy <= iymax; iy++) {
				for (int iz = izmin; iz <= izmax; iz++) {
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, kymax, 
								 kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		// ix > ixmax
		for (int ix = ixmax+1; ix < nxf; ix++) {
			for (int iy = iymin; iy <= iymax; iy++) {
				for (int iz = izmin; iz <= izmax; iz++) {
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, kymax, 
								 kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		// iy < iymin
		for (int iy = 0; iy < iymin; iy++) {
			for (int ix = ixmin; ix <= ixmax; ix++) {
				for (int iz = izmin; iz <= izmax; iz++) {
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, kymax, 
								 kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		// iy > iymax
		for (int iy = iymax+1; iy < nyf; iy++) {
			for (int ix = ixmin; ix <= ixmax; ix++) {
				for (int iz = izmin; iz <= izmax; iz++) {
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, kymax, 
								 kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		// iz < izmin
		for (int iz = 0; iz < izmin; iz++) {
			for (int ix = ixmin; ix <= ixmax; ix++) {
				for (int iy = iymin; iy <= iymax; iy++) {
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, kymax, 
								 kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		// iz > izmax
		for (int iz = izmax+1; iz < nzf; iz++) {
			for (int ix = ixmin; ix <= ixmax; ix++) {
				for (int iy = iymin; iy <= iymax; iy++) {
					(*result)(ix,iy,iz) =
						mult_circ(*K, *f, kzmin, kzmax, kymin, kymax, 
								 kxmin, kxmax,
								 nzf, nyf, nxf, iz, iy, ix);
				}
			}
		}
		//K->set_array_offsets(K_saved_offsets);
		result->done_data();
		return result;
	}

}
*/        
/* vim: set ts=4 noet: */
