
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <emdata.h>
#include <quaternion.h>
#include <transform.h>
#include <vec3.h>

// Using =======================================================================
using namespace boost::python;

// Declarations ================================================================
namespace  {

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(EMAN_Transform3D_get_rotation_overloads_0_1, get_rotation, 0, 1)


}// namespace 


// Module ======================================================================
BOOST_PYTHON_MODULE(libpyTransform2)
{
    class_< EMAN::Vec3f, boost::noncopyable >("Vec3f", init<  >())
        .def(init< float, float, optional< float > >())
        .def(init< const std::vector<float,std::allocator<float> >& >())
        .def(init< const EMAN::Vec3i& >())
        .def("normalize", &EMAN::Vec3f::normalize)
        .def("length", &EMAN::Vec3f::length)
        .def("dot", &EMAN::Vec3f::dot)
        .def("cross", &EMAN::Vec3f::cross)
        .def("as_list", &EMAN::Vec3f::as_list)
        .def("set_value", (void (EMAN::Vec3f::*)(const std::vector<float,std::allocator<float> >&) )&EMAN::Vec3f::set_value)
        .def("set_value", (void (EMAN::Vec3f::*)(float, float, float) )&EMAN::Vec3f::set_value)
        .def("at", &EMAN::Vec3f::at)
        .def( other< EMAN::Vec3i >() + self )
        .def( self != self )
        .def( self == self )
        .def( self / other< float >() )
        .def( self * other< float >() )
        .def( other< float >() * self )
        .def( self - other< float >() )
        .def( other< float >() - self )
        .def( self + other< float >() )
        .def( other< float >() + self )
        .def( other< EMAN::Vec3i >() * self )
        .def( self + other< EMAN::Vec3i >() )
        .def( self - self )
        .def( self * self )
        .def( -self )
        .def( self * other< EMAN::Vec3i >() )
        .def( other< EMAN::Vec3i >() - self )
        .def( self - other< EMAN::Vec3i >() )
        .def( self + self )
        .def( self += self )
        .def( self += other< EMAN::Vec3i >() )
        .def( self += other< float >() )
        .def( self -= self )
        .def( self -= other< EMAN::Vec3i >() )
        .def( self -= other< float >() )
        .def( self *= other< float >() )
        .def( self /= other< float >() )
    ;

    class_< EMAN::Vec3i, boost::noncopyable >("Vec3i", init<  >())
        .def(init< int, int, optional< int > >())
        .def(init< const std::vector<int,std::allocator<int> >& >())
        .def("normalize", &EMAN::Vec3i::normalize)
        .def("length", &EMAN::Vec3i::length)
        .def("dot", &EMAN::Vec3i::dot)
        .def("cross", &EMAN::Vec3i::cross)
        .def("as_list", &EMAN::Vec3i::as_list)
        .def("set_value", (void (EMAN::Vec3i::*)(const std::vector<int,std::allocator<int> >&) )&EMAN::Vec3i::set_value)
        .def("set_value", (void (EMAN::Vec3i::*)(int, int, int) )&EMAN::Vec3i::set_value)
        .def( other< int >() - self )
        .def( self - other< int >() )
        .def( other< int >() * self )
        .def( self * self )
        .def( self / other< int >() )
        .def( self * other< int >() )
        .def( self != self )
        .def( self == self )
        .def( self - self )
        .def( other< int >() + self )
        .def( self + self )
        .def( self + other< int >() )
        .def( self + other< EMAN::Vec3f >() )
        .def( self * other< EMAN::Vec3f >() )
        .def( other< EMAN::Vec3f >() + self )
        .def( other< EMAN::Vec3f >() * self )
        .def( self - other< EMAN::Vec3f >() )
        .def( other< EMAN::Vec3f >() - self )
        .def( self += self )
        .def( self += other< int >() )
        .def( self -= self )
        .def( self -= other< int >() )
        .def( self *= other< int >() )
        .def( self /= other< int >() )
    ;

    class_< EMAN::Quaternion, boost::noncopyable >("Quaternion", init<  >())
        .def(init< float, float, float, float >())
        .def(init< float, const EMAN::Vec3f& >())
        .def(init< const EMAN::Vec3f&, float >())
        .def(init< const std::vector<float,std::allocator<float> >& >())
        .def("norm", &EMAN::Quaternion::norm)
        .def("conj", &EMAN::Quaternion::conj)
        .def("abs", &EMAN::Quaternion::abs)
        .def("normalize", &EMAN::Quaternion::normalize)
        .def("inverse", &EMAN::Quaternion::inverse, return_internal_reference< 1 >())
        .def("create_inverse", &EMAN::Quaternion::create_inverse)
        .def("rotate", &EMAN::Quaternion::rotate)
        .def("to_angle", &EMAN::Quaternion::to_angle)
        .def("to_axis", &EMAN::Quaternion::to_axis)
        .def("to_matrix3", &EMAN::Quaternion::to_matrix3)
        .def("real", &EMAN::Quaternion::real)
        .def("unreal", &EMAN::Quaternion::unreal)
        .def("as_list", &EMAN::Quaternion::as_list)
        .def("interpolate", &EMAN::Quaternion::interpolate)
        .staticmethod("interpolate")
        .def( self != self )
        .def( self == self )
        .def( self / self )
        .def( other< float >() * self )
        .def( self * other< float >() )
        .def( self * self )
        .def( self - self )
        .def( self + self )
        .def( self += self )
        .def( self -= self )
        .def( self *= self )
        .def( self *= other< float >() )
        .def( self /= self )
        .def( self /= other< float >() )
    ;

    scope* EMAN_Transform3D_scope = new scope(
    class_< EMAN::Transform3D, boost::noncopyable >("Transform3D", init<  >())
        .def(init< float, float, float >())
        .def(init< const EMAN::Vec3f&, float, float, float >())
        .def(init< EMAN::Transform3D::EulerType, float, float, float >())
        .def(init< EMAN::Transform3D::EulerType, EMAN::Dict& >())
        .def(init< const EMAN::Vec3f&, const EMAN::Vec3f&, float, float, float >())
        .def_readonly("ERR_LIMIT", &EMAN::Transform3D::ERR_LIMIT)
        .def("set_posttrans", &EMAN::Transform3D::set_posttrans)
        .def("apply_scale", &EMAN::Transform3D::apply_scale)
        .def("set_scale", &EMAN::Transform3D::set_scale)
        .def("orthogonalize", &EMAN::Transform3D::orthogonalize)
        .def("set_rotation", (void (EMAN::Transform3D::*)(float, float, float) )&EMAN::Transform3D::set_rotation)
        .def("set_rotation", (void (EMAN::Transform3D::*)(EMAN::Transform3D::EulerType, float, float, float) )&EMAN::Transform3D::set_rotation)
        .def("set_rotation", (void (EMAN::Transform3D::*)(EMAN::Transform3D::EulerType, EMAN::Dict&) )&EMAN::Transform3D::set_rotation)
        .def("get_posttrans", (EMAN::Vec3f (EMAN::Transform3D::*)(EMAN::Vec3f&) const)&EMAN::Transform3D::get_posttrans)
        .def("get_posttrans", (EMAN::Vec3f (EMAN::Transform3D::*)() const)&EMAN::Transform3D::get_posttrans)
        .def("get_center", &EMAN::Transform3D::get_center)
        .def("get_matrix3_col", &EMAN::Transform3D::get_matrix3_col)
        .def("get_matrix3_row", &EMAN::Transform3D::get_matrix3_row)
        .def("get_rotation", &EMAN::Transform3D::get_rotation, EMAN_Transform3D_get_rotation_overloads_0_1())
        .def("print", &EMAN::Transform3D::print)
        .def("fubar", &EMAN::Transform3D::fubar)
        .def("at", &EMAN::Transform3D::at)
        .def("get_nsym", &EMAN::Transform3D::get_nsym)
        .def("get_sym", &EMAN::Transform3D::get_sym)
        .def("set_center", &EMAN::Transform3D::set_center)
        .def("set_pretrans", &EMAN::Transform3D::set_pretrans)
        .def("get_scale", &EMAN::Transform3D::get_scale)
        .def("to_identity", &EMAN::Transform3D::to_identity)
        .def("is_identity", &EMAN::Transform3D::is_identity)
        .staticmethod("get_nsym")
        .def( other< EMAN::Vec3f >() * self )
        .def( self * self )
        .def( self * other< EMAN::Vec3f >() )
    );

    enum_< EMAN::Transform3D::EulerType >("EulerType")
        .value("UNKNOWN", EMAN::Transform3D::UNKNOWN)
        .value("IMAGIC", EMAN::Transform3D::IMAGIC)
        .value("SPIDER", EMAN::Transform3D::SPIDER)
        .value("QUATERNION", EMAN::Transform3D::QUATERNION)
        .value("SGIROT", EMAN::Transform3D::SGIROT)
        .value("MRC", EMAN::Transform3D::MRC)
        .value("SPIN", EMAN::Transform3D::SPIN)
        .value("EMAN", EMAN::Transform3D::EMAN)
    ;

    delete EMAN_Transform3D_scope;

}

