/**
 * $Id$
 */
#include "byteorder.h"
#include "Assert.h"
#include "emconstants.h"
#include "emdata.h"

#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <iomanip>
#include <sstream>

#include <gsl/gsl_sf_bessel.h>
#include <sys/types.h>
#include <boost/algorithm/string.hpp>
#include <gsl/gsl_sf_bessel.h>
#include <algorithm>

#ifndef WIN32
	#include <unistd.h>
	#include <sys/param.h>
#else
	#include <io.h>
	#define access _access
	#define F_OK 00
#endif  //WIN32

using namespace std;
using namespace boost;
using namespace EMAN;

void Util::ap2ri(float *data, size_t n)
{
	Assert(n > 0);
	
	if (!data) {
		throw NullPointerException("pixel data array");
	}

	for (size_t i = 0; i < n; i += 2) {
		float f = data[i] * sin(data[i + 1]);
		data[i] = data[i] * cos(data[i + 1]);
		data[i + 1] = f;
	}
}

void Util::flip_complex_phase(float *data, size_t n)
{
	Assert(n > 0);
	
	if (!data) {
		throw NullPointerException("pixel data array");
	}

	for (size_t i = 0; i < n; i += 2) {
		data[i + 1] *= -1;
	}
}

int Util::file_lock_wait(FILE * file)
{
#ifdef WIN32
	return 1;
#else

	if (!file) {
		throw NullPointerException("Tried to lock NULL file");
	}

	int fdes = fileno(file);

	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
#ifdef WIN32
	fl.l_pid = _getpid();
#else
	fl.l_pid = getpid();
#endif

#if defined __sgi
	fl.l_sysid = getpid();
#endif

	int err = 0;
	if (fcntl(fdes, F_SETLKW, &fl) == -1) {
		LOGERR("file locking error! NFS problem?");

		int i = 0;
		for (i = 0; i < 5; i++) {
			if (fcntl(fdes, F_SETLKW, &fl) != -1) {
				break;
			}
			else {
#ifdef WIN32
				Sleep(1000);
#else
				sleep(1);
#endif

			}
		}
		if (i == 5) {
			LOGERR("Fatal file locking error");
			err = 1;
		}
	}

	return err;
#endif
}



bool Util::check_file_by_magic(const void *first_block, const char *magic)
{
	if (!first_block || !magic) {
		throw NullPointerException("first_block/magic");
	}

	const char *buf = static_cast < const char *>(first_block);

	if (strncmp(buf, magic, strlen(magic)) == 0) {
		return true;
	}
	return false;
}

bool Util::is_file_exist(const string & filename)
{
	if (access(filename.c_str(), F_OK) == 0) {
		return true;
	}
	return false;
}


void Util::flip_image(float *data, size_t nx, size_t ny)
{
	if (!data) {
		throw NullPointerException("image data array");
	}
	Assert(nx > 0);
	Assert(ny > 0);
	
	float *buf = new float[nx];
	size_t row_size = nx * sizeof(float);

	for (size_t i = 0; i < ny / 2; i++) {
		memcpy(buf, &data[i * nx], row_size);
		memcpy(&data[i * nx], &data[(ny - 1 - i) * nx], row_size);
		memcpy(&data[(ny - 1 - i) * nx], buf, row_size);
	}

	if( buf )
	{ 
		delete[]buf;
		buf = 0;
	}
}

bool Util::sstrncmp(const char *s1, const char *s2)
{
	if (!s1 || !s2) {
		throw NullPointerException("Null string");
	}

	if (strncmp(s1, s2, strlen(s2)) == 0) {
		return true;
	}
	
	return false;
}

string Util::int2str(int n)
{
	char s[32] = {'\0'};
	sprintf(s, "%d", n);
	return string(s);
}

string Util::get_line_from_string(char **slines)
{
	if (!slines || !(*slines)) {
		throw NullPointerException("Null string");
	}
	
	string result = "";
	char *str = *slines;

	while (*str != '\n' && *str != '\0') {
		result.push_back(*str);
		str++;
	}
	if (*str != '\0') {
		str++;
	}
	*slines = str;
	
	return result;
}
		


