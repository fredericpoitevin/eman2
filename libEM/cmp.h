/**
 * $Id$
 */
#ifndef eman_cmp__h__
#define eman_cmp__h__ 1


#include "emobject.h"

namespace EMAN
{

	class EMData;
	class Transform3D;

	/** Cmp class defines image comparison method. Using default
	 * arguments, smaller values indicate more similar images.
	 *
	 * Cmp class is the base comparison class. Each specific
     * comparison class is a subclass of Cmp, and must have a unique
     * name. The name is used to  create a new Cmp instance or call a Cmp.
	 *
	 * All Cmp classes in EMAN are managed by a Factory pattern. So 
	 * each Cmp class must define:
	 *   - a unique name to idenfity itself in the factory.
	 *   - a static method to register itself in the factory.
	 *
	 *
     * Typical usage of Cmp:
     *
     *  - How to get all Cmp names
     @code
     *      vector<string> all_cmps = Factory<Cmp>::get_list();
     @endcode
	 *
     *  - How to use a Cmp
     @code
     *      EMData *image1 = ...;
     *      EMData *image2 = ...;
	 *      Dict params = ...;
     *      float result = image1->cmp("CMP_NAME", image2, params);
     @endcode
	 *
     *  - How to define a new Cmp class \n
     *    A new XYZCmp class should implement the following functions:
	 *    (Please replace 'XYZ' with your own class name).
	 @code
     *        float cmp(EMData * image, EMData * with) const;
     *        TypeDict get_param_types() const;
     *        string get_name() const { return "XYZ"; }
     *        static Cmp *NEW() { return XYZCmp(); }
	 @endcode
     */

	class Cmp
	{
	  public:
		virtual ~ Cmp()
		{
		}
		
		/** To compare 'image' with another image passed in through
		 * its parameters. An optional transformation may be used
		 * to transform the 2 images.
		 *
		 * @param image The first image to be compared.
		 * @param with The second image to be comppared.
		 * @return The comparison result. Smaller better by default
		 */			
		virtual float cmp(EMData * image, EMData * with) const = 0;
		
		/** Get the Cmp's name. Each Cmp is identified by a unique name.
		 * @return The Cmp's name.
		 */
		virtual string get_name() const = 0;

		virtual string get_desc() const = 0;

		/** Get the Cmp parameters in a key/value dictionary.
		 * @return A key/value pair dictionary containing the parameters.
		 */
		virtual Dict get_params() const
		{
			return params;
		}

		/** Set the Cmp parameters using a key/value dictionary.
		 * @param new_params A dictionary containing the new parameters.
		 */
		virtual void set_params(const Dict & new_params)
		{
			params = new_params;
		}

		/** Get Cmp parameter information in a dictionary. Each
		 * parameter has one record in the dictionary. Each record
		 * contains its name, data-type, and description.
		 *
		 * @return A dictionary containing the parameter info.
		 */	 
		virtual TypeDict get_param_types() const = 0;
		
	protected:
		void validate_input_args(const EMData * image, const EMData *with) const;
		
		mutable Dict params;
	};

	/** Use dot product of 2 same-size images to do the comparison.
     * For complex images, it does not check r/i vs a/p.
    */
	class DotCmp:public Cmp
	{
	  public:
		float cmp(EMData * image, EMData * with) const;

		string get_name() const
		{
			return "dot";
		}

		string get_desc() const
		{
			return "Dot product * -1";
		}

		static Cmp *NEW()
		{
			return new DotCmp();
		}

		TypeDict get_param_types() const
		{
			TypeDict d;
			d.put("negative", EMObject::INT, "If set, returns -1 * dot product. Set by default so smaller is better");
			d.put("evenonly", EMObject::INT, "If set, consider only even numbered pixels.");
			return d;
		}

	};

