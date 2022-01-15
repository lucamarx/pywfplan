#include <exception>
#include <optional>
#include <string>
#include <vector>
#include <map>
#include <unordered_set>

#include <boost/python.hpp>

#include "regexp.h"
#include "shift.h"
#include "target.h"
#include "plan.h"
#include "staff_planner.h"

// Type that allows for registration of conversions from python
// iterable types.
//
// see https://stackoverflow.com/questions/15842126/feeding-a-python-list-into-a-function-taking-in-a-vector-with-boost-python
struct iterable_converter
{
  // Registers converter from a python interable type to the
  // provided type.
  template <typename Container>
  iterable_converter &
  from_python()
  {
  boost::python::converter::registry::push_back(&iterable_converter::convertible,
                                                  &iterable_converter::construct<Container>,
                                                  boost::python::type_id<Container>());
    // Support chaining.
 return *this;
  }

  // Check if PyObject is iterable.
  static void *convertible(PyObject *object)
  {
  return PyObject_GetIter(object) ? object : NULL;
  }

  // Convert iterable PyObject to C++ container type.
  //
  // Container Concept requirements:
  //
  //   * Container::value_type is CopyConstructable.
  //   * Container can be constructed and populated with two iterators.
  //     I.e. Container(begin, end)
  template <typename Container>
  static void construct(PyObject *object,
                        boost::python::converter::rvalue_from_python_stage1_data *data)
  {
  namespace python = boost::python;
    // Object is a borrowed reference, so create a handle indicting it is
    // borrowed for proper reference counting.
 python::handle<> handle(python::borrowed(object));

    // Obtain a handle to the memory block that the converter has allocated
    // for the C++ type.
 typedef python::converter::rvalue_from_python_storage<Container>
  storage_type;
 void *storage = reinterpret_cast<storage_type *>(data)->storage.bytes;

 typedef python::stl_input_iterator<typename Container::value_type>
  iterator;

    // Allocate the C++ type into the converter's memory block, and assign
    // its handle to the converter's convertible variable.  The C++
    // container is populated by passing the begin and end iterators of
    // the python object to the container's constructor.
 new (storage) Container(iterator(python::object(handle)), // begin
                            iterator());                      // end
 data->convertible = storage;
  }
};

// Convert optional from C++ to Python
//
// see https://stackoverflow.com/questions/36485840/wrap-boostoptional-using-boostpython
struct AttributeError : std::exception
{
  const char *what() const throw() { return "AttributeError exception"; }
};

struct TypeError : std::exception
{
  const char *what() const throw() { return "TypeError exception"; }
};

void translate(const std::exception &e)
{
  if (dynamic_cast<const AttributeError *>(&e))
    PyErr_SetString(PyExc_AttributeError, e.what());
  if (dynamic_cast<const TypeError *>(&e))
    PyErr_SetString(PyExc_TypeError, e.what());
}

template <typename T>
struct to_python_optional
{
  static PyObject *convert(const std::optional<T> &obj)
  {
    namespace bp = boost::python;

    if (obj.has_value())
      return bp::incref(bp::object(*obj).ptr());
    else
      throw AttributeError();
  }
};

template <typename T>
struct to_python_list
{
  static PyObject *convert(const T &container)
  {
    namespace bp = boost::python;

    bp::list l;
    for (auto x : container)
      l.append(x);

    return bp::incref(bp::object(l).ptr());
  }
};

template <typename K, typename V>
struct to_python_dict
{
  static PyObject *convert(const std::map<K, V> &map)
  {
    namespace bp = boost::python;

    bp::dict d;
    for (auto i : map)
      d[i.first] = i.second;

    return bp::incref(bp::object(d).ptr());
  }
};

BOOST_PYTHON_MODULE(pywfplan_ext)
{
  using namespace shift;
  using namespace target;
  using namespace plan;
  using namespace regexp;
  using namespace staff_planner;
  using namespace boost::python;

  Py_Initialize();

  // register iterable conversions
  iterable_converter()
    .from_python<std::vector<Shift>>()
    .from_python<std::vector<std::vector<int>>>()
    .from_python<std::vector<std::string>>()
    .from_python<std::vector<double>>()
    .from_python<std::vector<int>>();

  // register container conversion
  to_python_converter<std::vector<double>, to_python_list<std::vector<double>>>();
  to_python_converter<std::vector<std::string>, to_python_list<std::vector<std::string>>>();
  to_python_converter<std::vector<Shift>, to_python_list<std::vector<Shift>>>();
  to_python_converter<std::unordered_set<Shift>, to_python_list<std::unordered_set<Shift>>>();

  // register exception translators
  register_exception_translator<AttributeError>(&translate);
  register_exception_translator<TypeError>(&translate);

  // --------------------------------------------------------------------------------

  class_<Shift>("ShiftExt", "A work/rest shift to be assigned to an agent", init<std::string, std::vector<std::vector<int>>>())
    .def("__repr__", &Shift::to_string)
    .def("__eq__",   &Shift::operator==)
    .def("__ne__",   &Shift::operator!=)
    .def("code",     &Shift::code, "Get shift code")
    .def("work",     &Shift::work, "Check whether it is a work shift")
    .def("t0",       &Shift::t0,   "Enter time in minutes")
    .def("t1",       &Shift::t1,   "Exit time in minutes");

  // --------------------------------------------------------------------------------
  using regexp_t = RegExp<Shift>;

  class_<regexp_t>("ShiftRule", "Regular expression over shifts", init<Shift>())
    .def("__repr__",   &regexp_t::to_string)
    .def("__eq__",     &regexp_t::operator==)
    .def("__ne__",     &regexp_t::operator!=)
    .def("is_literal", &regexp_t::is_literal, "Check if regexp is literal")
    .def("shifts",     &regexp_t::alphabet,   "Extract the shifts from a rule")
    .def("shift",      &regexp_t::letter,     "Extract the shift from a literal")
    .def("kstar",      &regexp_t::kstar,      "Kleene star")
    .def(self * self)
    .def(self + self)
    .def(self & self);

  // --------------------------------------------------------------------------------

  class_<Target>("TargetExt", "The staffing target curve", init<unsigned int, unsigned int, std::vector<double>>())
    .def("__repr__", &Target::to_string);

  // --------------------------------------------------------------------------------

  class_<Plan>("PlanExt", "The staffing plan", init<unsigned int, std::vector<std::string>, Target>())
    .def("__repr__",           &Plan::to_string)
    .def("savePlan",           &Plan::savePlan,           "Save whole plan to file")
    .def("getAgentPlan",       &Plan::getAgentPlan,       "Get plan for agent")
    .def("saveStaffing",       &Plan::saveStaffing,       "Save staffing curves to file")
    .def("getTargetStaffing",  &Plan::getTargetStaffing,  "Get the (rescaled) target staffing curve")
    .def("getPlannedStaffing", &Plan::getPlannedStaffing, "Get the planned staffing curve");

  // --------------------------------------------------------------------------------

  class_<StaffPlanner>("StaffPlannerExt", "The planner itself", init<std::string, Plan, double, double>())
    .def("__repr__", &StaffPlanner::to_string)
    .def("run",             &StaffPlanner::run,             "Run simulation")
    .def("setAgentSampler", &StaffPlanner::setAgentSampler, "Set a sampler for an agent")
    .def("setWeek",         &StaffPlanner::setWeek,         "Set week to plan")
    .def("getPlan",         &StaffPlanner::getPlan,         "Retrieve the optimized plan")
    .def("getReport",       &StaffPlanner::getReport,       "Get the planning report");
}