bool Util::get_str_float(const char *s, const char *float_var, float *p_val)
{
	if (!s || !float_var || !p_val) {
		throw NullPointerException("string float");
	}
	size_t n = strlen(float_var);
	if (strncmp(s, float_var, n) == 0) {
		*p_val = (float) atof(&s[n]);
		return true;
	}
	
	return false;
}

bool Util::get_str_float(const char *s, const char *float_var, float *p_v1, float *p_v2)
{
	if (!s || !float_var || !p_v1 || !p_v2) {
		throw NullPointerException("string float");
	}

	size_t n = strlen(float_var);
	if (strncmp(s, float_var, n) == 0) {
		sscanf(&s[n], "%f,%f", p_v1, p_v2);
		return true;
	}

	return false;
}

bool Util::get_str_float(const char *s, const char *float_var,
						 int *p_v0, float *p_v1, float *p_v2)
{
	if (!s || !float_var || !p_v0 || !p_v1 || !p_v2) {
		throw NullPointerException("string float");
	}
	
	size_t n = strlen(float_var);
	*p_v0 = 0;
	if (strncmp(s, float_var, n) == 0) {
		if (s[n] == '=') {
			*p_v0 = 2;
			sscanf(&s[n + 1], "%f,%f", p_v1, p_v2);
		}
		else {
			*p_v0 = 1;
		}
		return true;
	}
	return false;
}

bool Util::get_str_int(const char *s, const char *int_var, int *p_val)
{
	if (!s || !int_var || !p_val) {
		throw NullPointerException("string int");
	}
	
	size_t n = strlen(int_var);
	if (strncmp(s, int_var, n) == 0) {
		*p_val = atoi(&s[n]);
		return true;
	}
	return false;
}

bool Util::get_str_int(const char *s, const char *int_var, int *p_v1, int *p_v2)
{
	if (!s || !int_var || !p_v1 || !p_v2) {
		throw NullPointerException("string int");
	}
	
	size_t n = strlen(int_var);
	if (strncmp(s, int_var, n) == 0) {
		sscanf(&s[n], "%d,%d", p_v1, p_v2);
		return true;
	}
	return false;
}

bool Util::get_str_int(const char *s, const char *int_var, int *p_v0, int *p_v1, int *p_v2)
{
	if (!s || !int_var || !p_v0 || !p_v1 || !p_v2) {
		throw NullPointerException("string int");
	}
	
	size_t n = strlen(int_var);
	*p_v0 = 0;
	if (strncmp(s, int_var, n) == 0) {
		if (s[n] == '=') {
			*p_v0 = 2;
			sscanf(&s[n + 1], "%d,%d", p_v1, p_v2);
		}
		else {
			*p_v0 = 1;
		}
		return true;
	}
	return false;
}

string Util::change_filename_ext(const string & old_filename,
								 const string & ext)
{
	Assert(old_filename != "");
	if (ext == "") {
		return old_filename;
	}
	
	string filename = old_filename;
	size_t dot_pos = filename.rfind(".");
	if (dot_pos != string::npos) {
		filename = filename.substr(0, dot_pos+1);
	}
	else {
		filename = filename + ".";
	}
	filename = filename + ext;
	return filename;
}

string Util::remove_filename_ext(const string& filename)
{
    if (filename == "") {
        return "";
    }
	
	char *buf = new char[filename.size()+1];
	strcpy(buf, filename.c_str());
	char *old_ext = strrchr(buf, '.');
	if (old_ext) {
		buf[strlen(buf) - strlen(old_ext)] = '\0';
	}
	string result = string(buf);
	if( buf )
	{
		delete [] buf;
		buf = 0;
	}
	return result;
}

string Util::sbasename(const string & filename)
{
    if (filename == "") {
        return "";
    }
	
	char s = '/';	
#ifdef WIN32
	s = '\\';
#endif
	char * c = strrchr(filename.c_str(), s);
    if (!c) {
        return filename;
    }
	else {
		c++;
	}
    return string(c);
}


string Util::get_filename_ext(const string& filename)
{
    if (filename == "") {
        return "";
    }

	string result = "";
	char *ext = strrchr(filename.c_str(), '.');
	if (ext) {
		ext++;
		result = string(ext);
	}
	return result;
}