	/** Variance between two data sets after various modifications. 
	* Generally, 'this' should be noisy and 'with' should be less noisy. 
	* linear scaling (mx + b) of the densities in 'this' is performed 
	* to produce the smallest possible variance between images. 
	*
	* If keepzero is set, then zero pixels are left at zero (b is not added).
	* if matchfilt is set, then 'with' is filtered so its radial power spectrum matches 'this'
	* If invert is set, then (y-b)/m is applied to the second image rather than mx+b to the first.
	*
	* To modify 'this' to match the operation performed here, scale 
	* should be applied first, then b should be added
	*/
	class OptVarianceCmp:public Cmp
	{
	  public:
		OptVarianceCmp() : scale(0), shift(0) {}
		
		float cmp(EMData * image, EMData * with) const;

		string get_name() const
		{
			return "optvariance";
		}

		string get_desc() const
		{
			return "Real-space variance after density optimization, self should be noisy and target less noisy. Linear transform applied to density to minimize variance.";
		}

		static Cmp *NEW()
		{
			return new OptVarianceCmp();
		}

		TypeDict get_param_types() const
		{
			TypeDict d;
			d.put("invert", EMObject::INT, "If set, 'with' is rescaled rather than 'this'. 'this' should still be the noisier image.");
			d.put("keepzero", EMObject::INT, "If set, zero pixels will not be adjusted in the linear density optimization");
			d.put("matchfilt", EMObject::INT, "If set, with will be filtered so its radial power spectrum matches 'this' before density optimization of this");
			d.put("debug", EMObject::INT, "Performs various debugging actions if set.");
			return d;
		}

		float get_scale() const
		{
			return scale;
		}

		float get_shift() const
		{
			return shift;
		}
		
	private:
		mutable float scale;
		mutable float shift;
	};
	
	/** Variance between 'this' and 'with' (no sqrt)*/
	class VarianceCmp:public Cmp
	{
	  public:
		VarianceCmp() {}
		
		float cmp(EMData * image, EMData * with) const;

		string get_name() const
		{
			return "variance";
		}

		string get_desc() const
		{
			return "Real-space variance sum(a^2 - b^2)/n.";
		}

		static Cmp *NEW()
		{
			return new VarianceCmp();
		}

		TypeDict get_param_types() const
		{
			TypeDict d;
			return d;
		}

	};

	/** Amplitude weighted mean phase difference (radians) with optional
     * SNR weight. SNR should be an array as returned by ctfcurve()
     * 'data' should be the less noisy image, since it's amplitudes 
     * will be used to weight the phase residual. 2D only.
	 *
     * Use Phase Residual as a measure of similarity
     * Differential phase residual (DPR) is a measure of statistical
     * dependency between two averages, computed over rings in Fourier
     * space as a function of ring radius (= spatial frequency, or resolution) 
     */
	class PhaseCmp:public Cmp
	{
	  public:
		float cmp(EMData * image, EMData * with) const;

		string get_name() const
		{
			return "phase";
		}

		string get_desc() const
		{
			return "Mean phase difference";
		}

		static Cmp *NEW()
		{
			return new PhaseCmp();
		}

		TypeDict get_param_types() const
		{
			TypeDict d;
			return d;
		}
	};

	/** FRCCmp returns a quality factor based on FRC between images.
     *  Fourier ring correlation (FRC) is a measure of statistical
     * dependency between two averages, computed by comparison of
     * rings in Fourier space. 1 means prefect agreement. 0 means no
     * correlation.    
     */
	class FRCCmp:public Cmp
	{
	  public:
		float cmp(EMData * image, EMData * with) const;

		string get_name() const
		{
			return "frc";
		}

		string get_desc() const
		{
			return "Mean Fourier ring correlation";
		}

		static Cmp *NEW()
		{
			return new FRCCmp();
		}

		TypeDict get_param_types() const
		{
			TypeDict d;
			d.put("snr", EMObject::FLOATARRAY);
			return d;
		}
	};

	template <> Factory < Cmp >::Factory();

	void dump_cmps();
}


#endif