void Util::calc_least_square_fit(size_t nitems, const float *data_x, const float *data_y,
								 float *slope, float *intercept, bool ignore_zero)
{
	Assert(nitems > 0);
	
	if (!data_x || !data_y || !slope || !intercept) {
		throw NullPointerException("null float pointer");
	}
	double sum = 0;
	double sum_x = 0;
	double sum_y = 0;
	double sum_xx = 0;
	double sum_xy = 0;

	for (size_t i = 0; i < nitems; i++) {
		if (!ignore_zero || (data_x[i] != 0 && data_y[i] != 0)) {
			double y = data_y[i];
			double x = i;
			if (data_x) {
				x = data_x[i];
			}

			sum_x += x;
			sum_y += y;
			sum_xx += x * x;
			sum_xy += x * y;
			sum++;
		}
	}

	double div = sum * sum_xx - sum_x * sum_x;
	if (div == 0) {
		div = 0.0000001f;
	}

	*intercept = (float) ((sum_xx * sum_y - sum_x * sum_xy) / div);
	*slope = (float) ((sum * sum_xy - sum_x * sum_y) / div);
}

void Util::save_data(const vector < float >&x_array, const vector < float >&y_array,
					 const string & filename)
{
	Assert(x_array.size() > 0);
	Assert(y_array.size() > 0);
	Assert(filename != "");
	
	if (x_array.size() != y_array.size()) {
		LOGERR("array x and array y have different size: %d != %d\n",
			   x_array.size(), y_array.size());
		return;
	}

	FILE *out = fopen(filename.c_str(), "wb");
	if (!out) {
		throw FileAccessException(filename);
	}

	for (size_t i = 0; i < x_array.size(); i++) {
		fprintf(out, "%g\t%g\n", x_array[i], y_array[i]);
	}
	fclose(out);
}

void Util::save_data(float x0, float dx, const vector < float >&y_array,
					 const string & filename)
{
	Assert(dx != 0);
	Assert(y_array.size() > 0);
	Assert(filename != "");
	
	FILE *out = fopen(filename.c_str(), "wb");
	if (!out) {
		throw FileAccessException(filename);
	}

	for (size_t i = 0; i < y_array.size(); i++) {
		fprintf(out, "%g\t%g\n", x0 + dx * i, y_array[i]);
	}
	fclose(out);
}


void Util::save_data(float x0, float dx, float *y_array, 
					 size_t array_size, const string & filename)
{
	Assert(dx > 0);
	Assert(array_size > 0);
	Assert(filename != "");
	
	if (!y_array) {
		throw NullPointerException("y array");
	}

	FILE *out = fopen(filename.c_str(), "wb");
	if (!out) {
		throw FileAccessException(filename);
	}

	for (size_t i = 0; i < array_size; i++) {
		fprintf(out, "%g\t%g\n", x0 + dx * i, y_array[i]);
	}
	fclose(out);
}

float Util::get_frand(int lo, int hi)
{
	return get_frand((float)lo, (float)hi);
}

float Util::get_frand(float lo, float hi)
{
	static bool inited = false;
	if (!inited) {
		srand(time(0));
		inited = true;
	}

	float r = (hi - lo) * rand() / (RAND_MAX + 1.0f) + lo;
	return r;
}

float Util::get_frand(double lo, double hi)
{
	static bool inited = false;
	if (!inited) {
		srand(time(0));
		inited = true;
	}

	double r = (hi - lo) * rand() / (RAND_MAX + 1.0) + lo;
	return (float)r;
}

float Util::get_gauss_rand(float mean, float sigma)
{
	float x = 0;
	float r = 0;
	bool valid = true;

	while (valid) {
		x = get_frand(-1.0, 1.0);
		float y = get_frand(-1.0, 1.0);
		r = x * x + y * y;

		if (r <= 1.0 && r > 0) {
			valid = false;
		}
	}

	float f = sqrt(-2.0f * log(r) / r);
	float result = x * f * sigma + mean;
	return result;
}

void Util::find_max(const float *data, size_t nitems, float *max_val, int *max_index)
{
	Assert(nitems > 0);
	
	if (!data || !max_val || !max_index) {
		throw NullPointerException("data/max_val/max_index");
	}
	float max = -FLT_MAX;
	int m = 0;

	for (size_t i = 0; i < nitems; i++) {
		if (data[i] > max) {
			max = data[i];
			m = (int)i;
		}
	}

	*max_val = (float)m;

	if (max_index) {
		*max_index = m;
	}
}

void Util::find_min_and_max(const float *data, size_t nitems,
							float *max_val, float *min_val,
							int *max_index, int *min_index)
{
	Assert(nitems > 0);
	
	if (!data || !max_val || !min_val || !max_index || !min_index) {
		throw NullPointerException("data/max_val/min_val/max_index/min_index");
	}
	float max = -FLT_MAX;
	float min = FLT_MAX;
	int max_i = 0;
	int min_i = 0;

	for (size_t i = 0; i < nitems; i++) {
		if (data[i] > max) {
			max = data[i];
			max_i = (int)i;
		}
		if (data[i] < min) {
			min = data[i];
			min_i = (int)i;
		}
	}

	*max_val = max;
	*min_val = min;

	if (min_index) {
		*min_index = min_i;
	}

	if (max_index) {
		*max_index = max_i;
	}

}



int Util::calc_best_fft_size(int low)
{
	Assert(low >= 0);
	
	//array containing valid sizes <1024 for speed
	static char *valid = NULL;

	if (!valid) {
		valid = (char *) calloc(4096, 1);

		for (float i2 = 1; i2 < 12.0; i2 += 1.0) {

			float f1 = pow((float) 2.0, i2);
			for (float i3 = 0; i3 < 8.0; i3 += 1.0) {

				float f2 = pow((float) 3.0, i3);
				for (float i5 = 0; i5 < 6.0; i5 += 1.0) {

					float f3 = pow((float) 5.0, i5);
					for (float i7 = 0; i7 < 5.0; i7 += 1.0) {

						float f = f1 * f2 * f3 * pow((float) 7.0, i7);
						if (f <= 4095.0) {
							int n = (int) f;
							valid[n] = 1;
						}
					}
				}
			}
		}
	}

	for (int i = low; i < 4096; i++) {
		if (valid[i]) {
			return i;
		}
	}

	LOGERR("Sorry, can only find good fft sizes up to 4096 right now.");

	return 1;
}

string Util::get_time_label()
{
	time_t t0 = time(0);
	struct tm *t = localtime(&t0);
	char label[32];
	sprintf(label, "%d/%02d/%04d %d:%02d",
			t->tm_mon + 1, t->tm_mday, t->tm_year + 1900, t->tm_hour, t->tm_min);	
	return string(label);
}


void Util::set_log_level(int argc, char *argv[])
{
	if (argc > 1 && strncmp(argv[1], "-v", 2) == 0) {
		char level_str[32];
		strcpy(level_str, argv[1] + 2);
		Log::LogLevel log_level = (Log::LogLevel) atoi(level_str);
		Log::logger()->set_level(log_level);
	}
}

void Util::printMatI3D(MIArray3D& mat, const string str, ostream& out) {
	// Note: Don't need to check if 3-D because 3D is part of 
	//       the MIArray3D typedef.
	out << "Printing 3D Integer data: " << str << std::endl;
	const multi_array_types::size_type* sizes = mat.shape();
	int nx = sizes[0], ny = sizes[1], nz = sizes[2];
	const multi_array_types::index* indices = mat.index_bases();
	int bx = indices[0], by = indices[1], bz = indices[2];
	for (int iz = bz; iz < nz+bz; iz++) {
		cout << "(z = " << iz << " slice)" << endl;
		for (int ix = bx; ix < nx+bx; ix++) {
			for (int iy = by; iy < ny+by; iy++) {
				cout << setiosflags(ios::fixed) << setw(5) 
					 << mat[ix][iy][iz] << "  ";
			}
			cout << endl;
		}
	}
}

vector<float>
Util::voea(float delta, float t1, float t2, float p1, float p2)
{
	vector<float> angles;
	float psi = 0.0;
	if ((0.0 == t1)&&(0.0 == t2)||(t1 >= t2)) {
		t1 = 0.0f;
		t2 = 90.0f;
	}
	if ((0.0 == p1)&&(0.0 == p2)||(p1 >= p2)) {
		p1 = 0.0f;
		p2 = 359.9f;
	}
	bool skip = ((t1 < 90.0)&&(90.0 == t2)&&(0.0 == p1)&&(p2 > 180.0));
	for (float theta = t1; theta <= t2; theta += delta) {
		float detphi;
		int lt;
		if ((0.0 == theta)||(180.0 == theta)) {
			detphi = 360.0f;
			lt = 1;
		} else {
			detphi = delta/sin(theta*dgr_to_rad);
			lt = int((p2 - p1)/detphi)-1;
			if (lt < 1) lt = 1;
			detphi = (p2 - p1)/lt;
		}
		for (int i = 0; i < lt; i++) {
			float phi = p1 + i*detphi;
			if (skip&&(90.0 == theta)&&(phi > 180.0)) continue;
			angles.push_back(phi);
			angles.push_back(theta);
			angles.push_back(psi);
		}
	}
	return angles;
}


float Util::triquad(double r, double s, double t, float f[]) {
	const float c2 = 1.0f / 2.0f;
	const float c4 = 1.0f / 4.0f;
	const float c8 = 1.0f / 8.0f;
	float rs = (float)(r*s);
	float st = (float)(s*t);
	float rt = (float)(r*t);
	float rst = (float)(r*st);
	float rsq = (float)(1 - r*r);
	float ssq = (float)(1 - s*s);
	float tsq = (float)(1 - t*t);
	float rm1 = (float)(1 - r);
	float sm1 = (float)(1 - s);
	float tm1 = (float)(1 - t);
	float rp1 = (float)(1 + r);
	float sp1 = (float)(1 + s);
	float tp1 = (float)(1 + t);

	return (float)(
		(-c8) * rst * rm1  * sm1  * tm1 * f[ 0] +
		( c4) * st	* rsq  * sm1  * tm1 * f[ 1] +
		( c8) * rst * rp1  * sm1  * tm1 * f[ 2] +
		( c4) * rt	* rm1  * ssq  * tm1 * f[ 3] +
		(-c2) * t	* rsq  * ssq  * tm1 * f[ 4] +
		(-c4) * rt	* rp1  * ssq  * tm1 * f[ 5] +
		( c8) * rst * rm1  * sp1  * tm1 * f[ 6] +
		(-c4) * st	* rsq  * sp1  * tm1 * f[ 7] +
		(-c8) * rst * rp1  * sp1  * tm1 * f[ 8] +

		( c4) * rs	* rm1  * sm1  * tsq * f[ 9] +
		(-c2) * s	* rsq  * sm1  * tsq * f[10] +
		(-c4) * rs	* rp1  * sm1  * tsq * f[11] +
		(-c2) * r	* rm1  * ssq  * tsq * f[12] +
					  rsq  * ssq  * tsq * f[13] +
		( c2) * r	* rp1  * ssq  * tsq * f[14] +
		(-c4) * rs	* rm1  * sp1  * tsq * f[15] +
		( c2) * s	* rsq  * sp1  * tsq * f[16] +
		( c4) * rs	* rp1  * sp1  * tsq * f[17] +

		( c8) * rst * rm1  * sm1  * tp1 * f[18] +
		(-c4) * st	* rsq  * sm1  * tp1 * f[19] +
		(-c8) * rst * rp1  * sm1  * tp1 * f[20] +
		(-c4) * rt	* rm1  * ssq  * tp1 * f[21] +
		( c2) * t	* rsq  * ssq  * tp1 * f[22] +
		( c4) * rt	* rp1  * ssq  * tp1 * f[23] +
		(-c8) * rst * rm1  * sp1  * tp1 * f[24] +
		( c4) * st	* rsq  * sp1  * tp1 * f[25] +
		( c8) * rst * rp1  * sp1  * tp1 * f[26]);
}

float Util::quadri(float x, float y, int nxdata, int nydata, 
			 EMData* image, int zslice) {
	// sanity check
	if (image->get_ysize() <= 1) {
		throw ImageDimensionException("Interpolated image must be at least 2D");
	}
	MArray3D fdata = image->get_3dview(1,1,1);
	// periodic boundary conditions
	if (x < 1.0) 
		x += (1-int(x)/nxdata)*nxdata;
	if (x > float(nxdata) + 0.5)
		x = fmodf(x - 1.0f, (float)nxdata) + 1.0f;
	if (y < 1.0)
		y += (1 - int(y)/nydata)*nydata;
	if (y > float(nydata) + 0.5f)
		y = fmodf(y - 1.0f, (float)nydata) + 1.0f;
	int i = int(x);
	int j = int(y);
	float dx0 = x - i;
	float dy0 = y - j;
	int ip1 = (i + 1) % nxdata + 1; // enforce ip1 in [1, nxdata]
	int im1 = (i - 1) % nxdata + 1;
	int jp1 = (j + 1) % nydata + 1;
	int jm1 = (j - 1) % nydata + 1;
	float f0 = fdata[i][j][zslice];
	float c1 = fdata[ip1][j][zslice] - f0;
	float c2 = (c1 - f0 + fdata[im1][j][zslice])*.5f;
	float c3 = fdata[i][jp1][zslice] - f0;
	float c4 = (c3 - f0 + fdata[i][jm1][zslice])*.5f;
	float dxb = dx0 - 1;
	float dyb = dy0 - 1;
	// hxc and hyc are either +1 or -1
	float hxc = (float)((dx0 >= 0) ? 1 : -1);
	float hyc = (float)((dy0 >= 0) ? 1 : -1);
	int ic = int(fmodf(i + hxc, float(nxdata)) + 1);
	int jc = int(fmodf(j + hyc, float(nydata)) + 1);
	float c5 = (fdata[ic][jc][zslice] - f0 - hxc*c1
				- (hxc*(hxc - 1.0f))*c2 - hyc*c3
				- (hyc*(hyc - 1.0f))*c4) * (hxc*hyc);
	float result = f0 + dx0*(c1 + dxb*c2 + dy0*c5)
				 + dy0*(c3 + dyb*c4);
	return result;
}

void Util::KaiserBessel::build_table()  {
	int lnb = -ln/2;
	int lne = -lnb;
	int n = 2*m;
	v = float(ln)/(2.0*float(n));
	rrr = float(m/2);
	// Adjust "v" to ensure that it is not zero at the window border
	float vadjust = 1.1*v;
	int ltab = int(float(ltabi)/1.25 + 0.5);
	if (ltabi > ltab) fill(tabi+ltab+1, tabi+ltabi+1, 0.f);
	float fac = twopi*alpha*rrr*vadjust;
	float b0 = sqrt(fac)*gsl_sf_bessel_I0(fac);
	float fltb = float(ltab)/float(lne);
	for (int i = 0; i <=ltab; i++) {
		float s = float(i)/(fltb*n);
		if (s < vadjust) {
			float xx = sqrt(1.0f - pow(s/vadjust, 2));
			tabi[i] = sqrt(fac*xx)*gsl_sf_bessel_I0(fac*xx)/b0;
		} else {
			tabi[i] = 0.0f;
		}
	}
}

float Util::KaiserBessel::kb1d(float x) {
	float fac = twopi*alpha*rrr*v;
	float kb_0 = sinh(fac)/(fac);
	float xscale = x/rrr;
	float kb_x = 0;
	if (0.0 == xscale) {
		kb_x = 1.0f;
	} else if (xscale < alpha) {
		float xx = sqrt(1.0f - pow((xscale/alpha), 2));
		kb_x = (sinh(fac*xx)/(fac*xx))/kb_0;
	} else if (xscale == alpha) {
		kb_x = 1.0f/kb_0;
	} else {
		float xx = sqrt(pow(xscale/alpha, 2) - 1.0f);
		kb_x = (sin(fac*xx)/(fac*xx))/kb_0;
	}
	return kb_x;
}

/* vim: set ts=4 noet: */
